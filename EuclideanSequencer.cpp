// #define SERIAL_DEBUG

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "device.h"
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

inline bool isChained(){
  return !(SEQUENCER_CHAINED_SWITCH_PINS & _BV(SEQUENCER_CHAINED_SWITCH_PIN));
}

GateSequencer seqA(SEQUENCER_OUTPUT_PIN_A, 
		   SEQUENCER_TRIGGER_SWITCH_PIN_A,
		   SEQUENCER_ALTERNATE_SWITCH_PIN_A,
		   SEQUENCER_LED_A_PIN);

GateSequencer seqB(SEQUENCER_OUTPUT_PIN_B, 
		   SEQUENCER_TRIGGER_SWITCH_PIN_B,
		   SEQUENCER_ALTERNATE_SWITCH_PIN_B,
		   SEQUENCER_LED_B_PIN);


class MetaSequencer {
public:
  uint8_t counter;
  void rise(){
    if(++counter >= seqA.length+seqB.length)
      counter = 0;
    if(counter < seqA.length){
      seqA.rise();
      seqA.push(seqB);
    }else{
      seqB.rise();
      seqB.push(seqA);
    }
  }
  void fall(){
    if(counter < seqA.length){
      seqA.fall();
      seqA.push(seqB);
    }else{
      seqB.fall();
      seqB.push(seqA);
    }
  }
  void reset(){
    counter = 0;
  }
};

MetaSequencer combined;

void reset(){
  seqA.reset();
  seqB.reset();
  combined.reset();
}

/* Reset interrupt */
SIGNAL(INT0_vect){
  reset();
  // hold everything until reset is released
  while(resetIsHigh());
}

#if SEQUENCER_CHAINED_SWITCH_PIN == SEQUENCER_CLOCK_PIN
#error Chained mode switch and clock input must have different pin numbers!
#endif

#define NORMAL_AND_LOW   (_BV(SEQUENCER_CHAINED_SWITCH_PIN) | _BV(SEQUENCER_CLOCK_PIN))
#define NORMAL_AND_HIGH   _BV(SEQUENCER_CHAINED_SWITCH_PIN)
#define CHAINED_AND_LOW                                       _BV(SEQUENCER_CLOCK_PIN)
#define CHAINED_AND_HIGH  0

/* Clock interrupt */
SIGNAL(INT1_vect){
  uint8_t mode = 
    (SEQUENCER_CHAINED_SWITCH_PINS & _BV(SEQUENCER_CHAINED_SWITCH_PIN)) |
    (SEQUENCER_CLOCK_PINS & _BV(SEQUENCER_CLOCK_PIN));
  switch(mode){
  case NORMAL_AND_LOW:
    seqA.fall();
    seqB.fall();
    SEQUENCER_LEDS_PORT &= ~_BV(SEQUENCER_LED_C_PIN);
    break;
  case NORMAL_AND_HIGH:
    seqA.rise();
    seqB.rise();
    SEQUENCER_LEDS_PORT |= _BV(SEQUENCER_LED_C_PIN);
    break;
  case CHAINED_AND_LOW:
    combined.fall();
    SEQUENCER_LEDS_PORT &= ~_BV(SEQUENCER_LED_C_PIN);
    break;
  case CHAINED_AND_HIGH:
    combined.rise();
    SEQUENCER_LEDS_PORT |= _BV(SEQUENCER_LED_C_PIN);
    break;
  }
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
  SEQUENCER_CHAINED_SWITCH_DDR  &= ~_BV(SEQUENCER_CHAINED_SWITCH_PIN);
  SEQUENCER_CHAINED_SWITCH_PORT |= _BV(SEQUENCER_CHAINED_SWITCH_PIN);
  SEQUENCER_LEDS_DDR |= _BV(SEQUENCER_LED_C_PIN);
  reset();
  sei();
#ifdef SERIAL_DEBUG
  beginSerial(9600);
  printString("hello\n");
#endif
}

void loop(){
  seqA.rotation.update(getAnalogValue(SEQUENCER_ROTATE_A_CONTROL));
  seqA.step.update(getAnalogValue(SEQUENCER_STEP_A_CONTROL));
  seqA.fill.update(getAnalogValue(SEQUENCER_FILL_A_CONTROL));
  seqA.update();

  seqB.rotation.update(getAnalogValue(SEQUENCER_ROTATE_B_CONTROL));
  seqB.step.update(getAnalogValue(SEQUENCER_STEP_B_CONTROL));
  seqB.fill.update(getAnalogValue(SEQUENCER_FILL_B_CONTROL));
  seqB.update();

#ifdef SERIAL_DEBUG
  if(serialAvailable() > 0){
    serialRead();
    printString("a: [");
    seqA.dump();
    printString("] ");
    seqA.print();
    printString("b: [");
    seqB.dump();
    printString("] ");
    seqB.print();
    if(clockIsHigh())
      printString(" clock high");
    if(resetIsHigh())
      printString(" reset high");
    if(isChained())
      printString(" chained");
    printNewline();
  }
#endif
}
