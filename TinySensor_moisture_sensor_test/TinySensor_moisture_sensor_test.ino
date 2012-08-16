#include <SoftwareSerial.h>
#define rxPin 7 //PA3
#define txPin 3 //PA3
SoftwareSerial mySerial(rxPin, txPin);

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif



#define MOISTUREPIN 1
unsigned int moistureReading;

#define LDR 2
unsigned int ldrReading;

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


void setup(){
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
  
    // set the data rate for the NewSoftmySerial port
    mySerial.begin(38400);
    mySerial.println("\n[TinySensor....]");
    pinMode(MOISTUREPIN, INPUT);
    pinMode(LDR, INPUT);
    pinMode(PB0,OUTPUT);
    digitalWrite(PB0, HIGH);
    
//    analogReference(DEFAULT);
    delay(500);
    
    //ADC prescaler=64, 8Mhz/64 results 125Khz ADC sample rate
    sbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    cbi(ADCSRA, ADPS0);

    //
    cbi(ADCSRB, ACME);
    
    //ACD: Analog Comparator Disable
    sbi(ACSR,ACD);
    
    sbi(ADCSRA, ADEN);
}


void loop(){
   
  //      digitalWrite(PB0, HIGH);
//        delay(10);
        ldrReading=0;
        myanalogRead(LDR);
        delay(50);
	for(int i = 0; i < 10 ; i++) // take 10 more readings
	{
		ldrReading+=myanalogRead(LDR); // accumulate readings
                delay(15);
	}
	ldrReading = ldrReading / 10 ; // calculate the average

	mySerial.print("Raw LDR reading:"); mySerial.println(ldrReading);	
//        digitalWrite(PB0, LOW);

        delay(500);
//        digitalWrite(PB0, HIGH);
//        delay(10);
  	moistureReading=0;
        myanalogRead(MOISTUREPIN);
        delay(50);
	for(int i = 0; i < 10 ; i++) // take 10 more readings
	{
		moistureReading+=myanalogRead(MOISTUREPIN); // accumulate readings
                delay(15);
	}
	moistureReading = moistureReading / 10 ; // calculate the average
	mySerial.print("Raw soil moisture reading:"); mySerial.println(moistureReading);	        
//        digitalWrite(PB0, LOW);

	delay(50);

   }


// Used from Arduino wiring_analog.c
int myanalogRead(uint8_t pin) {
	uint8_t low, high;
	
	// set the analog reference (high two bits of ADMUX) and select the
	// channel (low 4 bits).  this also sets ADLAR (left-adjust result)
	// to 0 (the default).
	//	ADMUX = (analog_reference << 6) | (pin & 0x3f); // more MUX
	// sapo per tiny45

	ADMUX =  pin & 0x3f;

//VCC used as analog reference, disconnected from PA0 (AREF)
	cbi(ADMUX,REFS0);
	cbi(ADMUX,REFS1);
        ADMUX &= ~_BV( ADLAR );       // Right-adjust result
        
	// without a delay, we seem to read from the wrong channel
	delay(20);
       
        ADCSRA &= ~( _BV( ADATE ) |_BV( ADIE ) ); // Disable autotrigger, Disable Interrupt

	// start the conversion
	sbi(ADCSRA, ADSC);
	
	// ADSC is cleared when the conversion finishes
	while (bit_is_set(ADCSRA, ADSC));
	
	// we have to read ADCL first; doing so locks both ADCL
	// and ADCH until ADCH is read.  reading ADCL second would
	// cause the results of each conversion to be discarded,
	// as ADCL and ADCH would be locked when it completed.
	low = ADCL;
	high = ADCH;
	
	// combine the two bytes
	return (high << 8) | low;
}

