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

unsigned int getAvrTemp(void)
{
  /* Tiny84
  T = k * [(ADCH << 8) | ADCL] + TOS
  */
    unsigned char high, low;
    ADMUX = (INTERNAL << 6) | B00100010; //Selecting the ADC8 channel by writing the MUX5:0 bits in ADMUX register to â€œ100010â€ enables the temperature sensor.
  CLKPR = bit(CLKPCE); 
  CLKPR = 4; // div 16 
  delayMicroseconds(62); 
  CLKPR = bit(CLKPCE); 
  CLKPR = 0; 
    ADCSRA |= bit(ADSC); // Start a conversion for Temperature
    while (ADCSRA & bit(ADSC));
    low = ADCL; // Must be read first as it locks
    high = ADCH; // read second and releases the lock
   return (high << 8) | low;
 }
 
 
void setup() {
  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  // Adjust low battery voltage to 2.2V UNTESTED!!!!!!!!!!!!!!!!!!!!!
  rf12_control(0xC040);
  rf12_sleep(0);                          // Put the RFM12 to sleep
  analogReference(INTERNAL);  // Set the aref to the internal 1.1V reference

}

void loop() {
  

  int temperatureC = getAvrTemp()- 97; // Correction Factor per individual CPU   
  temptx.temp = temperatureC * 100; // Convert temperature to an integer, reversed at receiving end
  temptx.temp2 = 1 * 100; // Convert temperature to an integer, reversed at receiving end

  temptx.supplyV = readVcc(); // Get supply voltage

  rfwrite(); // Send data via RF 

  for(int j = 0; j < 5; j++) {    // Sleep for 5 minutes
    Sleepy::loseSomeTime(1000); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)
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

