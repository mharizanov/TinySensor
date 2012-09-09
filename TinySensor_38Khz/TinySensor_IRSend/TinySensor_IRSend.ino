/*
 * IRremote: IRsendDemo - demonstrates sending IR codes with IRsend
 * An IR LED must be connected to Arduino PWM pin 3.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */

#include <IRremote.h>

IRsend irsend;

void setup()
{
  pinMode(3,OUTPUT);
  digitalWrite(3,HIGH);
 }

void loop() {
 
  while(1) {
    for (int i = 0; i < 3; i++) {
//      irsend.sendSony(0xa90, 12); // Sony TV power code
  #define JVCPower              0xC5E8
  irsend.sendJVC(JVCPower, 16,1); // hex value, 16 bits, repeat
  
    }
    delay(100);
  }
}
