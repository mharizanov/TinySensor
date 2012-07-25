//--------------------------------------------------------------------------------------
// TempTX-tiny ATtiny84 Based Wireless Temperature Sensor Node
// By Nathan Chantrell http://nathan.chantrell.net
//
// GNU GPL V3
//--------------------------------------------------------------------------------------

#ifdef F_CPU  
#undef F_CPU
#endif  
#define F_CPU 4000000 // 4 MHz  
#define BODS 7                   //BOD Sleep bit in MCUCR
#define BODSE 2                  //BOD Sleep enable bit in MCUCR

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"

ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define myNodeID 2      // RF12 node ID in the range 1-30
#define network 210      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module

#define tempPin A0       // TMP36 Vout connected to A0 (ATtiny pin 13)
#define tempPower 0      // TMP36 Power pin is connected on pin D9 (ATtiny pin 12)

#define LDR 2             //PA2

int tempReading;         // Analogue reading from the sensor
unsigned int ldrReading;         // Analogue reading from the sensor

//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  int temp;	// Temperature reading
  	  int supplyV;	// Supply voltage
  	  unsigned int temp2;	// LDR reading  
 } Payload;

 Payload temptx;

//########################################################################################################################

static void setPrescaler (uint8_t mode) {
    cli();
    CLKPR = bit(CLKPCE);
    CLKPR = mode;
    sei();
}

void setup() {
  MCUCR |= _BV(BODS) | _BV(BODSE);          //turn off the brown-out detector  
  setPrescaler(1);    // div 1, i.e. 4 MHz
  //setPrescaler(15); // div 8, i.e. 1 MHz div 256 (15) = 32kHz
  //setPrescaler(3);  // div 8, i.e. 1 MHz
  
  analogReference(INTERNAL);  // Set the aref to the internal 1.1V reference
  pinMode(tempPower, OUTPUT); // set power pin for TMP36 to output
  pinMode(tempPin, INPUT); // set power pin for TMP36 to input before sleeping, saves power
  pinMode(LDR, INPUT);
  
  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  // Adjust low battery voltage to 2.2V 
  rf12_control(0xC040);
  rf12_sleep(0);                          // Put the RFM12 to sleep
  
  PRR = bit(PRTIM1); // only keep timer 0 going
}

void loop() {
  
  setPrescaler(3);  // div 8, i.e. 1 MHz
  bitClear(PRR, PRADC); // power up the ADC
  ADCSRA |= bit(ADEN); // enable the ADC  
  digitalWrite(tempPower, HIGH); // turn TMP36 sensor on

  Sleepy::loseSomeTime(16); // Allow 10ms for the sensor to be ready
  analogRead(tempPin); // throw away the first reading
  analogRead(LDR); // throw away the first reading
  
  for(int i = 0; i < 10 ; i++) // take 10 more readings
  {
   tempReading += analogRead(tempPin); // accumulate readings
   ldrReading += analogRead(LDR); // accumulate readings
  }
  tempReading = tempReading / 10 ; // calculate the average
  ldrReading = ldrReading / 10 ; // calculate the average
  
  digitalWrite(tempPower, LOW); // turn TMP36 sensor off
  pinMode(tempPower, INPUT); // set power pin for TMP36 to input before sleeping, saves power
  digitalWrite(tempPower, HIGH); // enable pull-ups
  
  temptx.supplyV = readVcc(); // Get supply voltage
  
  ADCSRA &= ~ bit(ADEN); // disable the ADC
  bitSet(PRR, PRADC); // power down the ADC
  

//  double voltage = tempReading * (1100/1024); // Convert to mV (assume internal reference is accurate)
  
  double voltage = tempReading * 0.942382812; // Calibrated conversion to mV
  double temperatureC = (voltage - 500) / 10; // Convert to temperature in degrees C. 
  temptx.temp = temperatureC * 100; // Convert temperature to an integer, reversed at receiving end
  temptx.temp2 = ldrReading ; // Convert temperature to an integer, reversed at receiving end
  rfwrite(); // Send data via RF 

  for(byte j = 0; j < 15; j++) {    // Sleep for 5 minutes
    Sleepy::loseSomeTime(60000); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)
  }

  pinMode(tempPower, OUTPUT); // set power pin for TMP36 to output  
}

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//--------------------------------------------------------------------------------------------------
 static void rfwrite(){
  setPrescaler(1);    // div 1, i.e. 4 MHz
   bitClear(PRR, PRUSI); // enable USI h/w
   rf12_sleep(-1);     //wake up RF module
   while (!rf12_canSend())
     rf12_recvDone();
   rf12_sendStart(0, &temptx, sizeof temptx); 
   rf12_sendWait(2);    //wait for RF to finish sending while in standby mode
   rf12_sleep(0);    //put RF module to sleep
   bitSet(PRR, PRUSI); // disable USI h/w
   setPrescaler(3); // div 8, i.e. 1 MHz
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

