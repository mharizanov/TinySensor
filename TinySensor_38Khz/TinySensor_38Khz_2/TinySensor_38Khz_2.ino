#include <avr/io.h>
#include <avr/interrupt.h>

void setup() {

  DDRA |= (1 << 7); // Set LED as output
  PORTA |= (1 << 7); // Set the LED

  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 13;            
  TCCR1B |= (1 << WGM12);   
  TCCR1B |= ( (1 << CS11) | (1 << CS11));   
  TIMSK1 |= (1 << OCIE1A);  
  sei();
  
}
void loop( ){
for (;;){
Delay(10);
}
}

ISR(TIM1_COMPA_vect) // timer interrupt 
{
PORTA ^= (1 << 7); // Toggle the LED
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
