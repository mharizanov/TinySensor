//--------------------------------------------------------------------------------------
// TempTX-tiny ATtiny84 Based Wireless Temperature Sensor Node
// By Nathan Chantrell http://nathan.chantrell.net
//
// GNU GPL V3
//--------------------------------------------------------------------------------------

#ifdef F_CPU  
#undef F_CPU
#endif  
#define F_CPU 1000000 // 1 MHz  

/*
#include <SoftwareSerial.h>
#define rxPin 7 //PA3
#define txPin 3 //PA7
SoftwareSerial mySerial(rxPin, txPin);

*/


#include "EmonLib.h" // Include Emon Library
EnergyMonitor emon1; // Create an instance


#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"

/*
                     +-\/-+
               VCC  1|    |14  GND
          (D0) PB0  2|    |13  AREF (D10)
          (D1) PB1  3|    |12  PA1 (D9)
             RESET  4|    |11  PA2 (D8)
INT0  PWM (D2) PB2  5|    |10  PA3 (D7)
      PWM (D3) PA7  6|    |9   PA4 (D6)
      PWM (D4) PA6  7|    |8   PA5 (D5) PWM
                     +----+
*/


ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define myNodeID 20      // RF12 node ID in the range 1-30
#define network 210      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module


//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

// realPower|apparentPower|Vrms|Irms|powerFactor
typedef struct {int supplyV; int apparentPower;} PayloadTX;
PayloadTX emontx;

//########################################################################################################################

static void setPrescaler (uint8_t mode) {
    cli();
    CLKPR = bit(CLKPCE);
    CLKPR = mode;
    sei();
}

void setup() {

  
  setPrescaler(1);    // div 1, i.e. 4 MHz
  //setPrescaler(15); // div 8, i.e. 1 MHz div 256 (15) = 32kHz
  //setPrescaler(3);  // div 8, i.e. 1 MHz

    //pinMode(rxPin, INPUT);
    //pinMode(txPin, OUTPUT);
  
    // set the data rate for the NewSoftmySerial port
    //mySerial.begin(38400);
    //mySerial.println("\n[TinySensor current monitor]");

	
    analogReference(DEFAULT);

  //emon1.voltage(8, 212.658, 1.7);  // Voltage: input pin, calibration, phase_shift
//  emon1.current(PA1, 60.61);       // Current: input pin, calibration.
 
   emon1.current(PA1, 10.11);       // Current: input pin, calibration.

  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  // Adjust low battery voltage to 2.2V 
  rf12_control(0xC040);
  rf12_sleep(0);                          // Put the RFM12 to sleep
  
  PRR = bit(PRTIM1); // only keep timer 0 going
}

void loop() {
  
  #define BODS 7 //BOD Sleep bit in MCUCR
  #define BODSE 2 //BOD Sleep enable bit in MCUCR
  MCUCR |= _BV(BODS) | _BV(BODSE); //turn off the brown-out detector

  setPrescaler(3);  // div 8, i.e. 1 MHz
  bitClear(PRR, PRADC); // power up the ADC
  ADCSRA |= bit(ADEN); // enable the ADC  
  
  Sleepy::loseSomeTime(16); // Allow 10ms for the sensor to be ready
  
  emontx.supplyV = readVcc(); // Get supply voltage
  
  //emon1.calcVI(20,2000);         // Calculate all. No.of wavelengths, time-out
  // realPower|apparentPower|Vrms|Irms|powerFactor
  //emontx.realPower = emon1.realPower;
  double irms=emon1.calcIrms(1480);
  emontx.apparentPower = (int)(irms*220);  
  //emontx.Vrms = emon1.Vrms;
  //emontx.Irms = emon1.Irms;
  //emontx.powerFactor = emon1.powerFactor*100;
 
  //mySerial.print("Irms:");  mySerial.println(irms*220); 
  
  ADCSRA &= ~ bit(ADEN); // disable the ADC
  bitSet(PRR, PRADC); // power down the ADC
  
  rfwrite(); // Send data via RF 

//  for(byte j = 0; j < 5; j++) {    // Sleep for 5 minutes
    Sleepy::loseSomeTime(10000); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)
//  }

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
   rf12_sendStart(0, &emontx, sizeof emontx); 
   rf12_sendWait(2);    //wait for RF to finish sending while in standby mode
   rf12_sleep(0);    //put RF module to sleep
   bitSet(PRR, PRUSI); // disable USI h/w
   setPrescaler(4); // div 4, i.e. 1 MHz
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

