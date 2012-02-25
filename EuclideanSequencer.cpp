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

class Sequence {
public:
  Sequence() : pos(0), offset(0) {}
  void calculate(int fills){
    Bjorklund algo;
    bits = algo.compute(length, fills);
    printInteger(fills);
    printString(", ");
    printInteger(length);
    printString(" ->\t");
    for(int i=0; i<length; ++i)
      serialWrite((bits & i) ? '1' : '0');
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
//     SEQUENCER_TRIGGER_SWITCH_DDR &= ~trigger;
//     SEQUENCER_TRIGGER_SWITCH_PORT |= trigger;
//     SEQUENCER_ALTERNATE_SWITCH_DDR &= ~_BV(alternate);
//     SEQUENCER_ALTERNATE_SWITCH_PORT |= _BV(alternate);
//     SEQUENCER_OUTPUT_DDR |= output;
    off();
  }
  void rise(){
    // debug
    printString("rise ");
    if(isTriggering())
      printString("triggering ");
    if(isAlternating())
      printString("alternating ");
    if(SEQUENCER_ALTERNATE_SWITCH_PORT & _BV(alternate))
      printString("alt ");
    printNewline();
    //
    if(Sequence::next()){
//       if(isTriggering())
	on();
//       else if(isAlternating())
// 	toggle();
    }
  }
  void fall(){
//     if(!isAlternating())
    printString("fall\n");
//     if(isTriggering())
      off();
  }
  inline void on(){
    SEQUENCER_OUTPUT_PORT |= output;
  }
  inline void off(){
    SEQUENCER_OUTPUT_PORT &= ~output;
  }
  inline void toggle(){
    SEQUENCER_OUTPUT_PORT ^= output;
  }
  inline bool isTriggering(){
    return !(SEQUENCER_TRIGGER_SWITCH_PORT & trigger);
  }
  inline bool isAlternating(){
    return !(SEQUENCER_ALTERNATE_SWITCH_PORT & _BV(alternate));
  }
  inline bool isDisabled(){
    return !(isAlternating() || isTriggering());
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

    printInteger(val);
    printNewline();
  }
};

FillController fillA(seqA);
FillController fillB(seqB);
StepController stepA(seqA, fillA);
StepController stepB(seqB, fillB);
RotateController rotateA(seqA);
RotateController rotateB(seqB);

bool clock = true;
SIGNAL(INT0_vect){
  if(clock){
//   if(SEQUENCER_CLOCK_PORT & SEQUENCER_CLOCK_PIN){
    seqA.rise();
    seqB.rise();
  }else{
    seqA.fall();
    seqB.fall();
  }
  clock = !clock;
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
  SEQUENCER_CLOCK_DDR &= ~SEQUENCER_CLOCK_PIN;
  SEQUENCER_CLOCK_PORT |= SEQUENCER_CLOCK_PIN; // enable pull-up resistor

  SEQUENCER_TRIGGER_SWITCH_DDR &= ~SEQUENCER_TRIGGER_SWITCH_PIN_A;
  SEQUENCER_TRIGGER_SWITCH_PORT |= SEQUENCER_TRIGGER_SWITCH_PIN_A;
  SEQUENCER_ALTERNATE_SWITCH_DDR &= ~_BV(SEQUENCER_ALTERNATE_SWITCH_PIN_A);
  SEQUENCER_ALTERNATE_SWITCH_PORT |= _BV(SEQUENCER_ALTERNATE_SWITCH_PIN_A);
  SEQUENCER_OUTPUT_DDR |= SEQUENCER_OUTPUT_PIN_A;

  SEQUENCER_TRIGGER_SWITCH_DDR &= ~SEQUENCER_TRIGGER_SWITCH_PIN_B;
  SEQUENCER_TRIGGER_SWITCH_PORT |= SEQUENCER_TRIGGER_SWITCH_PIN_B;
  SEQUENCER_ALTERNATE_SWITCH_DDR &= ~_BV(SEQUENCER_ALTERNATE_SWITCH_PIN_B);
  SEQUENCER_ALTERNATE_SWITCH_PORT |= _BV(SEQUENCER_ALTERNATE_SWITCH_PIN_B);
  SEQUENCER_OUTPUT_DDR |= SEQUENCER_OUTPUT_PIN_B;

  setup_adc();

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

  // debug
  PORTB ^= _BV(PORTB5);

//   printInteger(getAnalogValue(SEQUENCER_STEP_A_CONTROL));
//   printString(",\t\t");
//   printInteger(getAnalogValue(SEQUENCER_FILL_A_CONTROL));
//   printString(",\t\t");
//   printInteger(getAnalogValue(SEQUENCER_ROTATE_A_CONTROL));
//   printNewline();

//   printInteger(stepA.value);
//   printString(",\t\t");
//   printInteger(fillA.value);
//   printString(",\t\t");
//   printInteger(rotateA.value);
//   printNewline();

//   if(!(SEQUENCER_CLOCK_PORT & SEQUENCER_CLOCK_PIN))
//     printString("clk\n");
//   if(!clock)
//     printString("clk\n");
//   serialWrite((SEQUENCER_CLOCK_PORT & SEQUENCER_CLOCK_PIN) ? '1' : '0');
//   serialWrite('\n');
}
