// Code credit to GekoCH:  http://forum.arduino.cc/index.php?topic=7490.0
// Modified to parse GGA strings

#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

SoftwareSerial mySerial(9, 8, false); //Green in 9 (rx), Yellow in 8 (tx)
LiquidCrystal lcd(7, 6, 5, 4, A5, A4, A3, A2, A1, A0);

int RGBLed[] = {10, 13, 11};

byte RED[] = {255, 0, 0};
byte ORANGE[] = {83, 4, 0};
byte YELLOW[] = {255, 255, 0};
byte GREEN[] = {0, 255, 0};
byte BLUE[] = {0, 0, 255};
byte INDIGO[] = {4, 0, 19};
byte VIOLET[] = {23, 0, 22};
byte CYAN[] = {0, 255, 255};
byte MAGENTA[] = {255, 0, 255};
byte WHITE[] = {255, 255, 255};
byte BLACK[] = {0, 0, 0};
byte PINK[] = {158, 4, 79};

#define BAUDRATE 38400
#define SerialON 1
#define LCD_ON  1
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
boolean SETUP = false;
boolean SETUPLATITUDE = false;
boolean SETUPLONGITUDE = false; 
char input[9]; //buffer for input characters
int lcdcolumnindexrow1 = 0; // used to indicate location of input key

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


void setColor(int* led, byte* color){
  for(int i = 0; i < 3; i++){
    //iterate through each of the three pins (red green blue)
    analogWrite(led[i], 255 - color[i]);
  //set the analog output value of each pin to the input value (ie led[0] (red pin) to 255- color[0] (red input color)
  //we use 255 - the value because our RGB LED is common anode, this means a color is full on when we output analogWrite(pin, 0)
  //and off when we output analogWrite(pin, 255).
  }
}

/* A version of setColor that takes a predefined color
(neccesary to allow const int pre-defined colors */
void setColor(int* led, const byte* color){
  byte tempByte[] = {color[0], color[1], color[2]};
  setColor(led, tempByte);
}

/* Fades the LED from a start color to an end color at fadeSpeed
led - (int array of three values defining the LEDs pins (led[0] = redPin, led[1] = greenPin, led[2] = bluePin))
startColor - (byte array of three values defing the start RGB color (startColor[0] = start Red value, startColor[1] = start Green value, startColor[2] = start Red value
endColor - (byte array of three values defing the finished RGB color (endColor[0] = end Red value, endColor[1] = end Green value, endColor[2] = end Red value
fadeSpeed - this is the delay in milliseconds between steps, defines the speed of the fade*/
void fadeToColor(int* led, byte* startColor, byte* endColor, int fadeSpeed){
  int changeRed = endColor[0] - startColor[0];
  //the difference in the two colors for the red channel
  int changeGreen = endColor[1] - startColor[1];
  //the difference in the two colors for the green channel
  int changeBlue = endColor[2] - startColor[2];
  //the difference in the two colors for the blue channel
  int steps = max(abs(changeRed),max(abs(changeGreen), abs(changeBlue)));
  //make the number of change steps the maximum channel change
  for(int i = 0 ; i < steps; i++){
    //iterate for the channel with the maximum change
    byte newRed = startColor[0] + (i * changeRed / steps);
    //the newRed intensity dependant on the start intensity and the change determined above
    byte newGreen = startColor[1] + (i * changeGreen / steps);
    //the newGreen intensity
    byte newBlue = startColor[2] + (i * changeBlue / steps);
    //the newBlue intensity
    byte newColor[] = {newRed, newGreen, newBlue};
    //Define an RGB color array for the new color
    setColor(led, newColor);
    //Set the LED to the calculated value
    delay(fadeSpeed);
    //Delay fadeSpeed milliseconds before going on to the next color
  }
  setColor(led, endColor);
  //The LED should be at the endColor but set to endColor to avoid rounding errors
}

  void setup()  
{
  if(SerialON) Serial.begin(BAUDRATE);
  if(LCD_ON)  lcd.begin(8, 2);
  mySerial.begin(BAUDRATE);
  ampm[1] = 'M';
  for(int i=0; i<3; i++) pinMode(RGBLed[i], OUTPUT);
}
  
 void loop()
{ 
  //Serial.println("Working");
  //fadeToColor(RGBLed, RED, GREEN, 1);  CAN'T DO THIS... IT MISSES THE SENT DATA WHILE WAITING!!!
  //fadeToColor(RGBLed, GREEN, BLUE, 1);
  //fadeToColor(RGBLed, BLUE, RED, 1);
  
  uint32_t tmp;
  readline();
  if (strncmp(buffer, "$GPGGA",6) == 0)  // Parse the string when it begins with $GPGGA
   { 
    Serial.println(buffer);
     // hhmmss time data
    parseptr = buffer+7;
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
    
    if(SerialON) {
      Serial.print("\nTime: ");
      Serial.print(hour, DEC); Serial.print(':');
      if(minute < 10) Serial.print("0");
      Serial.print(minute, DEC); Serial.print('.');
      Serial.print(second, DEC); Serial.print(' ');
      Serial.println(ampm);
      Serial.print("Lat: "); 
      if (latdir == 'N')
         Serial.print('+');
      else if (latdir == 'S')
         Serial.print('-');
      Serial.println(longitude);
      Serial.print(latitude/1000000, DEC); Serial.print("\260"); Serial.print(' ');
      Serial.print((latitude/10000)%100, DEC); Serial.print('\''); Serial.print(' ');
      Serial.print((latitude%10000)*6/1000, DEC); Serial.print('.');
      Serial.print(((latitude%10000)*6/10)%100, DEC); Serial.println('\"');
      Serial.print("Long: "); 
      if (latdir == 'N')
         Serial.print('+');
      else if (latdir == 'S')
         Serial.print('-');
      Serial.print(longitude/1000000, DEC); Serial.print("\260"); Serial.print(' ');
      Serial.print((longitude/10000)%100, DEC); Serial.print('\''); Serial.print(' ');
      Serial.print((longitude%10000)*6/1000, DEC); Serial.print('.');
      Serial.print(((longitude%10000)*6/10)%100, DEC); Serial.println('\"');
      Serial.print("Altitude: "); 
      Serial.print(altitude/100, DEC); Serial.print('.');
      Serial.print(altitude%100, DEC); Serial.println(" m");
    }
    if(LCD_ON && !displayTime) {
      lcd.setCursor(0,0);
      lcd.print(latitude);
      lcd.setCursor(0,1);
      lcd.print(longitude);
    }
   else if(LCD_ON && displayTime) {
      String totalTime = "";
      totalTime = totalTime + hour;
      totalTime = totalTime + ":";
      totalTime = totalTime + minute;
      totalTime = totalTime + ".";
      totalTime = totalTime + second;  
      lcd.setCursor(0,0);
      lcd.print("TIME:   ");
      lcd.setCursor(0,1);
      lcd.print(totalTime);
    }
    
 }}
 
 
 
 
 
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


