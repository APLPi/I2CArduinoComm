/*
 * I2C to Arduino Mino Pro Communication Layer
 * for Arduino mini pro pinouts see http://arduino.cc/en/uploads/Main/Arduino-Pro-Mini-schematic.pdf
 * for motor control pin names see https://content.solarbotics.com/products/datasheets/solarbotics_l298_compact_motor_driver_kit.pdf
 *
 */

#include <Wire.h>
#include <Servo.h>

const boolean DEBUG = true;
boolean newDataAvailable = false; // Is there a response ready?
boolean RESULT; // Did the last request produce a "result"?

const int I2C_ADDRESS =  4;
const int NumPins   =   20; // Arduino Mini
const int MaxBuf    =   32;
const int MaxServos =    1;
const int MaxLog    = 1024;

Servo Servos[MaxServos]; // Up to 6 servos
int UsedServos = 0;

byte INPUTPINS[NumPins]; // List of input pins
int NumInputs = 0; // Count the number of input pins


byte OUTDATA[MaxBuf];
byte RESULTDATA[MaxBuf]; 
char LOGTXT[MaxLog]; int logend = 0; int logptr = 0; byte LOGBUF[MaxBuf]; // Debug log
char PINTYPE[NumPins]; //Pin Type (blank, or one of ADSadp)
byte PININFO[NumPins]; //Extra control data (used for pulse pairs)

void logstr(char text[32])
{ logend = logend + sprintf(&LOGTXT[logend],"%s", text); }
void logint(int i)
{ logend = logend + sprintf(&LOGTXT[logend],"%d", i); }
void logchar(char c)
{ LOGTXT[logend] = c; logend++; }

void resetall(void)
{ 
  for (int i=0; i < NumPins; i++) 
  {  PINTYPE[i] = ' ';
     PININFO[i] = 0;
  }
  NumInputs = 0;
  newDataAvailable = false;
}

void registerinput(int pin)
{
  pinMode(pin, INPUT);
  INPUTPINS[NumInputs]=pin;
  NumInputs++;
}
void loop() { 
    // Collect input data and store it in OUTDATA 
    int outvalue;
    byte bufptr=1;     
    for (int i=0 ; i < NumInputs; i++) // Loop through all input pins
    {
      int pin = INPUTPINS[i];
      switch (PINTYPE[pin])
      {
      case 'a': // Analog Input
        { 
          outvalue = analogRead(pin); // AnalogInputPins[pin]); 
          break;
        }
      case 'd': // Digital Input
        { 
          outvalue = digitalRead(pin); 
          break;
        }
      case 'p': // Pulse input
        { 
          byte trigger =PININFO[pin];
          if (trigger > 0) // Trigger Pulse =>Pull pin high for 10us
          {
            digitalWrite(trigger, LOW); 
            delayMicroseconds(2);
            digitalWrite(trigger, HIGH); 
            delayMicroseconds(10);
            digitalWrite(trigger, LOW);
          }
          long ov = pulseIn(pin, HIGH); // Timeout 10ms
          outvalue = (int) ov;
          break;
        }
      }    
      byte * bytePointer = (byte*)&outvalue; 
      OUTDATA[bufptr] = bytePointer[1]; bufptr++; // Store MSB
      OUTDATA[bufptr] = bytePointer[0]; bufptr++; // Store LSB
      // int len = sprintf(&OUTDATA[bufptr]," %d", outvalue);
      // bufptr = bufptr + len;
    }
    OUTDATA[0] = bufptr; // First byte is length
}

void ReceiveEvent(int numBytes)
{
  RESULT = false;
  logptr = 0; logend = 0; // Reset LOG
  byte bufptr = 1;  // Pointer into OUTDATA while preparing output
  int ptr = 0;      // Pointer into input data

  byte n = Wire.read(); // Command length

  if (n == numBytes-1)
  {
    char function = Wire.read(); 
    ptr++;
    switch (function)
    {
    case 'R': // Reset
      {        
        resetall(); // for (int i=0; i < NumPins; i++) PINTYPE[i] = ' ';
        if (DEBUG == true) {logstr("R: "); logint(NumPins); logstr(" pins reset.\n");} 
        break;
      } 

    case 'I': // Identify
      {
        // Builds OUTDATA buffer for next I2C READ event
        if (DEBUG == true) logstr("I: ArdCom version 0.2:\n");
        RESULTDATA[1]='0'; RESULTDATA[2]='2'; // Ardcom Major & Minor Version numbers
        bufptr=3; 
        int pins=0;
        
        for (int pin=0 ; pin < NumPins; pin++) // Document each pin
        {
          char pintype = PINTYPE[pin];
          if (pintype != ' ') // && bufptr < 32) // Defined pins - but don't overwrite buffer
          {
            RESULTDATA[bufptr] = pin; bufptr++;
            RESULTDATA[bufptr] = pintype; bufptr++;
            pins++;
           if (DEBUG == true) {logint(pin); logstr("="); logchar(pintype); logstr(",");}
          }
        }
        RESULTDATA[0]=bufptr-1;
        RESULT = true; // The function put data in OUTDATA
        if (DEBUG == true) logstr("\n");
        break;
      }

    case 'S':  // Setup
      { 
        if (DEBUG == true) logstr("S:\n");
        int pins = 0;
        while (ptr < n) // Triplets of pin,type,info
        {
          byte pin = Wire.read(); 
          char pintype = Wire.read(); 
          PININFO[pin] = Wire.read(); 
          PINTYPE[pin] = pintype;
          if (DEBUG == true) {logstr("Pin "); logint(pin); logstr(" = "); logchar(pintype);}
          ptr=ptr+3; 
          switch (pintype)
          {
          case 'S': // Servo
            {
              Servos[UsedServos].attach(pin);
              PININFO[pin]=UsedServos;
              UsedServos++;
              pinMode(pin, OUTPUT);
              if (DEBUG == true) {logstr (" - servo #"); logint(UsedServos); logstr(" attached.");}
              break;
            }
          case 'p': // Pulse Input
            { 
              registerinput(pin);
              pinMode(PININFO[pin], OUTPUT); // Trigger Pin is output
              if (DEBUG == true) {logstr (" - trigger pin is #"); logint(PININFO[pin]); logstr(".");}
              break;
            }
          case 'd':
            { 
              registerinput(pin);
              break; 
            }
          case 'a':
            { 
              registerinput(pin);
              break; 
            }
          case 'A':
            { 
              pinMode(pin, OUTPUT); 
              break; 
            }
          case 'D':
            { 
              pinMode(pin, OUTPUT); 
              break; 
            }
          }
           if (DEBUG == true) logstr("\n.");
           pins++;
        } // While (each pin)
        if (DEBUG == true) 
          {logint(pins); logstr(" pins defined, of which ");
           logint(NumInputs); logstr(" inputs.\n");}        
        break;
      }
    case 'W':  // Write      
      { 
        while (ptr < numBytes)
        { //Pairs of pin, value
          byte pin = Wire.read(); 
          byte value = Wire.read(); 
          ptr=ptr+2;
          switch(PINTYPE[pin])
          {
          case 'A':
            { 
              analogWrite(pin,value); 
              break; 
            }
          case 'D':
            { 
              digitalWrite(pin,value?HIGH:LOW); 
              break; 
            }
          case 'S':
            { 
              Servos[PININFO[pin]].write(value); 
              break; 
            }        
          }     
        } 
        break;
      }
     default:
     {logstr("Invalid command received: "); logchar(function); logstr("\n");}
    }
  }
  else
  {
    do {
      Wire.read(); ptr++; // Error: Numbytes does not match - just throw it away
    } while (ptr < numBytes);  
     logstr("Ill formatted command.\n");
}
}

void onI2CRequest()
{ 
  byte sent;

  // First, see if there is diagnostic output to be sent 
  if (logend > logptr)
  {
   LOGBUF[1]=254; // Signal to recipient that this is diagnostic output (MSB of data cannot be 254)
   int i = logend - logptr; if (i > 30) i = 30;  // How much to send? We need 2 bytes for envelope.     
   memcpy(&LOGBUF[2], &LOGTXT[logptr], i);
   logptr = logptr + i;
   LOGBUF[0] = i+1;
   sent = Wire.write(LOGBUF,i+2);
   return;
  }

  // Next: Is there a pending result from the last command?
  if (RESULT == true) 
  {
    sent = Wire.write(RESULTDATA,1+RESULTDATA[0]); 
    RESULT = false;
  }
  else // Return the latest sample data
  {
    sent = Wire.write(OUTDATA,1+OUTDATA[0]); 
    // OUTDATA[OUTDATA[0]]=0;
    // sent = Wire.write(OUTDATA); 
  }
}

void setup()
{
  resetall(); 
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onI2CRequest);
  Wire.onReceive(ReceiveEvent);
}

