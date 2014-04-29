#include <Servo.h>

Servo myservo;
int outPin = 15;  //Out Magnetic Sensor attached here
int inPin = 14;   //In Magnetic Sensor attached here
int outPulse = 1100;
int inPulse = 1900;
int stopPulse = 1465;
int servonumber = stopPulse;  //stopped
long timestamp = 0;
int OutLimit = 0; 
int InLimit = 0; 
boolean DirectionOut = false;
boolean DirectionIn = false;

void setup() {
  Serial.begin(38400);
  myservo.attach(16);
  pinMode(outPin, INPUT);  //Out Limit Sensor
  pinMode(inPin, INPUT);  //In Limit Sensor
}

void loop() {

  
  if(millis() - timestamp > 1000) { 
    Serial.print("Out:  ");
    Serial.print(OutLimit);
    Serial.print("    In:   ");
    Serial.print(InLimit);
    Serial.print("    Pulse:   ");
    Serial.println(servonumber);
    Serial.println("> IN   < OUT   S  STOP");
    timestamp = millis();
  }   

  OutLimit = digitalRead(outPin);
  InLimit = digitalRead(inPin);
  if(OutLimit == 0 && DirectionIn == false) servonumber = 1465;
  else if(InLimit == 0 && DirectionOut == false) servonumber = 1465;
  else if(InLimit == 0 && DirectionOut == true) servonumber = outPulse;
  else if(OutLimit == 0 && DirectionIn == true) servonumber = inPulse;
  else if(DirectionIn) servonumber = inPulse;
  else if(DirectionOut) servonumber = outPulse;
  else servonumber = stopPulse;
  myservo.writeMicroseconds(servonumber);
}

  
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 

if(inChar == 62) { //  > = IN
  DirectionOut = false;
  DirectionIn = true; }
else if(inChar == 60) {  //  < = OUT
  DirectionOut = true;
  DirectionIn = false; }
else { // S = STOP
  DirectionOut = false;
  DirectionIn = false; }  
}
}
