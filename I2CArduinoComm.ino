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

// Right (from front of robot) side motor pins 
const int L4  = 3;    // yellow, d3
const int L3  = 4;    // orange, d4
// Left side motor pins
const int L2  = 7;    // yellow, d7
const int L1  = 8;    // orange, d8
// Lights
const int onboardLED = 13;

void loop(){}  //  Required for Arduino main function

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
					analogWrite(pin,value);
					break;
				}
				case 'a':
				{
					break;
				}
				case 'D':
				{            
					digitalWrite(pin,value?HIGH:LOW);
					break;
				}
				case 'd':
				{
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

const int I2C_ADDRESS = 4;
void setup()
{
	pinMode(L1, OUTPUT);
	pinMode(L2, OUTPUT);
	pinMode(L3, OUTPUT);
	pinMode(L4, OUTPUT);
	
	Wire.begin(I2C_ADDRESS);
	Wire.onReceive(ReceiveEvent);
	
	flashForStartup();
}
