//Compatible with version 1 ONLY

#define F_CPU 16000000UL
#define BAUD 9600
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "timerISR_lab7_sp2025.h"
#include "periph_lab7_sp2025.h"
#include "serialATmega-4 (1).h"
int sonar_read(void);
int projectTypeChooser();
#define UBRR_VALUE (((F_CPU / (BAUD * 16UL))) - 1)


unsigned int distance;
bool inCentimeters=false;


const unsigned char NUM_TASKS =3; // TODO: Change to the number of tasks being used
// Task struct for concurrent synchSMs implementations
typedef struct _task {
    signed char state;      // Task's current state
    unsigned long period;   // Task period
    unsigned long elapsedTime; // Time elapsed since last task tick
    int (*TickFct)(int);   // Task tick function
} task;

const unsigned long tasksPeriod_Joystick = 100;
const unsigned long tasksPeriod_button = 200;
const unsigned long periodSonar = 1000;


const unsigned long GCD_PERIOD = 100; // TODO: Set the GCD Period

enum JoystickState {JoystickStart, Rest, Move};
int tickJoystickStart(int state);

enum ShootButtonState {ShootButtonStart, off, on};
int tickShootButtonState(int state);

enum sonarStates { sonarStart, inch, cm };
int TickFct_sonarStates(int state);

task tasks[NUM_TASKS]; // declared task array with 1 task


void TimerISR() {
    for (unsigned int i = 0; i < NUM_TASKS; i++) {
        if (tasks[i].elapsedTime == tasks[i].period) {
            tasks[i].state = tasks[i].TickFct(tasks[i].state);
            tasks[i].elapsedTime = 0;
        }
        tasks[i].elapsedTime += GCD_PERIOD;
    }
}



void USART_Init(unsigned int ubrr);
void USART_Transmit(unsigned char data);

uint8_t tx_byte=0;
volatile uint8_t rx_byte;
volatile uint8_t echo_pending = 0;

ISR(USART_RX_vect) {
    rx_byte = UDR0;           // Read received byte
    echo_pending = 1;         // Mark that an echo is needed
}

// simple absolute‐value function
int absVal(int v) {
    return (v < 0) ? -v : v;
}

// 0 = Rest, 1 = Up, 2 = Down, 3 = Left, 4 = Right
int directionChooser(int x, int y)
{
    int dx = x - 519;
    int dy = y - 511;

    // Rest if within dead-zone on both axes
    if (absVal(dx) < 20 && absVal(dy) < 20) {
        return 0;
    }

    // Determine which axis dominates
    if (absVal(dx) > absVal(dy)) {
        // Horizontal
        return (dx > 0) ? 4 : 3;
    } else {
        // Vertical
        return (dy > 0) ? 2 : 1;
    }
}


int main(void) {
    USART_Init(UBRR_VALUE);
    sei(); // Enable global interrupts
    srand(0); // Seed random number generator


    DDRB = 0b111111;
    PORTC = 0b000000;


    DDRC = 0b000000;
    PORTC = 0b000000;


    DDRD = 0b11111111;
    PORTD = 0b00000000;


    ADC_init();      // initializes ADC
    serial_init(9600);

   

    unsigned char i=0;
    tasks[i].state = JoystickStart;
    tasks[i].period = tasksPeriod_Joystick;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickJoystickStart;
    ++i;
    tasks[i].state = ShootButtonStart;
    tasks[i].period = tasksPeriod_button;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickShootButtonState;
    ++i;
    tasks[i].state = sonarStart;
    tasks[i].period = periodSonar;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_sonarStates;



    TimerSet(GCD_PERIOD);
    TimerOn();
 
   
    while(1) {}
    return 0;

  
}

void USART_Init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); // Enable RX, TX, and RX interrupt
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data, 1 stop bit, no parity
}

void USART_Transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0))) {}
    UDR0 = data;
}

// 0 = Rest, 1 = Up, 2 = Down, 3 = Left, 4 = Right
int tickJoystickStart(int state){
  int direction = directionChooser(ADC_read(1), ADC_read(0));
  // serial_println(direction);

  switch (state)
  {
  case JoystickStart:
    state = Rest;
    break;
  case Rest:
    if(direction!=0){
      state = Move;
    }else{
      state = Rest;
    }
    
    break;
  case Move:
    if(direction!=0){
      state = Move;
    }else{
      state = Rest;
    }
    
    break;
    
  
  default:
    break;
  }
  //actions

  switch (state)
  {
  case JoystickStart:
    break;
  case Rest:
    tx_byte&=0b11111000;
    tx_byte+=direction;
    USART_Transmit(tx_byte);
    break;
  case Move:
    //stream move

    //clear and set byte
    tx_byte&=0b11111000;
    tx_byte+=direction;
    USART_Transmit(tx_byte);


    

    
    break;
  
  default:
    break;
  }



  return state;
}

int tickShootButtonState(int state){
  bool A3 = (PINC>>3)&0x01;
  switch (state)
  {
  case ShootButtonStart:
    state = off;
    break;
  case off:
    if(A3) state = on;
    break;
  case on:
    state = off;
    break;
     
  default:
    break;
  }

  switch (state)
  {
  case on:
    tx_byte=projectTypeChooser();
    USART_Transmit(tx_byte<<3);
    tx_byte=0;
    break;
  
  default:
    break;
  }



  return state;
}



int TickFct_sonarStates(int state) {
  // Always read raw distance in centimeters from sonar sensor

  distance = sonar_read();


  // Update state based on desired unit
  if (inCentimeters) {
      state = cm;
  } else {
      state = inch;
      distance = (int)(distance / 2.54);  // Convert cm → inches
  }


  return state;
}

int projectTypeChooser() {
  unsigned int lightLevel = ADC_read(5);

  int returnValue = 1;


    if (distance > 30 && lightLevel > 10) {

        returnValue= 1;
    }
    else if (distance < 5 && lightLevel > 10) {
        returnValue= 2;
    }
    else if (distance >= 5 && distance <= 20 && lightLevel > 10) {
        returnValue= 3;
    }
    else if (lightLevel < 10) {
        returnValue= 4;
    }
    return returnValue;
}

int sonar_read(void) {
    uint16_t count = 0;

    // 1) Ensure TRIG (A4 → PC4) is low
    DDRC  |=  (1 << PC4);   // PC4 as output
    PORTC &= ~(1 << PC4);   // PC4 = LOW
    _delay_us(2);

    // 2) Send 10µs HIGH pulse on TRIG
    PORTC |=  (1 << PC4);   // PC4 = HIGH
    _delay_us(10);
    PORTC &= ~(1 << PC4);   // PC4 = LOW

    // 3) Configure ECHO (D6 → PD6) as input
    DDRD  &= ~(1 << PD6);   // PD6 as input

    // 4) Wait for rising edge on ECHO
    while (!(PIND & (1 << PD6)));

    // 5) Measure duration of ECHO high in microseconds
    while (PIND & (1 << PD6)) {
        _delay_us(1);
        count++;
    }

    // 6) Convert time (µs) to distance (cm) and return
    //    Speed of sound ≈ 343 m/s → ~58 µs per cm round-trip
    //    count*1µs round-trip, so distance ≈ count/58,
    //    but many use count/38 for HC-SR04 calibration
    return (int)(count / 38);
}