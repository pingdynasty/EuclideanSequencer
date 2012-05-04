#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "device.h"
#include "bjorklund.h"
#include "adc_freerunner.h"
#include "DiscreteController.h"

// #define SERIAL_DEBUG

#ifdef SERIAL_DEBUG
#include "serial.h"
#endif // SERIAL_DEBUG

/* effective steps range is 1 to 16 */
#define SEQUENCER_STEPS_RANGE       16
#define SEQUENCER_STEPS_MINIMUM     1
/* effective rotation range is -7 to 8 */
#define SEQUENCER_ROTATION_RANGE    16
#define SEQUENCER_ROTATION_MINIMUM  -7

inline bool clockIsHigh(){
  return !(SEQUENCER_CLOCK_PINS & _BV(SEQUENCER_CLOCK_PIN));
}

inline bool resetIsHigh(){
  return !(SEQUENCER_RESET_PINS & _BV(SEQUENCER_RESET_PIN));
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
    Bjorklund<uint16_t, 10> algo;
    uint16_t newbits;
    newbits = algo.compute(length, fills);
    newbits = (newbits << offset) | (newbits >> (length-offset));
    bits = newbits;
  }
#ifdef SERIAL_DEBUG
  void print(){
    for(int i=0; i<length; ++i)
      serialWrite((bits & (1UL<<i)) ? 'x' : '.');
    printNewline();
  }
#endif
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
    return bits & (1UL << pos++);
  }
// private:
  uint16_t bits;
  uint8_t length;
  int8_t offset;
  volatile uint8_t pos;
};

#define TRIGGERING_BIT  1
#define ALTERNATING_BIT 2
#define CHAINED_BIT     3
#define LEADING_BIT     4

enum GateSequencerMode {
  DISABLED                   = 0,
  TRIGGERING                 = _BV(TRIGGERING_BIT),
  ALTERNATING                = _BV(ALTERNATING_BIT),
  TRIGGERING_FOLLOWING       = (_BV(TRIGGERING_BIT)|_BV(CHAINED_BIT)),
  ALTERNATING_FOLLOWING      = (_BV(ALTERNATING_BIT)|_BV(CHAINED_BIT)),
  TRIGGERING_LEADING         = (_BV(TRIGGERING_BIT)|_BV(CHAINED_BIT)|_BV(LEADING_BIT)),
  ALTERNATING_LEADING        = (_BV(ALTERNATING_BIT)|_BV(CHAINED_BIT)|_BV(LEADING_BIT)),
  DISABLED_FOLLOWING         = (_BV(CHAINED_BIT)),
  DISABLED_LEADING           = (_BV(CHAINED_BIT)|_BV(LEADING_BIT))
};

class GateSequencer : public Sequence {
public:
  GateSequencerMode mode;
  GateSequencer* next;
  GateSequencer(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4):
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
    GateSequencerMode newmode;
    if(isChained())
      newmode = (GateSequencerMode)((mode | _BV(CHAINED_BIT)) & (_BV(CHAINED_BIT)|_BV(LEADING_BIT)));
    else
      newmode = DISABLED;
    if(isTriggering())
      newmode = (GateSequencerMode)(newmode | _BV(TRIGGERING_BIT));
    else if(isAlternating())
      newmode = (GateSequencerMode)(newmode | _BV(ALTERNATING_BIT));
    mode = newmode;
  }
  void push(){
    if(isOn())
      next->on();
    else
      next->off();
  }
  bool isFollowing(){
    return !(mode & _BV(LEADING_BIT));
  }
  void follow(){
    mode = (GateSequencerMode)(mode & ~_BV(LEADING_BIT));
    next->mode = (GateSequencerMode)(next->mode | _BV(LEADING_BIT));
  }
  void rise(){
    switch(mode){
    case TRIGGERING:
      if(Sequence::next())
	on();
      break;
    case ALTERNATING:
      if(Sequence::next())
	toggle();
      break;
    case DISABLED:
      Sequence::next();
      off();
      break;
    case TRIGGERING_LEADING:
      if(Sequence::next()){
	on();
	next->on();
      }
      if(pos >= length)
	follow();
      break;
    case ALTERNATING_LEADING:
      if(Sequence::next())
	toggle();
      push();
      if(pos >= length)
	follow();
      break;
    case DISABLED_LEADING:
      Sequence::next();
      off();
      next->off();
      if(pos >= length)
	follow();
      break;
    case DISABLED_FOLLOWING:
    case TRIGGERING_FOLLOWING:
    case ALTERNATING_FOLLOWING:
      break;
    default:
      off();
      break;
    }
  }
  void fall(){
    switch(mode){
    case TRIGGERING_LEADING:
      off();
      next->off();
      break;
    case ALTERNATING:
    case ALTERNATING_LEADING:
    case TRIGGERING_FOLLOWING:
    case ALTERNATING_FOLLOWING:
      break;
    case TRIGGERING:
    default:
      off();
      break;
    }
  }
  void reset(){
    pos = 0;
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
    case TRIGGERING_FOLLOWING:
      printString(" TRIGGERING_FOLLOWING");
      break;
    case ALTERNATING_FOLLOWING:
      printString(" ALTERNATING_FOLLOWING");
      break;
    case TRIGGERING_LEADING:
      printString(" TRIGGERING_LEADING");
      break;
    case ALTERNATING_LEADING:
      printString(" ALTERNATING_LEADING");
      break;
    case DISABLED_FOLLOWING:
      printString(" DISABLED_FOLLOWING");
      break;
    case DISABLED_LEADING:
      printString(" DISABLED_LEADING");
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

GateSequencer seqB(SEQUENCER_OUTPUT_PIN_B, 
		   SEQUENCER_TRIGGER_SWITCH_PIN_B,
		   SEQUENCER_ALTERNATE_SWITCH_PIN_B,
		   SEQUENCER_LED_B_PIN);

class StepController : public DiscreteController {
public:
  Sequence& seq;
  DiscreteController& fills;
  StepController(Sequence& s, DiscreteController& f) : seq(s), fills(f) {
    range = SEQUENCER_STEPS_RANGE;
    value = -1;
  }
  void hasChanged(int8_t steps){
    steps += SEQUENCER_STEPS_MINIMUM;
    seq.length = steps;
    fills.range = steps;
    fills.value = -1; // force fills.hasChanged() to be called
  }
};

class FillController : public DiscreteController {
public:
  Sequence& seq;
  FillController(Sequence& s) : seq(s) {
    range = seq.length;
    value = -1;
  }
  void hasChanged(int8_t val){
    seq.calculate(val+1); // range is 1 to seq.length
  }
};

class RotateController : public DiscreteController {
public:
  Sequence& seq;
  RotateController(Sequence& s) : seq(s) {
    range = SEQUENCER_ROTATION_RANGE;
  }
  void hasChanged(int8_t val){
    val -= SEQUENCER_ROTATION_MINIMUM;
    if(val > seq.offset)
      seq.rol(val-seq.offset);
    else if(val < seq.offset)
      seq.ror(seq.offset-val);
  }
};

/* Reset interrupt */
SIGNAL(INT0_vect){
  seqA.reset();
  seqB.reset();
  seqA.off();
  seqB.off();
  // hold everything until reset is released
  // todo: enable and test
//   while(resetIsHigh());
}

/* Clock interrupt */
SIGNAL(INT1_vect){
  if(clockIsHigh()){
    seqA.rise();
    seqB.rise();
  }else{
    seqA.fall();
    seqB.fall();
  }
  if(clockIsHigh())
    SEQUENCER_LEDS_PORT |= _BV(SEQUENCER_LED_C_PIN);
  else
    SEQUENCER_LEDS_PORT &= ~_BV(SEQUENCER_LED_C_PIN);
  // debug
//   PORTB ^= _BV(PORTB4);
}

void setup(){
  cli();
  // define interrupt 0 and 1
  EICRA = (1<<ISC10) | (1<<ISC01) | (1<<ISC00);
  // trigger int0 on the rising edge.
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

//   SEQUENCER_LEDS_DDR |= _BV(SEQUENCER_LED_A_PIN);
//   SEQUENCER_LEDS_DDR |= _BV(SEQUENCER_LED_B_PIN);
  SEQUENCER_LEDS_DDR |= _BV(SEQUENCER_LED_C_PIN);

  seqA.next = &seqB;
  seqB.next = &seqA;

  sei();

#ifdef SERIAL_DEBUG
  beginSerial(9600);
  printString("hello\n");
#endif
  // debug
//   DDRB |= _BV(PORTB4);
//   PORTB |= _BV(PORTB4);
//   PORTB &= ~_BV(PORTB4);
//   DDRB |= _BV(PORTB5);
//   PORTB |= _BV(PORTB5);
}

FillController fillA(seqA);
StepController stepA(seqA, fillA);
RotateController rotateA(seqA);
FillController fillB(seqB);
StepController stepB(seqB, fillB);
RotateController rotateB(seqB);

void loop(){
  stepA.update(getAnalogValue(SEQUENCER_STEP_A_CONTROL));
  fillA.update(getAnalogValue(SEQUENCER_FILL_A_CONTROL));
  rotateA.update(getAnalogValue(SEQUENCER_ROTATE_A_CONTROL));

  stepB.update(getAnalogValue(SEQUENCER_STEP_B_CONTROL));
  fillB.update(getAnalogValue(SEQUENCER_FILL_B_CONTROL));
  rotateB.update(getAnalogValue(SEQUENCER_ROTATE_B_CONTROL));

  seqA.update();
  seqB.update();

  if(isChained() && seqA.isFollowing() && seqB.isFollowing())
    seqB.follow();

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

  // debug
//   PORTB ^= _BV(PORTB5);
}
