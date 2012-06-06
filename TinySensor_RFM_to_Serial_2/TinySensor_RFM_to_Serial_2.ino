#include <NewSoftSerial.h>
#define rxPin 7 //PA3
#define txPin 3 //PA7

#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"

NewSoftSerial mySerial(rxPin, txPin);


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

#include <RF12.h>

#define MYNODE 22            
#define freq RF12_868MHZ     // frequency
#define group 210            // network group 

// realPower|apparentPower|Vrms|Irms|powerFactor
typedef struct { int realPower, apparentPower, Vrms, powerFactor; double Irms; } PayloadTX;
PayloadTX emontx;

int incomingByte = 0;   // for incoming mySerial data

void setup(){
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
  
    // set the data rate for the NewSoftmySerial port
    mySerial.begin(9600);
  
    mySerial.println("\n[TinySensor RFM12b to Serial v1.1]");
    rf12_initialize(MYNODE, freq,group);
    delay(500);
}


void loop(){
  
   delay(500);

   if (mySerial.available() > 0) {
   // read the incoming byte:
     incomingByte = mySerial.read();

   // say what you got:
      mySerial.print("I received: ");
      mySerial.println(incomingByte, DEC);
   }
                
 if (rf12_recvDone()){      
      if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)
      {               
        int node_id = (rf12_hdr & 0x1F);

        if (node_id == 10)                                               // EMONTX
        {
          emontx = *(PayloadTX*) rf12_data;                              // get emontx data
          mySerial.print("{realPower:");     mySerial.print(emontx.realPower);          // Add power reading
          mySerial.print(",apparentPower:"); mySerial.print(emontx.apparentPower);          // Add power reading
          mySerial.print(",Vrms:");          mySerial.print(emontx.Vrms);          // Add power reading 
          mySerial.print(",Irms:");          mySerial.print(emontx.Irms);        // 
          mySerial.print(",powerFactor:");   mySerial.print(emontx.powerFactor);        //                 
          mySerial.println("}");                                    
        }   
        }        
}}

