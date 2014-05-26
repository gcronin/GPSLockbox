// Code credit to GekoCH:  http://forum.arduino.cc/index.php?topic=7490.0
// Modified to parse GGA strings

#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <Wire.h> 

///////////////////////////////////////////////////////LED SETUP///////////////////////////////////////////////////
int RGBLed[] = {13, 10, 11};
byte Colors[][3]={
      {255, 0, 0},  //RED
      {0, 255, 0},   // GREEN
      {0, 0, 255},  //BLUE
      {220, 50, 0},  //YELLOW
};
byte RED[] = {Colors[0][0], Colors[0][1], Colors[0][2]};
byte GREEN[] = {Colors[1][0], Colors[1][1], Colors[1][2]};
byte BLUE[] = {Colors[2][0], Colors[2][1], Colors[2][2]};
byte YELLOW[] = {Colors[3][0], Colors[3][1], Colors[3][2]};
long HotColdScaleFactor = 1000;
long elapsedTime = 0;
int increment2 = 0;
int increment1 = 0;
boolean NewColor = true;
int changeRed, changeBlue, changeGreen, steps;

/////////////////////////////////////////////////////////COMPUTER SERIAL SETUP///////////////////////////////////////
#define SerialON 1

/////////////////////////////////////////////////////////GPS SETUP////////////////////////////////////////////////////
SoftwareSerial mySerial(9, 8, false); //Green in 9 (rx), Yellow in 8 (tx)
#define BAUDRATE 38400
#define BUFFERSIZE 90
char buffer[BUFFERSIZE];
char *parseptr;
char bufferIndex;
uint8_t minute, second, year, month, date;
int8_t hour;
const int TimeZone = -7;
char ampm[3];
uint32_t latitude, longitude, altitude, targetLatitude, targetLongitude;
char latdir, longdir;
float targetDistance;   //how far away the target is
int precision = 50;   //we need to be within this value of the target in order to open the lockbox

 
/////////////////////////////////////////////////////KEYPAD SETUP/////////////////////////////////////////////////////// 
#define PCA9555 0x20 // address for PCA9555
#define OUT_P0 0x02 // Write Output port0
#define CONFIG_P0 0x06 // Configuration port0 configures the direction of the I/O pins, 0 is output, 1 is input
#define IN_P0 0x00 //Read Input port0
 
#if defined(ARDUINO) && ARDUINO >= 100
#define printIIC(args) Wire.write((uint8_t)args)
#define readIIC() Wire.read()
#else
#define printIIC(args) Wire.send(args)
#define readIIC() Wire.receive()
#endif

/////////////////////////////////////////////////SERVO SETUP//////////////////////////////////////////////////////////////
int servoPin = 5;
int servoEnable = 7;
Servo myservo; 

/////////////////////////////////////////////////LCD SETUP//////////////////////////////////////////////////////////////
LiquidCrystal lcd(A5, A4, A0, A3, A2, A1);
byte displayType = 0;   //0 = coordinates, 1 = time, 2 = distance to target
const int gpsCoordinateInputBufferSize = 9;
char input[gpsCoordinateInputBufferSize]; //buffer for input characters
int lcdcolumnindexrow1 = 0; // used to indicate location of input key


///////////////////////////////////////////////VARIABLE SETUP//////////////////////////////////////////////////////////
boolean SETUP = false;
boolean SETUPLATITUDE = false;
boolean SETUPLONGITUDE = false; 
boolean SETUPDONE = false;
boolean LOCKED = false;
boolean STARTINGDISTANCETOTARGET = false;



void setup()  
{
  //SERVO SETUP
  myservo.attach(servoPin);  
  pinMode(servoEnable, OUTPUT);
  digitalWrite(servoEnable, LOW);  
  
  //KEYPAD SETUP
  Wire.begin(PCA9555); // join i2c bus (address optional for master) tried to get working
  write_io (CONFIG_P0, B00001111); //define port 0.7-0.4 as output, 0.3-0.0 as input
  
  //LCD SETUP
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("Press A for GPS");
  lcd.setCursor(0,1);
  lcd.print("Coordinate Setup");

  //SERIAL TO COMPUTER
  if(SerialON) Serial.begin(38400);
  
  //GPS SETUP
  mySerial.begin(BAUDRATE);
  ampm[1] = 'M';
  
  //LED SETUP
  for(int i=0; i<3; i++) pinMode(RGBLed[i], OUTPUT);
  setColor(RGBLed, RED);

}

void loop()
{
  if(SETUPDONE)  {  readline(); }
  unsigned char key;             
  key=readdata();
  while (readdata()==key && key!= 'E')  {}  //keypad debounce
  if(key == 'B') {displayType = (displayType+1)%3;  lcd.clear(); } // Setup LCD Display Mode
  else if(key == 'C') {   // Toggle Locked Mode
    if(LOCKED) unlockBox();
    else lockBox();
  } 
  else if(key == 'A')  // Check for setup mode
  {   
    if(!LOCKED) lockBox();
    SETUPDONE = false;
    setColor(RGBLed, GREEN);
    delay(2000);
    lcd.clear();
    lcd.print("Enter Latitude");
    lcd.setCursor(0,1);
    lcd.print("ddmmmmmm");
    SETUPLATITUDE = true;
    for(int i = 0; i < gpsCoordinateInputBufferSize; i++) { input[i] = '0'; }  // initialize input as zero
    lcdcolumnindexrow1 = 0;
    setColor(RGBLed, RED);
  }
  else if(SETUPLATITUDE)  // user input of latitude
  {
    if(key != 'E') {// received a keystroke
        if(key == 'D') {
          lcd.clear();
          lcd.print("Got Target Lat");
          setColor(RGBLed, GREEN);
          delay(1500);
          SETUPLATITUDE = false;
          SETUPLONGITUDE = true; 
          targetLatitude = atol(input);
          Serial.println(targetLatitude);
          lcd.clear();
          lcd.print("Enter Longitude");
          lcd.setCursor(0,1);
          lcd.print("dddmmmmmm");
          for(int i = 0; i < gpsCoordinateInputBufferSize; i++) { input[i] = '0'; }  // initialize input as zero
          lcdcolumnindexrow1 = 0;
          setColor(RGBLed, RED);
         }
        else if(key != 'B' && key!= 'C' && key != '#' && key != '*' && key != 'A') {        
          if(lcdcolumnindexrow1 < (gpsCoordinateInputBufferSize - 1)) {  // take 8 key strokes only
            for(int i =0; i < (gpsCoordinateInputBufferSize-1); i++) {input[i] = input[i+1]; }  //shift all the values left
            input[(gpsCoordinateInputBufferSize-1)] = key;
            lcd.setCursor(lcdcolumnindexrow1,1);  //print on left of LCD moving right
            lcd.print(input[(gpsCoordinateInputBufferSize-1)]);
            ++lcdcolumnindexrow1;
            //byte RedToGreen[] = {(255-255*pow(lcdcolumnindexrow1,2)/pow((gpsCoordinateInputBufferSize - 1), 2)), 255*pow(lcdcolumnindexrow1,2)/pow((gpsCoordinateInputBufferSize - 1), 2), 0};
            //setColor(RGBLed, YELLOW);
          }
          else {
            lcd.setCursor(10, 1);
            lcd.print("Hit D");
          }
        }
    }
  }
  else if(SETUPLONGITUDE)  // user input of longitude
  {
    if(key != 'E') {// received a keystroke
        if(key == 'D') {
          lcd.clear();
          lcd.print("Got Target Long");
          setColor(RGBLed, GREEN);
          delay(1500);
          SETUPLONGITUDE = false;
          SETUP = true; 
          targetLongitude = atol(input);
          Serial.println(targetLongitude);
          lcd.clear();
          lcd.println("Setup Done      ");
          delay(2000);
          SETUPDONE = true;
          setColor(RGBLed, BLUE);
          }
        else if(key != 'B' && key!= 'C' && key != '#' && key != '*' && key != 'A') {
          if(lcdcolumnindexrow1 < gpsCoordinateInputBufferSize) {  // take 9 key strokes only
            for(int i =0; i < (gpsCoordinateInputBufferSize-1); i++) {input[i] = input[i+1]; }  //shift all the values left
            input[(gpsCoordinateInputBufferSize-1)] = key;
            lcd.setCursor(lcdcolumnindexrow1,1);  //print on left of LCD moving right
            lcd.print(input[(gpsCoordinateInputBufferSize-1)]);
            ++lcdcolumnindexrow1;
            //byte RedToGreen[] = {(255-255*lcdcolumnindexrow1/gpsCoordinateInputBufferSize), 255*lcdcolumnindexrow1/gpsCoordinateInputBufferSize, 0};
            //setColor(RGBLed, YELLOW);
           }
           else {
             lcd.setCursor(10, 1);
             lcd.print("Hit D");
           }
        }
    }
  }
  else if(!SETUP) { fadeColor(50);  }  // infinite loop for the very beginner so it keeps displaying setup prompt
  else {
      uint32_t tmp;
      int startPosition;
      boolean Parse = false;
      if(StringContains(buffer, "$GPGGA")!= -1 && StringContains(buffer, "*") > StringContains(buffer, "$GPGGA")) {  // this means we've found GPGGA and a * to the right indicating the checksum
        startPosition = StringContains(buffer, "$GPGGA") +7;
        Parse = true;
      }
        
      if(Parse) { 
       // hhmmss time data
        parseptr = buffer+startPosition;
        tmp = parsedecimal(parseptr); 
        hour = tmp / 10000;
        minute = (tmp / 100) % 100;
        second = tmp % 100;

        //Time zone adjustment
        hour += TimeZone;
        if (hour < 0)    hour += 24;
        if (hour >= 24)  hour -= 24;
        if (hour > 12) { 
            hour -= 12;
            ampm[0] = 'P'; }
        else ampm[0] = 'A';
    
        // grab latitude & long data
        // latitude
        parseptr = strchr(parseptr, ',') + 1;
        latitude = parsedecimal(parseptr);
        if (latitude != 0) {
            latitude *= 10000;
            parseptr = strchr(parseptr, '.')+1;
            latitude += parsedecimal(parseptr);
        }
        parseptr = strchr(parseptr, ',') + 1;
        // read latitude N/S data
        if (parseptr[0] != ',') {
        latdir = parseptr[0];
        }
    
        // longitude
        parseptr = strchr(parseptr, ',')+1;
        longitude = parsedecimal(parseptr);
        if (longitude != 0) {
          longitude *= 10000;
          parseptr = strchr(parseptr, '.')+1;
          longitude += parsedecimal(parseptr);
        }
        parseptr = strchr(parseptr, ',')+1;
        // read longitude E/W data
        if (parseptr[0] != ',') {
          longdir = parseptr[0];
        }
    
        for(int i = 0; i<4; i++) {parseptr = strchr(parseptr, ',')+1; }  //Skip over the next three fields
        altitude = parsedecimal(parseptr);    
        if (altitude != 0) {
          altitude *= 100;
          parseptr = strchr(parseptr, '.')+1;
          altitude += parsedecimal(parseptr);
        }
        
        if(!STARTINGDISTANCETOTARGET && (latitude+longitude+altitude)!=0)
        {
          //find the distance from starting to ending locations
          HotColdScaleFactor = long(distanceToTarget(targetLatitude, targetLongitude, latitude, longitude, 1.0));     
          STARTINGDISTANCETOTARGET = true; //only do this once
        }
        if(SerialON) {
           Serial.print("scale factor: ");
           Serial.println(HotColdScaleFactor); }
        
        if(checkDistance(precision)) {
          SETUPDONE = false;
          if(SerialON) Serial.println("Opening");
          lcd.clear();
          lcd.print("Arrived!");
          if(LOCKED) unlockBox();
          fadeColor(0); }
        else { 
          byte HotColdRed = (targetDistance-precision) > HotColdScaleFactor ? 0 : (255 - 255*(targetDistance-precision)/HotColdScaleFactor);
          byte HotColdBlue = (targetDistance-precision) > HotColdScaleFactor ? 255 : (255*(targetDistance-precision)/HotColdScaleFactor);
          byte HotColdColor[] = {HotColdRed, 0, HotColdBlue};
          setColor(RGBLed, HotColdColor);  
        }

                
        if(SerialON) {
          Serial.print("\nTime: ");
          Serial.print(hour, DEC); Serial.print(":");
          if(minute < 10) Serial.print("0");
          Serial.print(minute, DEC); Serial.print(".");
          if(second < 10) Serial.print("0");
          Serial.print(second, DEC); Serial.print(" ");
          Serial.println(ampm);
          Serial.print("Lat: "); 
          if (latdir == 'N')
             Serial.print("+");
          else if (latdir == 'S')
             Serial.print("-");
          Serial.print(latitude/1000000, DEC); Serial.print("\260"); Serial.print(" ");
          Serial.print((latitude/10000)%100, DEC); Serial.print(".");
          Serial.print(latitude%10000, DEC); Serial.println("\'"); 
          Serial.print("Long: "); 
          if (longdir == 'E')
             Serial.print("+");
          else if (longdir == 'W')
             Serial.print("-");
          Serial.print(longitude/1000000, DEC); Serial.print("\260"); Serial.print(" ");
          Serial.print((longitude/10000)%100, DEC); Serial.print(".");
          Serial.print(longitude%10000, DEC); Serial.println("\'"); 
          Serial.print("Altitude: "); 
          Serial.print(altitude/100, DEC); Serial.print(".");
          Serial.print(altitude%100, DEC); Serial.println(" m");
        }
        if(displayType==0) {
          lcd.setCursor(0,0);
          lcd.print("Lat:"); 
          if (latdir == 'N')
              lcd.print("+");
          else if (latdir == 'S')
              lcd.print("-"); 
          lcd.print(latitude/1000000);
          lcd.print(char(0x00+223)); //degree symbol
          lcd.print((latitude/10000)%100);
          lcd.print(".");
          lcd.print(latitude%10000);
          lcd.print("\'");
          lcd.setCursor(0,1);
          lcd.print("Lon:"); 
          if (longdir == 'E')
              lcd.print("+");
          else if (longdir == 'W')
              lcd.print("-");
          lcd.print(longitude/1000000);
          lcd.print(char(0x00+223));
          lcd.print((longitude/10000)%100);
          lcd.print(".");
          lcd.print(longitude%10000);
        }
       else if(displayType==1) {
          String totalTime = "";
          totalTime +=hour;
          totalTime +=":";
          if(minute < 10) totalTime += "0";
          totalTime += minute;
          totalTime += ".";
          if(second < 10) totalTime += "0";
          totalTime += second;
          totalTime += " ";
          totalTime += ampm;
          lcd.setCursor(0,0);
          lcd.print("TIME:           ");
          lcd.setCursor(0,1);
          lcd.print(totalTime);
        }
       else {
          lcd.setCursor(0,0);
          lcd.print("Distance To Go:");
          lcd.setCursor(0,1);
          lcd.print(targetDistance);
          lcd.setCursor(15,1);
          lcd.print("m");
       }
       
    
     }

  }
}

////////////////////////////////////////////////////////////////////LED FUNCTIONS////////////////////////////////////////////////////////////
void setColor(int* led, byte* color){
  for(int i = 0; i < 3; i++){
    //iterate through each of the three pins (red green blue)
    analogWrite(led[i], 255 - color[i]);
  //set the analog output value of each pin to the input value (ie led[0] (red pin) to 255- color[0] (red input color)
  //we use 255 - the value because our RGB LED is common anode, this means a color is full on when we output analogWrite(pin, 0)
  //and off when we output analogWrite(pin, 255).
  }
}

void fadeColor(int delayTime) {     
  if(NewColor) {
    changeRed = Colors[(increment1+1)%3][0] - Colors[increment1][0];  //the difference in the two colors for the red channel
    changeGreen = Colors[(increment1+1)%3][1] - Colors[increment1][1];  //the difference in the two colors for the green channel
    changeBlue =  Colors[(increment1+1)%3][2] - Colors[increment1][2];  //the difference in the two colors for the blue channel
    steps = max(abs(changeRed),max(abs(changeGreen), abs(changeBlue)))/40;
    NewColor = false;
  }

  if((millis() - elapsedTime) > delayTime) {
      byte newRed = Colors[increment1][0] + (increment2 * changeRed / steps);  //the newRed intensity dependant on the start intensity and the change determined above
      byte newGreen =  Colors[increment1][1] + (increment2 * changeGreen / steps);     //the newGreen intensity
      byte newBlue = Colors[increment1][2] + (increment2 * changeBlue / steps);     //the newBlue intensity
      byte newColor[] = {newRed, newGreen, newBlue};    //Define an RGB color array for the new color
      setColor(RGBLed, newColor); //Set the LED to the calculated value
      Serial.print(newRed);
      Serial.print("  ");
      Serial.print(newGreen);
      Serial.print("  ");
      Serial.println(newBlue);
      increment2 = (increment2+1)%255;
      elapsedTime = millis();
      if(increment2 == (steps-1)) {
          increment2 = 0;
          increment1 = (increment1 + 1)%3;
          NewColor = true;
      }
   }
}

/////////////////////////////////////////////////////////////////////LOCKBOX FUNCTIONS////////////////////////////////////////////////////////
boolean checkDistance(int precision) {
  targetDistance = distanceToTarget(targetLatitude, targetLongitude, latitude, longitude, 1.0);
  if(targetDistance < precision){ 
    return true;
  }
  else return false;
}

void lockBox() {
    digitalWrite(servoEnable, HIGH);
    delay(100);
    myservo.writeMicroseconds(1400);
    delay(2000);
    digitalWrite(servoEnable, LOW);
    LOCKED = true;
}

void unlockBox() {
    digitalWrite(servoEnable, HIGH);
    delay(100);
    myservo.writeMicroseconds(2100);
    delay(2000);
    digitalWrite(servoEnable, LOW);
    LOCKED = false;
}


/////////////////////////////////////////////////////////////////////DISTANCE FUNCTIONS////////////////////////////////////////////////////////
float distance_between(float lat1, float long1, float lat2, float long2, float units_per_meter) {
	// returns distance in meters between two positions, both specified
	// as signed decimal-degrees latitude and longitude. Uses great-circle
	// distance computation for hypothised sphere of radius 6372795 meters.
	// Because Earth is no exact sphere, rounding errors may be upto 0.5%.
  float delta = radians(long1-long2);
  float sdlong = sin(delta);
  float cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  float slat1 = sin(lat1);
  float clat1 = cos(lat1);
  float slat2 = sin(lat2);
  float clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta = sq(delta);
  delta += sq(clat2 * sdlong);
  delta = sqrt(delta);
  float denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  return delta * 6372795 * units_per_meter;
}

float convertRawToFloat(uint32_t rawGPS) {
  float decimalGPS = float(rawGPS);
  decimalGPS /=1000000;
  return decimalGPS;
}

float distanceToTarget(uint32_t lat1, uint32_t long1, uint32_t lat2, uint32_t long2, float units_per_meter)
{
  float targetLat = convertRawToFloat(lat1);
  float targetLong = convertRawToFloat(long1);
  float currentLat = convertRawToFloat(lat2);
  float currentLong = convertRawToFloat(long2);
  return distance_between(targetLat, targetLong, currentLat, currentLong, units_per_meter);
}


/////////////////////////////////////////////////////////////////////GPS FUNCTIONS/////////////////////////////////////////////////////////////
int StringContains(String s, String search) {
    int max = s.length() - search.length();
    int lgsearch = search.length();

    for (int i = 0; i <= max; i++) {
        if (s.substring(i, i + lgsearch) == search) return i;
    }

 return -1;
}

uint32_t parsedecimal(char *str) {
  uint32_t d = 0;
  
  while (str[0] != 0) {
   if ((str[0] > '9') || (str[0] < '0'))
     return d;
   d *= 10;
   d += str[0] - '0';
   str++;
  }
  return d;
}

void readline(void) {
  char c;
  
  bufferIndex = 0;
  while (1) {
      c=mySerial.read();    // read a byte
      if (c == -1)          // if the byte is -1 don't record and skip to the next byte
        continue;
      //Serial.print(c);
      if (c == '\n')        // if the byte is newline don't record and skip to the next byte
        continue;
      if ((bufferIndex == BUFFERSIZE-1) || (c == '\r')) {    // if buffer overflow OR byte is a carriage return, reset the buffer and quit readline
        buffer[bufferIndex] = 0;
        return;
      }
      buffer[bufferIndex++]= c;  // otherwise store the byte
  }
}



//////////////////////////////////////////////////////////////////KEYPAD FUNCTIONS//////////////////////////////////////////////////////////////
unsigned char readdata(void)          //main read function
{
                       //binary 10000000, set pin 0.7 HIGH
  for (int i=0;i<4;i++)               //for loop
  {
    int input=0xF0; 
    input ^= (1 << (i+4));
    write_io (OUT_P0, input);
      
    for (int j=0;j<4;j++)
      {  
        unsigned int temp=0x0F;            //temporary integer, binary 1111, to compare with gpio_read() function and determine pression 
        temp ^= (1 << j);  
        //Serial.println(gpio_read(PCA9555));
        if (gpio_read(PCA9555)==temp)
         { return outputchar(i,j);}   // output the char
      }
    input=input>>1;                   //shift right, set the next pin HIGH, set previous one LOW
  }
  return 'E';                         // if no button is pressed, return E
}
  
 
 void write_io(int command, int value)  //write into register
{
  Wire.beginTransmission(PCA9555);      //begin transmission  
  printIIC(command);                  //which register
  printIIC(value);                    //write value
  Wire.endTransmission();               //end transmission
}
 
unsigned int gpio_read(int address)   //read from pin 0.3 ~ 0.0
{
  int data = 0;                   
  Wire.beginTransmission(address);
  printIIC(IN_P0);                // warning: this may be a bug in Arduino 1.0. transform 0x00 (input register) into byte 0, otherwise this compiler will return error
  Wire.endTransmission();
  Wire.beginTransmission(address);
  Wire.requestFrom(address, 1);
  if (Wire.available()) 
  {   
    data = readIIC( )&0x0F ;           // read lower 4 bit  0.3 ~ 0.0
        
  }
  Wire.endTransmission();
  return data;
}
 
 
unsigned char outputchar(int i, int j)   // output chars on the pad
{
  if (i==0)
  { switch (j)
    {case 0:           return '1'; break;
     case 1:           return '2'; break;
     case 2:           return '3'; break;
     case 3:           return 'A'; break;
    }
  }
   if (i==1)
  { switch (j)
    {case 0:           return '4'; break;
     case 1:           return '5'; break;
     case 2:           return '6'; break;
     case 3:           return 'B'; break;
    }
  }
   if (i==2)
  { switch (j)
    {case 0:           return '7'; break;
     case 1:           return '8'; break;
     case 2:           return '9'; break;
     case 3:           return 'C'; break;
    }
  }
   if (i==3)
  { switch (j)
    {case 0:           return '*'; break;
     case 1:           return '0'; break;
     case 2:           return '#'; break;
     case 3:           return 'D'; break;
    }
  }
}
//////////////////////////////////////////////////////////////////END KEYPAD FUNCTIONS//////////////////////////////////////////////////////////////
