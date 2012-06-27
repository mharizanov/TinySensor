// Attiny84 power savings test

#include <NewSoftSerial.h>
#define rxPin 7 //PA3
#define txPin 3 //PA7

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


#include <JeeLib.h>
#include <util/crc16.h>
#include <util/parity.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#define LED_PIN     8   // activity LED, comment out to disable

static void activityLed (byte on) {

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, on);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// RF12 configuration setup code

typedef struct {
    byte nodeId;
    byte group;
//    char msg[30-4];
    word crc;
} RF12Config;

static RF12Config config;

static char cmd;
static byte value, stack[30], top, sendLen, dest, quiet;
static byte testbuf[30], testCounter;

static void setPrescaler (uint8_t mode) {
    cli();
    CLKPR = bit(CLKPCE);
    CLKPR = mode;
    sei();
}

char helpText1[] PROGMEM = 
    "\n"
    "Available commands:" "\n"
    "  <nn> i     - set node ID (standard node ids are 1..26)" "\n"
    "               (or enter an uppercase 'A'..'Z' to set id)" "\n"
    "  <n> b      - set MHz band (4 = 433, 8 = 868, 9 = 915)" "\n"
    "  <nnn> g    - set network group (RFM12 only allows 212, 0 = any)" "\n"
    "  <n> c      - set collect mode (advanced, normally 0)" "\n"
    "  t          - broadcast max-size test packet, with ack" "\n"
    "  ...,<nn> a - send data packet to node <nn>, with ack" "\n"
    "  ...,<nn> s - send data packet to node <nn>, no ack" "\n"
    "  <n> l      - turn activity LED on DIG8 on or off" "\n"
    "  <n> q      - set quiet mode (1 = don't report bad packets)" "\n"
;

static void showString (PGM_P s) {
    for (;;) {
        char c = pgm_read_byte(s++);
        if (c == 0)
            break;
        if (c == '\n')
            mySerial.print('\r');
        mySerial.print(c);
    }
}

static void showHelp () {
    showString(helpText1);
}

static void handleInput (char c) {
    if ('0' <= c && c <= '9')
        value = 10 * value + c - '0';
    else if (c == ',') {
        if (top < sizeof stack)
            stack[top++] = value;
        value = 0;
    } else if ('a' <= c && c <='z') {
        mySerial.print("> ");
        mySerial.print((int) value);
        mySerial.println(c);
        switch (c) {
            default:
                showHelp();
                break;
            case 'u': // USI
                  if(value)
                      {
                      bitClear(PRR, PRUSI); // enable USI h/w
                      mySerial.println("USI enabled");
                      }
                  else
                      {
                      bitSet(PRR, PRUSI); // enable USI h/w
                      mySerial.println("USI disabled");
                      }                     
                break;
                
            case 'a': // ADC
                  if(value)
                      {
                      bitClear(PRR, PRADC); // power up the ADC
                      ADCSRA |= bit(ADEN); // enable the ADC  

                      mySerial.println("ADC enabled");
                      }
                  else
                      {
                      ADCSRA &= ~ bit(ADEN); // disable the ADC
                      bitSet(PRR, PRADC); // power down the ADC
                      mySerial.println("ADC disabled");
                      }
                break;
 
            case 'p': 
                mySerial.print("Set prescaler to");
                mySerial.println(value);                
                setPrescaler(value);
                break;
                

            case 'l': // turn activity LED on or off
                activityLed(value);
                break;
        }
        value = top = 0;
        memset(stack, 0, sizeof stack);
     
}

void setup() {
    
    activityLed(1);
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    
    // set the data rate for the NewSoftmymySerial port
    mySerial.begin(57600);
    
    mySerial.print("\n[Attiny84 power saving test]");

    delay(2000);
    rf12_initialize(15, RF12_868MHZ ,210);        
    }
   
    showHelp();
   activityLed(0);
}

void loop() {
    if (mySerial.available())
        handleInput(mySerial.read());
}

