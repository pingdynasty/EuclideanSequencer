// #define SERIAL_DEBUG

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "device.klasmata.h"
#include "adc_freerunner.cpp"
#include "DeadbandController.h"
#include "GateSequencer.h"

#ifdef SERIAL_DEBUG
#include "serial.h"
#endif // SERIAL_DEBUG

inline bool clockIsHigh(){
  return !(SEQUENCER_CLOCK_PINS & _BV(SEQUENCER_CLOCK_PIN));
}

inline bool resetIsHigh(){
  return !(SEQUENCER_RESET_PINS & _BV(SEQUENCER_RESET_PIN));
}

#define TRIGGERING_BIT  1
#define ALTERNATING_BIT 2

GateSequencer seqA(SEQUENCER_OUTPUT_PIN_A, 
		   SEQUENCER_TRIGGER_SWITCH_PIN_A,
		   SEQUENCER_ALTERNATE_SWITCH_PIN_A,
		   SEQUENCER_LED_A_PIN);

/* Reset interrupt */
SIGNAL(INT0_vect){
  seqA.reset();
  // hold everything until reset is released
  while(resetIsHigh());
}

/* Clock interrupt */
SIGNAL(INT1_vect){
  if(clockIsHigh()){
    seqA.rise();
    SEQUENCER_LEDS_PORT |= _BV(SEQUENCER_LED_C_PIN);
  }else{
    seqA.fall();
    SEQUENCER_LEDS_PORT &= ~_BV(SEQUENCER_LED_C_PIN);
  }
  // debug
//   PORTB ^= _BV(PORTB4);
}

void setup(){
  cli();
  // define interrupt 0 and 1
//   EICRA = (1<<ISC10) | (1<<ISC01) | (1<<ISC00); // trigger int0 on rising edge
  EICRA = (1<<ISC10) | (1<<ISC01);
  // trigger int0 on the falling edge, since input is inverted
  // trigger int1 on any logical change.
  // pulses that last longer than one clock period will generate an interrupt.
  EIMSK =  (1<<INT1) | (1<<INT0);
  // enables INT0 and INT1
  SEQUENCER_CLOCK_DDR &= ~_BV(SEQUENCER_CLOCK_PIN);
  SEQUENCER_CLOCK_PORT |= _BV(SEQUENCER_CLOCK_PIN); // enable pull-up resistor

  SEQUENCER_RESET_DDR &= ~_BV(SEQUENCER_RESET_PIN);
  SEQUENCER_RESET_PORT |= _BV(SEQUENCER_RESET_PIN); // enable pull-up resistor

  setup_adc();

  SEQUENCER_LEDS_DDR |= _BV(SEQUENCER_LED_A_PIN);
  SEQUENCER_LEDS_DDR |= _BV(SEQUENCER_LED_C_PIN);

  sei();

#ifdef SERIAL_DEBUG
  beginSerial(9600);
  printString("hello\n");
#endif
}

void loop(){
  seqA.rotation.update(getAnalogValue(SEQUENCER_ROTATE_CONTROL));
  seqA.step.update(getAnalogValue(SEQUENCER_STEP_CONTROL));
  seqA.fill.update(getAnalogValue(SEQUENCER_FILL_CONTROL));
  seqA.update();

#ifdef SERIAL_DEBUG
  if(serialAvailable() > 0){
    serialRead();
    printString("a: [");
    seqA.dump();
    printString("] ");
    seqA.print();
    if(clockIsHigh())
      printString(" clock high");
    if(resetIsHigh())
      printString(" reset high");
    printNewline();
  }
#endif
}
