// #define SERIAL_DEBUG

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "device.h"
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

inline bool isChained(){
  return !(SEQUENCER_CHAINED_SWITCH_PINS & _BV(SEQUENCER_CHAINED_SWITCH_PIN));
}

class GateSequencer;

class SequenceController : public DeadbandController<SEQUENCER_DEADBAND_THRESHOLD> {
public:
  GateSequencer* seq;
  SequenceController(GateSequencer* s) : seq(s) {}
  void hasChanged(uint16_t value);
};

class RotateController : public DeadbandController<SEQUENCER_DEADBAND_THRESHOLD> {
public:
  GateSequencer* seq;
  RotateController(GateSequencer* s) : seq(s) {}
  void hasChanged(uint16_t val);
};

enum GateSequencerMode {
  DISABLED                   =  0,
  TRIGGERING                 =  1,
  ALTERNATING                =  2
};

class GateSequencer : public Sequence<uint16_t> {
public:
  SequenceController step;
  SequenceController fill;
  RotateController rotation;
  volatile GateSequencerMode mode;
  bool recalculate;
  GateSequencer(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4):
    step(this), fill(this), rotation(this), mode(DISABLED), recalculate(true),
    output(p1), trigger(p2), alternate(p3), led(p4){
    SEQUENCER_TRIGGER_SWITCH_DDR &= ~_BV(trigger);
    SEQUENCER_TRIGGER_SWITCH_PORT |= _BV(trigger);
    SEQUENCER_ALTERNATE_SWITCH_DDR &= ~_BV(alternate);
    SEQUENCER_ALTERNATE_SWITCH_PORT |= _BV(alternate);
    SEQUENCER_OUTPUT_DDR |= _BV(output);
    SEQUENCER_LEDS_DDR |= _BV(led);
    SEQUENCER_LEDS_PORT |= _BV(led);
    off();
  }
  void update(){
    if(recalculate){
      uint8_t s = SEQUENCER_STEPS_RANGE - (step.value >> 8);
      uint8_t f = s - ((fill.value >> 2) * s) / (ADC_VALUE_RANGE >> 2);
      calculate(s, f);
      recalculate = false;
#ifdef SERIAL_DEBUG
      printInteger(s);
      printByte('.');
      printInteger(f);
      printNewline();
#endif
    }
    GateSequencerMode newmode;
    if(isTriggering())
      newmode = TRIGGERING;
    else if(isAlternating())
      newmode = ALTERNATING;
    else
      newmode = DISABLED;
    mode = newmode;
  }
  void push(GateSequencer& seq){
    if(isOn())
      seq.on();
    else
      seq.off();
  }
  void rise(){
    switch(mode){
    case TRIGGERING:
      if(next())
	on();
      break;
    case ALTERNATING:
      if(next())
	toggle();
      break;
    case DISABLED:
      next();
      off();
      break;
    }
  }
  void fall(){
    switch(mode){
    case DISABLED:
    case TRIGGERING:
      off();
      break;
    case ALTERNATING:
      break;
    }
  }
  void reset(){
    Sequence<uint16_t>::reset();
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

void SequenceController::hasChanged(uint16_t value){
  seq->recalculate = true;
}

void RotateController::hasChanged(uint16_t val){
  val >>= 8; // scale 0-4095 down to 0-15
  seq->rotate(val);
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

/* Clock interrupt */
SIGNAL(INT1_vect){
  if(clockIsHigh()){
    if(isChained()){
      combined.rise();
    }else{
      seqA.rise();
      seqB.rise();
    }
    SEQUENCER_LEDS_PORT |= _BV(SEQUENCER_LED_C_PIN);
  }else{
    if(isChained()){
      combined.fall();
    }else{
    seqA.fall();
    seqB.fall();
    }
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
