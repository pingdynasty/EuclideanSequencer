// #define SERIAL_DEBUG

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "device.klasmata.h"
#include "adc_freerunner.cpp"
#include "DeadbandController.h"
#include "Sequence.h"

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

enum GateSequencerMode {
  DISABLED                   = 0,
  TRIGGERING                 = _BV(TRIGGERING_BIT),
  ALTERNATING                = _BV(ALTERNATING_BIT)
};

class GateSequencer : public Sequence<uint32_t> {
public:
  GateSequencerMode mode;
  //   GateSequencer* linked;
  GateSequencer(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4):
    output(p1), trigger(p2), alternate(p3), led(p4){
    SEQUENCER_TRIGGER_SWITCH_PORT |= _BV(trigger);
    SEQUENCER_ALTERNATE_SWITCH_PORT |= _BV(alternate);
    SEQUENCER_OUTPUT_DDR |= _BV(output);
    SEQUENCER_LEDS_PORT |= _BV(led);
    off();
  }
  void update(){
    if(isAlternating())
      mode = ALTERNATING;
    else
      mode = TRIGGERING;
  }
  void rise(){
    switch(mode){
    case ALTERNATING:
      if(next())
	toggle();
      break;
    case TRIGGERING:
      if(next())
	on();
      break;
    }
  }
  void fall(){
    switch(mode){
    case TRIGGERING:
      off();
      break;
    }
  }
  void reset(){
    Sequence<uint32_t>::reset();
    off();
  }
  inline void on(){
    SEQUENCER_OUTPUT_PORT &= ~_BV(output);
    SEQUENCER_LEDS_PORT |= _BV(led);
  }
  inline void off(){
    SEQUENCER_OUTPUT_PORT |= _BV(output);
    SEQUENCER_LEDS_PORT &= ~_BV(led);
  }
  inline void toggle(){
    SEQUENCER_OUTPUT_PORT ^= _BV(output);
    SEQUENCER_LEDS_PORT ^= _BV(led);
  }
  inline bool isOn(){
    return !(SEQUENCER_OUTPUT_PINS & _BV(output));
  }
  inline bool isTriggering(){
    return !(SEQUENCER_TRIGGER_SWITCH_PINS & _BV(trigger));
  }
  inline bool isAlternating(){
    return !(SEQUENCER_ALTERNATE_SWITCH_PINS & _BV(alternate));
  }
  inline bool isEnabled(){
    return isAlternating() || isTriggering();
  }

#ifdef SERIAL_DEBUG
  void dump(){
    printInteger(pos);
    printString(", ");
    printInteger(length);
    printString(", ");
    printInteger(offset);
    if(isOn())
      printString(", on");
    if(isTriggering())
      printString(", triggering");
    if(isAlternating())
      printString(", alternating");
    switch(mode){
    case DISABLED:
      printString(" DISABLED");
      break;
    case TRIGGERING:
      printString(" TRIGGERING");
      break;
    case ALTERNATING:
      printString(" ALTERNATING");
      break;
    }
  }
#endif

private:
  uint8_t output;
  uint8_t trigger;
  uint8_t alternate;
  uint8_t led;
};

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

bool recalculate = true;

class SequenceController : public DeadbandController<SEQUENCER_DEADBAND_THRESHOLD> {
  void hasChanged(uint16_t value){
    recalculate = true;
  }
};

class RotateController : public DeadbandController<SEQUENCER_DEADBAND_THRESHOLD> {
  void hasChanged(uint16_t val){
    val >>= 8; // scale 0-4095 down to 0-15
    seqA.rotate(val);
#ifdef SERIAL_DEBUG
    printInteger(val);
    printByte(':');
    printInteger(seqA.offset);
    printNewline();
#endif
  }
};

SequenceController fillA;
SequenceController stepA;
RotateController rotateA;

void loop(){
  stepA.update(getAnalogValue(SEQUENCER_STEP_CONTROL));
  fillA.update(getAnalogValue(SEQUENCER_FILL_CONTROL));
  if(recalculate){
    uint8_t steps = SEQUENCER_STEPS_RANGE - (stepA.value >> 7);
    uint8_t fills = steps - ((fillA.value >> 2) * steps) / (ADC_VALUE_RANGE >> 2);
    seqA.calculate(steps, fills);
#ifdef SERIAL_DEBUG
    printInteger(steps);
    printByte('.');
    printInteger(fills);
    printNewline();
#endif
    recalculate = false;
  }
  rotateA.update(getAnalogValue(SEQUENCER_ROTATE_CONTROL));  
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
