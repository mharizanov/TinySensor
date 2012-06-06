//--------------------------------------------------------------------------------------
// TempTX-tiny ATtiny84 Based Wireless Temperature Sensor Node
// By Nathan Chantrell http://nathan.chantrell.net
//
// GNU GPL V3
//--------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define myNodeID 2      // RF12 node ID in the range 1-30
#define network 210      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module

#define tempPin A0       // TMP36 Vout connected to A0 (ATtiny pin 13)
#define tempPower 0      // TMP36 Power pin is connected on pin D9 (ATtiny pin 12)

int tempReading;         // Analogue reading from the sensor

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  int temp;	// Temperature reading
  	  int supplyV;	// Supply voltage
  	  int temp2;	// Temperature reading  
 } Payload;

 Payload temptx;

//########################################################################################################################

void setup() {
  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  // Adjust low battery voltage to 2.2V UNTESTED!!!!!!!!!!!!!!!!!!!!!
  rf12_control(0xC040);
  rf12_sleep(0);                          // Put the RFM12 to sleep
  analogReference(INTERNAL);  // Set the aref to the internal 1.1V reference
  pinMode(tempPower, OUTPUT); // set power pin for TMP36 to output
}

void loop() {
  
  digitalWrite(tempPower, HIGH); // turn TMP36 sensor on

  delay(10); // Allow 10ms for the sensor to be ready
 
  analogRead(tempPin); // throw away the first reading
  
  for(int i = 0; i < 10 ; i++) // take 10 more readings
  {
   tempReading += analogRead(tempPin); // accumulate readings
  }
  tempReading = tempReading / 10 ; // calculate the average

  digitalWrite(tempPower, LOW); // turn TMP36 sensor off

//  double voltage = tempReading * (1100/1024); // Convert to mV (assume internal reference is accurate)
  
  double voltage = tempReading * 0.942382812; // Calibrated conversion to mV

  double temperatureC = (voltage - 500) / 10; // Convert to temperature in degrees C. 

  temptx.temp = temperatureC * 100; // Convert temperature to an integer, reversed at receiving end
  temptx.temp2 = 1 * 100; // Convert temperature to an integer, reversed at receiving end

  temptx.supplyV = readVcc(); // Get supply voltage

  rfwrite(); // Send data via RF 

  for(int j = 0; j < 5; j++) {    // Sleep for 5 minutes
    Sleepy::loseSomeTime(60000); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)
  }
}

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//--------------------------------------------------------------------------------------------------
 static void rfwrite(){
   rf12_sleep(-1);     //wake up RF module
   while (!rf12_canSend())
   rf12_recvDone();
   rf12_sendStart(0, &temptx, sizeof temptx); 
   rf12_sendWait(2);    //wait for RF to finish sending while in standby mode
   rf12_sleep(0);    //put RF module to sleep
}

//--------------------------------------------------------------------------------------------------
// Read current supply voltage
//--------------------------------------------------------------------------------------------------

 long readVcc() {
   long result;
   // Read 1.1V reference against Vcc
   ADMUX = _BV(MUX5) | _BV(MUX0);
   delay(2); // Wait for Vref to settle
   ADCSRA |= _BV(ADSC); // Convert
   while (bit_is_set(ADCSRA,ADSC));
   result = ADCL;
   result |= ADCH<<8;
   result = 1126400L / result; // Back-calculate Vcc in mV
   return result;
}

