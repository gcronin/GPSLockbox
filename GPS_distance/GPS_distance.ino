// Code credit to GekoCH:  http://forum.arduino.cc/index.php?topic=7490.0
// Modified to parse GGA strings

#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <Wire.h> 

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
boolean displayTime = false;
 
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



/////////////////////////////////////////////////LCD SETUP//////////////////////////////////////////////////////////////
LiquidCrystal lcd(A5, A4, A0, A3, A2, A1);
boolean displayTime = false;
const int gpsCoordinateInputBufferSize = 10;
char input[gpsCoordinateInputBufferSize]; //buffer for input characters
int lcdcolumnindexrow1 = 0; // used to indicate location of input key


///////////////////////////////////////////////VARIABLE SETUP//////////////////////////////////////////////////////////
boolean SETUP = false;
boolean SETUPLATITUDE = false;
boolean SETUPLONGITUDE = false; 




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



void setup()  
{
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

}

void loop()
{
  unsigned char key;                                 
  key=readdata();
  while (readdata()==key && key!= 'E')  {}  //keypad debounce
  if(key == 'B') {}//displayTime = false; } // Display GPS mode
  else if(key == 'C') {}//displayTime = true; } // Display Time mode
  else if(key == 'A')  // Check for setup mode
  {   
    lcd.clear();
    lcd.print("Enter Latitude");
    lcd.setCursor(0,1);
    lcd.print("ddmmmmmm");
    SETUPLATITUDE = true;
    for(int i = 0; i < gpsCoordinateInputBufferSize; i++) { input[i] = '0'; }  // initialize input as zero
    lcdcolumnindexrow1 = 0;
  }
  else if(SETUPLATITUDE)  // user input of latitude
  {
    if(key != 'E') {// received a keystroke
        if(key == 'D') {
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
         }
        else if(key != 'B' && key!= 'C' && key != '#' && key != '*' && key != 'A') {        
          for(int i =0; i < (gpsCoordinateInputBufferSize-1); i++) {input[i] = input[i+1]; }  //shift all the values left
          input[(gpsCoordinateInputBufferSize-1)] = key;
          lcd.setCursor(lcdcolumnindexrow1,1);  //print on left of LCD moving right
          lcd.print(input[(gpsCoordinateInputBufferSize-1)]);
          ++lcdcolumnindexrow1;
        }
    }
  }
  else if(SETUPLONGITUDE)  // user input of longitude
  {
    if(key != 'E') {// received a keystroke
        if(key == 'D') {
          SETUPLONGITUDE = false;
          SETUP = true; 
          targetLongitude = atol(input);
          Serial.println(targetLongitude);
          }
        else if(key != 'B' && key!= 'C' && key != '#' && key != '*' && key != 'A') {        
          for(int i =0; i < (gpsCoordinateInputBufferSize-1); i++) {input[i] = input[i+1]; }  //shift all the values left
          input[(gpsCoordinateInputBufferSize-1)] = key;
          lcd.setCursor(lcdcolumnindexrow1,1);  //print on left of LCD moving right
          lcd.print(input[(gpsCoordinateInputBufferSize-1)]);
          ++lcdcolumnindexrow1;
        }
    }
  }
  else if(!SETUP) { }  // infinite loop for the very beginner so it keeps displaying setup prompt
  else {
    
  float latitudeDecimal = latitude;
  float longitudeDecimal = longitude;
  latitudeDecimal /= 1000000;
  longitudeDecimal /=1000000;
  Serial.println(latitudeDecimal, 6);
  Serial.println(longitudeDecimal, 6);
  delay(3000);
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
