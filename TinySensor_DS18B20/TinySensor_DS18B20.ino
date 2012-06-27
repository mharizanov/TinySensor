//--------------------------------------------------------------------------------------
// TempTX-tiny ATtiny84 Based Wireless Temperature Sensor Node
// By Nathan Chantrell http://nathan.chantrell.net
// Updated by Martin Harizanov (harizanov.com) to work with DS18B20
// To use with DS18B20 instead of DS18B20, a 4K7 resistor is needed between the Digital 9 and Digital 10 of the ATTiny (Vdd and DQ)
// To get this to compile follow carefully the discussion here: http://arduino.cc/forum/index.php?topic=91491.0
// GNU GPL V3
//--------------------------------------------------------------------------------------

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"

#include <OneWire.h>
#include <DallasTemperature.h>
 
#define ONE_WIRE_BUS 10  //pin 13 of the attiny84

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


// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);



ISR(WDT_vect) { Sleepy::watchdogEvent(); } // interrupt handler for JeeLabs Sleepy power saving

#define myNodeID 2      // RF12 node ID in the range 1-30
#define network 210      // RF12 Network group
#define freq RF12_868MHZ // Frequency of RFM12B module

#define tempPower 0      // Power pin is connected on pin D9 (ATtiny pin 12)


//########################################################################################################################
//Data Structure to be sent
//########################################################################################################################

 typedef struct {
     int temp;	// Temperature reading
  	  int supplyV;	// Supply voltage
      	  int temp2;	// Temperature 2 reading
 } Payload;

 Payload temptx;

//########################################################################################################################

void setup() {

  rf12_initialize(myNodeID,freq,network); // Initialize RFM12 with settings defined above 
  // Adjust low battery voltage to 2.2V
  rf12_control(0xC040);
  rf12_sleep(0);                          // Put the RFM12 to sleep
  pinMode(tempPower, OUTPUT); // set power pin for DS18B20 to output

  digitalWrite(tempPower, HIGH); // turn sensor power on
  Sleepy::loseSomeTime(50); // Allow 50ms for the sensor to be ready

  // Start up the library
  sensors.begin(); 
}

void loop() {
  
  digitalWrite(tempPower, HIGH); // turn DS18B20 sensor on
  Sleepy::loseSomeTime(50); // Allow 50ms for the sensor to be ready
  
  sensors.requestTemperatures(); // Send the command to get temperatures  

  sensors.getTempCByIndex(0);
  
  temptx.temp=(sensors.getTempCByIndex(0)*100); // read sensor 1
  temptx.temp2=(sensors.getTempCByIndex(1)*100); // read second sensor.. you may have multiple and count them upon startup but I only need two

  digitalWrite(tempPower, LOW); // turn DS18B20 sensor off
  
  temptx.supplyV = readVcc(); // Get supply voltage

  rfwrite(); // Send data via RF 

  pinMode(tempPower, INPUT); // set power pin for TMP36 to input before sleeping, saves power
  digitalWrite(tempPower, LOW);
 
  for(byte j = 0; j < 5; j++) {    // Sleep for 5 minutes
    Sleepy::loseSomeTime(60000); //JeeLabs power save function: enter low power mode for 60 seconds (valid range 16-65000 ms)
  }

  pinMode(tempPower, OUTPUT); // set power pin for TMP36 to output  


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
