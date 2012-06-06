#include <JeeLib.h> // https://github.com/jcw/jeelib
#include "pins_arduino.h"

#include <RF12.h>

#define MYNODE 22            
#define freq RF12_868MHZ     // frequency
#define group 210            // network group 

// realPower|apparentPower|Vrms|Irms|powerFactor
typedef struct { int realPower, apparentPower, Vrms, powerFactor; double Irms; } PayloadTX;
PayloadTX emontx;


void setup(){
    Serial.begin(9600);
    Serial.println("\n[TinySensor RFM12b to Serial v1.0]");
    rf12_initialize(MYNODE, freq,group);
    delay(5000);
}


void loop(){
   Serial.print(".");
   delay(500);

 if (rf12_recvDone()){      
      if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)
      {

        int node_id = (rf12_hdr & 0x1F);
        
        if (node_id == 10)                                               // EMONTX
        {
          emontx = *(PayloadTX*) rf12_data;                              // get emontx data
          Serial.println();                                              // print emontx data to serial
          Serial.print("1 emontx packet rx:");
          
          delay(50);
                               
          Serial.print("{realPower:");     Serial.print(emontx.realPower);          // Add power reading
          Serial.print(",apparentPower:"); Serial.print(emontx.apparentPower);          // Add power reading
          Serial.print(",Vrms:");          Serial.print(emontx.Vrms);          // Add power reading 
          Serial.print(",Irms:");          Serial.print(emontx.Irms);        // 
          Serial.print(",powerFactor:");   Serial.print(emontx.powerFactor);        //                 
          Serial.println("}");                           
          
        }
     
        }
        
}}

