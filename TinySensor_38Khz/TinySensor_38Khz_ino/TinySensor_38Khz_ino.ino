#define freq_to_timerval(x) ((F_CPU / x - 1)/ 2)

void initPWM0(){
    DDRA |= (1 << 7);			//Set OC0B to output

    TCCR0A |= ( (1 << COM0B1) | (1 << WGM01) | (1 << WGM00));

    							//This sets COMB to 2 which is
    							//non-inverted PWM generation.  Also WGM
    							//is set to 3 which is fast PWM mode

     TCCR0B |= ((1 << CS01) | (1 << CS01));  	//This turns the the 8 bit counter 0 on and
    							//sets the pre-scaler to 256 (305 Hz)
}


inline void setPWM0B(uint8_t num){
	OCR0B = num;
}

void setup() {

     pinMode(3,OUTPUT);
//     digitalWrite(3,HIGH);
//     delay(2000);
 //    digitalWrite(3,LOW);     
}
void loop( ){

    pinMode(3,OUTPUT);
    digitalWrite(3,HIGH);

    initPWM0();

    uint8_t pwmBSetting;
        setPWM0B(freq_to_timerval(38400));

    while(1){
    	pwmBSetting += 1;

//    	setPWM0B(pwmBSetting);
  
//        setPWM0B(freq_to_timerval(38400));
  
 //       Delay(1);

    }
}

void Delay(int delay)
{
int x, y;
for (x = delay; x != 0; x--)
   {
   for (y = 1000; y != 0; y--)
       {
          asm volatile ("nop"::); //do nothing for a bit
       }
    }
}
