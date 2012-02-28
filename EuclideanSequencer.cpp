#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "device.h"
#include "bjorklund.h"
#include "adc_freerunner.h"
#include "DiscreteController.h"

// debug
#include "serial.h"
// #include "wiring.h"
// #include "WProgram.h"
// void beginSerial(long baud);

inline bool clockIsHigh(){
  return SEQUENCER_CLOCK_PINS & _BV(SEQUENCER_CLOCK_PIN);
}

inline bool isChained(){
  return !(SEQUENCER_CHAINED_SWITCH_PINS & _BV(SEQUENCER_CHAINED_SWITCH_PIN));
}

/*
  take measurements:
  - BassStation output trigger level
  - doepfer square wave levels
  - firmware ms per loop cycle
  - ms from clock rising to output rising
  - ms from clock falling to output falling
*/
class Sequence {
public:
  Sequence() : pos(0), offset(0) {}
  void calculate(int fills){
    Bjorklund algo;
    bits = algo.compute(length, fills);
    int8_t offs = offset;
    offset = 0;
    rol(offs);
  }
  // debug
  void print(){
    for(int i=0; i<length; ++i)
      serialWrite((bits & (1<<i)) ? 'x' : '.');
    printNewline();
  }
  /* Rotate Left */
  void rol(uint8_t steps){
    bits = (bits << steps) | (bits >> (length-steps));
    offset += steps;
  }
  /* Rotate Right */
  void ror(uint8_t steps){
    bits = (bits >> steps) | (bits << (length-steps));
    offset -= steps;
  }
  bool next(){
    if(pos >= length)
      pos = 0;
    return bits & pos++;
  }
// private:
  uint32_t bits;
  uint8_t length;
  int8_t offset;
  volatile uint8_t pos;
};

class GateSequencer : public Sequence {
public:
  GateSequencer(uint8_t p1, uint8_t p2, uint8_t p3):
    output(p1), trigger(p2), alternate(p3){
    SEQUENCER_TRIGGER_SWITCH_DDR &= ~_BV(trigger);
    SEQUENCER_TRIGGER_SWITCH_PORT |= _BV(trigger);
    SEQUENCER_ALTERNATE_SWITCH_DDR &= ~_BV(alternate);
    SEQUENCER_ALTERNATE_SWITCH_PORT |= _BV(alternate);
    SEQUENCER_OUTPUT_DDR |= _BV(output);
    off();
  }
  void rise(){
    if(Sequence::next()){
      if(isTriggering())
	on();
      else if(isAlternating())
	toggle();
      else
	off();
    }else{
      off();
    }
  }
  void fall(){
    if(!isAlternating())
      off();
  }
  void reset(){
    pos = 0;
  }
  inline void on(){
    SEQUENCER_OUTPUT_PORT |= _BV(output);
  }
  inline void off(){
    SEQUENCER_OUTPUT_PORT &= ~_BV(output);
  }
  inline bool isOn(){
    // todo: should this be:
//     return SEQUENCER_OUTPUT_PINS & _BV(output);
    return SEQUENCER_OUTPUT_PORT & _BV(output);
  }
  inline void toggle(){
    SEQUENCER_OUTPUT_PORT ^= _BV(output);
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
private:
  uint8_t output;
  uint8_t trigger;
  uint8_t alternate;
};

GateSequencer seqA(SEQUENCER_OUTPUT_PIN_A, 
		   SEQUENCER_TRIGGER_SWITCH_PIN_A,
		   SEQUENCER_ALTERNATE_SWITCH_PIN_A);

GateSequencer seqB(SEQUENCER_OUTPUT_PIN_B, 
		   SEQUENCER_TRIGGER_SWITCH_PIN_B,
		   SEQUENCER_ALTERNATE_SWITCH_PIN_B);

class StepController : public DiscreteController {
public:
  Sequence& seq;
  DiscreteController& fills;
  StepController(Sequence& s, DiscreteController& f) : seq(s), fills(f) {
    range = 16;
    value = -1;
  }
  virtual void hasChanged(int8_t steps){
    steps += 8; // range is 8-24
    seq.length = steps;
    fills.range = steps;
    fills.value = -1; // force change
  }
};

class FillController : public DiscreteController {
public:
  Sequence& seq;
  FillController(Sequence& s) : seq(s) {}
  virtual void hasChanged(int8_t val){
    seq.calculate(val+1);
    // debug
    printString("E(");
    printInteger(val+1);
    printString(", ");
    printInteger(seq.length);
    printString(", ");
    printInteger(seq.offset);
    printString(") ->\t");
    seq.print();
  }
};

class RotateController : public DiscreteController {
public:
  Sequence& seq;
  RotateController(Sequence& s) : seq(s) {
    range = 8;
    value = 1;
  }
  virtual void hasChanged(int8_t val){
    if(val > seq.offset)
      seq.rol(val-seq.offset);
    else if(val < seq.offset)
      seq.ror(seq.offset-val);
    // debug
    printInteger(val);
    printString(": ");
    seq.print();
    printNewline();
  }
};

FillController fillA(seqA);
FillController fillB(seqB);
StepController stepA(seqA, fillA);
StepController stepB(seqB, fillB);
RotateController rotateA(seqA);
RotateController rotateB(seqB);

SIGNAL(INT0_vect){
  static uint8_t counter;
  if(isChained()){
    if(clockIsHigh())
      seqA.rise();
    else
      seqA.fall();
    if(counter++ < seqA.length){
      if(seqA.isOn())
	seqB.on();
      else
	seqB.off();
    }else{
      seqB.rise();
      if(counter >= seqA.length+seqB.length)
	counter = 0;
    }
  }else{
    if(clockIsHigh()){
      seqA.rise();
      seqB.rise();
    }else{
      seqA.fall();
      seqB.fall();
    }
    counter = seqA.pos;
  }
  // debug
  PORTB ^= _BV(PORTB4);
}

void setup(){
  cli();
  // define interrupt 0
  EICRA = 1 << ISC00; // trigger int0 on any logical change.
  // pulses that last longer than one clock period will generate an interrupt.
  EIMSK |= (1 << INT0);
//   EICRA = (EICRA & ~((1 << ISC00) | (1 << ISC01))) | (mode << ISC00);
//   EIMSK |= (1 << INT0);
// define interrupt 1
//       EICRA = (EICRA & ~((1 << ISC10) | (1 << ISC11))) | (mode << ISC10);
//       EIMSK |= (1 << INT1);
  SEQUENCER_CLOCK_DDR &= ~_BV(SEQUENCER_CLOCK_PIN);
  SEQUENCER_CLOCK_PORT |= _BV(SEQUENCER_CLOCK_PIN); // enable pull-up resistor

  setup_adc();

  SEQUENCER_CHAINED_SWITCH_DDR  &= ~_BV(SEQUENCER_CHAINED_SWITCH_PIN);
  SEQUENCER_CHAINED_SWITCH_PORT |= _BV(SEQUENCER_CHAINED_SWITCH_PIN);

  sei();

  // debug
  beginSerial(9600);
  printString("hello\n");
//   serialWrite('h');
//   serialWrite('\n');
  DDRB |= _BV(PORTB4);
  PORTB |= _BV(PORTB4);
  PORTB &= ~_BV(PORTB4);
  DDRB |= _BV(PORTB5);
//   PORTB |= _BV(PORTB5);
}

void loop(){
  stepA.update(getAnalogValue(SEQUENCER_STEP_A_CONTROL));
  fillA.update(getAnalogValue(SEQUENCER_FILL_A_CONTROL));
  rotateA.update(getAnalogValue(SEQUENCER_ROTATE_A_CONTROL));

//   stepB.update(getAnalogValue(SEQUENCER_STEP_B_CONTROL));
//   fillB.update(getAnalogValue(SEQUENCER_FILL_B_CONTROL));
//   rotateB.update(getAnalogValue(SEQUENCER_ROTATE_B_CONTROL));

//   if(seqA.isDisabled())
//     seqA.off();
//   if(seqB.isDisabled())
//     seqB.off();

  if(serialAvailable() > 0){
    serialRead();
    printString("rise ");
    if(seqA.isTriggering())
      printString("triggering ");
    if(seqA.isAlternating())
      printString("alternating ");
    if(clockIsHigh())
      printString("clocked ");
    printNewline();
  }

  // debug
  PORTB ^= _BV(PORTB5);
}
