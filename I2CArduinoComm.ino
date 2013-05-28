/*
 * I2C to Arduino Mino Pro Communication Layer
 * Converted from the quick2wire trackbot by Liam Flanagan
 * For more information see https://github.com/quick2wire/trackbot
 *
 * for Arduino mini pro pinouts see http://arduino.cc/en/uploads/Main/Arduino-Pro-Mini-schematic.pdf
 * for motor control pin names see https://content.solarbotics.com/products/datasheets/solarbotics_l298_compact_motor_driver_kit.pdf
 *
 */

#include <Wire.h>

// Lights
const int onboardLED = 13;
const int I2C_ADDRESS = 4;
// Sensors
int ReadPin = A3;
int readType = 0; // 0 for analog 1 for digital

void loop() { }  //  Required for Arduino main function

/*
 * Light helper functions
 */
void flashOnboardLED()
{
	digitalWrite(onboardLED, HIGH);
	delay(800);
	digitalWrite(onboardLED, LOW);
}
void flashForStartup()
{
	pinMode(onboardLED, OUTPUT);
	flashOnboardLED();
	delay(200);
	flashOnboardLED();
}


/*
 * Event callback function
 * This executes whenever data is received from master.
 * It is registered as an event in the setup function.
 */
const int PACKET_SIZE = 3;	// bytes
void ReceiveEvent(int numBytes)
{
  Serial.println("Receive event");
	flashOnboardLED();	// Let user know we have triggered an event
	
	if (0 == (numBytes % PACKET_SIZE))
	{
		int numPackets = numBytes / PACKET_SIZE;
		
		for (int i=0; i < numPackets; i++)
		{
			char	function = Wire.read();
			byte	pin      = Wire.read(),
					value    = Wire.read();
        
			switch (function)
			{
				case 'A':
				{
					pinMode(pin, OUTPUT);
					analogWrite(pin,value);
					break;
				}
				case 'a':
				{
					pinMode(pin, INPUT);
					ReadPin=pin;
					readType = 0;
					break;
				}
				case 'D':
				{            
					pinMode(pin, OUTPUT);
					digitalWrite(pin,value?HIGH:LOW);
					break;
				}
				case 'd':
				{
					pinMode(pin, INPUT);
					ReadPin=pin;
					readType = 1;
					break;
				}
			}
		}
	}
	else
	{
		// Error
		// Packet(s) not of a recognised size
		for (int i=0; i < numBytes; i++)
		{
			// Clear buffer on the master
			Wire.read();
		}
	}
}

void onI2CRequest()
{
  Serial.println("Send event");
  char buf[16];
  int PinVal;
  int len;
	if (0 == readType ) {
	  PinVal = analogRead(ReadPin);
	  len = sprintf(&buf[1],"a%d:%d;",ReadPin, PinVal);
	} else {
	  PinVal = digitalRead(ReadPin);
	  len = sprintf(&buf[1],"d%d:%d;",ReadPin, PinVal);
	}
  buf[0] = len;
  Serial.println(buf);
  Wire.write(buf);
}

void setup()
{
	Serial.begin(9600);

	Wire.begin(I2C_ADDRESS);
	Wire.onRequest(onI2CRequest);
	Wire.onReceive(ReceiveEvent);

	
	flashForStartup();
}
