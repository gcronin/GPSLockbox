#include <Wire.h> 
#include <LiquidCrystal.h>


/////////////////////////////////////////////////LCD SETUP//////////////////////////////////////////////////////////////
LiquidCrystal lcd(A5, A4, A0, A3, A2, A1);
boolean displayTime = false;
const int gpsCoordinateInputBufferSize = 9;
char input[gpsCoordinateInputBufferSize]; //buffer for input characters
int lcdcolumnindexrow1 = 0; // used to indicate location of input key



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



void setup()  
{
  //KEYPAD SETUP
  Wire.begin(PCA9555); // join i2c bus (address optional for master) tried to get working
  write_io (CONFIG_P0, B00001111); //define port 0.7-0.4 as output, 0.3-0.0 as input
  //Serial.begin(38400);
    //LCD SETUP
  lcd.begin(16, 2);
}

void loop() {
  lcd.clear();
   unsigned char key;   
  key=readdata();
  while (readdata()==key && key!= 'E')  {}  //keypad debounce
  if(key!= 'E') lcd.print(char(key));
delay(50);
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
