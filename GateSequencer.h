#ifndef _GATE_SEQUENCER_H_
#define _GATE_SEQUENCER_H_

#include "Sequence.h"

class GateSequencer : public Sequence<SEQUENCER_BITS_TYPE> {
public:

  class SequenceController : public DeadbandController<SEQUENCER_DEADBAND_THRESHOLD> {
  public:
    GateSequencer* seq;
    SequenceController(GateSequencer* s) : seq(s) {}
    void hasChanged(uint16_t value){
      seq->recalculate = true;
    }
  };

  class RotateController : public DeadbandController<SEQUENCER_DEADBAND_THRESHOLD> {
  public:
    GateSequencer* seq;
    RotateController(GateSequencer* s) : seq(s) {}
    void hasChanged(uint16_t val){
      val >>= 8; // scale 0-4095 down to 0-15
      seq->rotate(val);
    }
  };

  enum GateSequencerMode {
    DISABLED                   =  0,
    TRIGGERING                 =  1,
    ALTERNATING                =  2
  };

public:
  SequenceController step;
  SequenceController fill;
  RotateController rotation;
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
      uint8_t s = SEQUENCER_STEPS_RANGE - (step.value >> SEQUENCER_STEP_SCALING_FACTOR);
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
    if(isTriggering())
      mode = TRIGGERING;
    else if(isAlternating())
      mode = ALTERNATING;
    else
      mode = DISABLED;
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
    Sequence<SEQUENCER_BITS_TYPE>::reset();
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
  volatile GateSequencerMode mode;

};

#endif /* _GATE_SEQUENCER_H_ */
