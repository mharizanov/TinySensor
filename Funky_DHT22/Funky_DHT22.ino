//--------------------------------------------------------------------------------------
// DHT22 temperature measurement of Attiny84 based Funky step-up
// harizanov.com
// GNU GPL V3
//--------------------------------------------------------------------------------------

#include <DHT22.h> // https://github.com/nathanchantrell/Arduino-DHT22
#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"

#define PowerPin 7      // DHT Power pin is connected on pin 
#define DHT22Pin 3      // DHT Data pin

DHT22 dht(DHT22Pin); // Setup the DHT

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

#define LEDpin PB0


//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
  	  int temp;	// Temperature reading
//  	  int supplyV;	// Supply voltage
          int humidity;
 } Payload;

 Payload temptx;

static void setPrescaler (uint8_t mode) {
    cli();
    CLKPR = bit(CLKPCE);
    CLKPR = mode;
    sei();
}
 
void setup() {

//  setPrescaler(1);    // div 1, i.e. 4 MHz
  pinMode(LEDpin,OUTPUT);
  digitalWrite(LEDpin,LOW);    
  Sleepy::loseSomeTime(1024); 
  digitalWrite(LEDpin,HIGH);

  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  rf12_control(0xC040);
  rf12_sleep(0);                          // Put the RFM12 to sleep
  PRR = bit(PRTIM1); // only keep timer 0 going
}

void loop() {
  pinMode(LEDpin,OUTPUT);
  digitalWrite(LEDpin,LOW);

  pinMode(PowerPin,OUTPUT);
  digitalWrite(PowerPin,HIGH);
 
  bitClear(PRR, PRADC); // power up the ADC
  ADCSRA |= bit(ADEN); // enable the ADC  

  Sleepy::loseSomeTime(50); // Allow for the sensor to be ready
  
  digitalWrite(LEDpin,HIGH);    //LED off
    
  Sleepy::loseSomeTime(1500); // Allow for the sensor to be ready

  DHT22_ERROR_t errorCode;  
  errorCode = dht.readData(); // read data from sensor
  digitalWrite(PowerPin,LOW);   //Power DHT22 off

  if (errorCode == DHT_ERROR_NONE) { // data is good  
    temptx.temp = (dht.getTemperatureC()*100); // Get temperature reading and convert to integer, reversed at receiving end
//  temptx.supplyV = readVcc(); // Get supply voltage (no need in the step-up Funkya as it is always 3.3V
    temptx.humidity = (dht.getHumidity()*100); // Get humidity reading and convert to integer, reversed at receiving end
    rfwrite(); // Send data via RF   
}
  
  
  ADCSRA &= ~ bit(ADEN); // disable the ADC
  bitSet(PRR, PRADC); // power down the ADC

  for(int j = 0; j < 5; j++) {    // Sleep for 5 minutes
    Sleepy::loseSomeTime(65000); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)
  }
}

//--------------------------------------------------------------------------------------------------
// Send payload data via RF
//--------------------------------------------------------------------------------------------------
static void rfwrite(){
//  setPrescaler(1);    // div 1, i.e. 4 MHz
   bitClear(PRR, PRUSI); // enable USI h/w
   rf12_sleep(-1);     //wake up RF module
   while (!rf12_canSend())
     rf12_recvDone();
   rf12_sendStart(0, &temptx, sizeof temptx); 
   rf12_sendWait(2);    //wait for RF to finish sending while in standby mode
   rf12_sleep(0);    //put RF module to sleep
   bitSet(PRR, PRUSI); // disable USI h/w
//   setPrescaler(4); // div 4, i.e. 1 MHz
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


