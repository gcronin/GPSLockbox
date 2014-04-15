// Code credit to GekoCH:  http://forum.arduino.cc/index.php?topic=7490.0
// Modified to parse GGA strings

#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <Keypad.h>

SoftwareSerial mySerial(50, 51, false); //Green in 50 (rx), Yellow in 51 (tx)
LiquidCrystal lcd(7, 8, 3, 4, 5, 6, 9, 10, 11, 12);

const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns
// Define the Keymap
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'} };
  
  // Connect keypad ROW0, ROW1, ROW2 and ROW3 to these Arduino pins.
byte rowPins[ROWS] = { 54, 55, 56, 57 };
// Connect keypad COL0, COL1 and COL2 to these Arduino pins.
byte colPins[COLS] = { 58, 59, 60, 61 }; 

// Create the Keypad
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

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

void setup()  
{
  if(SerialON) Serial.begin(BAUDRATE);
  if(LCD_ON)  lcd.begin(8, 2);
  mySerial.begin(BAUDRATE);
  ampm[1] = 'M';
  lcd.setCursor(0,0);
  lcd.print("PRESS");
  lcd.setCursor(6,0);
  lcd.print("A");
  lcd.setCursor(0,1);
  lcd.print("4");
  lcd.setCursor(2,1);
  lcd.print("SETUP");
}

void loop()
{
  char key = kpd.getKey();
  if(key == 'B') {displayTime = false; } // Display GPS mode
  else if(key == 'C') {displayTime = true; } // Display Time mode
  else if(key == 'A')  // Check for setup mode
  {   
    lcd.clear();
    lcd.print("LATITUDE");
    lcd.setCursor(0,1);
    lcd.print("????????");
    SETUPLATITUDE = true;
    for(int i = 0; i < 8; i++) { input[i] = '0'; }  // initialize input as zero
    lcdcolumnindexrow1 = 0;
  }
  else if(SETUPLATITUDE)  // user input of latitude
  {
    if(key != NO_KEY) {// received a keystroke
        if(key == 'D') {
          SETUPLATITUDE = false;
          SETUPLONGITUDE = true; 
          targetLatitude = atol(input);
          Serial.println(targetLatitude);
          lcd.clear();
          lcd.print("LONGITUD");
          lcd.setCursor(0,1);
          lcd.print("????????");
          for(int i = 0; i < 8; i++) { input[i] = '0'; }  // initialize input as zero
          lcdcolumnindexrow1 = 0;
         }
        else if(key != 'B' && key!= 'C' && key != '#' && key != '*' && key != 'A') {        
          for(int i =0; i < 7; i++) {input[i] = input[i+1]; }  //shift all the values left
          input[7] = key;
          lcd.setCursor(lcdcolumnindexrow1,1);  //print on left of LCD moving right
          lcd.print(input[7]);
          ++lcdcolumnindexrow1;
        }
    }
  }
  else if(SETUPLONGITUDE)  // user input of longitude
  {
    if(key != NO_KEY) {// received a keystroke
        if(key == 'D') {
          SETUPLONGITUDE = false;
          SETUP = true; 
          targetLongitude = atol(input);
          Serial.println(targetLongitude);
          }
        else if(key != 'B' && key!= 'C' && key != '#' && key != '*' && key != 'A') {        
          for(int i =0; i < 7; i++) {input[i] = input[i+1]; }  //shift all the values left
          input[7] = key;
          lcd.setCursor(lcdcolumnindexrow1,1);  //print on left of LCD moving right
          lcd.print(input[7]);
          ++lcdcolumnindexrow1;
        }
    }
  }
  else if(!SETUP) { }  // infinite loop for the very beginner so it keeps displaying setup prompt
  else {
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


