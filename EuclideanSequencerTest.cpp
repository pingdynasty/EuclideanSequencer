/*
g++ -I../RebelTechnology/Libraries/wiring -I../RebelTechnology/Libraries/avrsim -I/opt/local/include -L/opt/local/lib -o EuclideanSequencerTest -lboost_unit_test_framework  EuclideanSequencerTest.cpp ../RebelTechnology/Libraries/avrsim/avr/io.c ../RebelTechnology/Libraries/wiring/serial.c ../RebelTechnology/Libraries/avrsim/avr/interrupt.c && ./EuclideanSequencerTest
*/

#define SERIAL_DEBUG

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test
#include <boost/test/unit_test.hpp>

#include "EuclideanSequencer.cpp"

struct PinFixture {
  PinFixture() {
    setup();
    PIND |= _BV(PORTD2);
    PIND |= _BV(PORTD3);
    PIND |= _BV(PORTD4);
    PIND |= _BV(PORTD5);
    PIND |= _BV(PORTD6);
    PIND |= _BV(PORTD7);
  }
};

bool outputIsHighA(){
  return !(PINB & _BV(PORTB0));
}

bool outputIsHighB(){
  return !(PINB & _BV(PORTB1));
}

void setChainedMode(bool chain = true){
  if(chain)
    PINB &= ~_BV(PORTB2);
  else
    PINB |= _BV(PORTB2);
}

void setTriggerModeA(){
  PIND |= _BV(PORTD5);
  PIND &= ~_BV(PORTD4);
}

void setTriggerModeB(){
  PIND |= _BV(PORTD7);
  PIND &= ~_BV(PORTD6);
}

void setToggleModeA(){
  PIND |= _BV(PORTD4);
  PIND &= ~_BV(PORTD5);
}

void setToggleModeB(){
  PIND |= _BV(PORTD6);
  PIND &= ~_BV(PORTD7);
}

void setReset(bool high = true){
  if(high)
    PIND &= ~_BV(PORTD2);
  else
    PIND |= _BV(PORTD2);
}

void setClock(bool high = true){
  if(high)
    PIND &= ~_BV(PORTD3);
  else
    PIND |= _BV(PORTD3);
  INT1_vect();
}

void toggleClock(int times = 1){
  for(int i=0; i<times; ++i){
    PIND ^= _BV(PORTD3);
    INT1_vect();
  }
}

void pulseClock(int times = 1){
  for(int i=0; i<times; ++i){
    PIND &= ~_BV(PORTD3); // clock high
    INT1_vect();
    PIND |= _BV(PORTD3); // clock low
    INT1_vect();
  }
}

void setAnalogueValue(int i, float value){
  adc_values[i] = 4095-value*4096;
}

void setRotateA(float value){
  setAnalogueValue(4, value);
}

void setRotateB(float value){
  setAnalogueValue(5, value);
}

void setStepA(float value){
  setAnalogueValue(2, value);
}

void setStepB(float value){
  setAnalogueValue(3, value);
}

void setFillA(float value){
  setAnalogueValue(0, value);
}

void setFillB(float value){
  setAnalogueValue(1, value);
}

struct DefaultFixture {
  PinFixture pins;
  DefaultFixture(){
    setRotateA(0.5);
    setRotateB(0.5);
    setStepA(0.5);
    setStepB(0.5);
    setFillA(0.5);
    setFillB(0.5);
    loop();
  }
};

int countFills(GateSequencer& seq){
  int hits = 0;
  for(int i=0; i<seq.length; ++i){
    if(seq.next())
      hits++;
  }
  return hits;
}

int countHighsA(int clocks){
  int hits = 0;
  for(int i=0; i<clocks; ++i){
    setClock(true);
    if(outputIsHighA())
      hits++;
    setClock(false);
  }
  return hits;
}

int countHighsB(int clocks){
  int hits = 0;
  for(int i=0; i<clocks; ++i){
    setClock(true);
    if(outputIsHighB())
      hits++;
    setClock(false);
  }
  return hits;
}

int firstHighA(int steps){
  int pos = steps;
  for(int i=0; i<steps; ++i){
    setClock(true);
    if(outputIsHighA())
      pos = i;
    setClock(false);
  }
  return pos;
}

int firstHighB(int steps){
  int pos = steps;
  for(int i=0; i<steps; ++i){
    setClock(true);
    if(outputIsHighB())
      pos = i;
    setClock(false);
  }
  return pos;
}

BOOST_AUTO_TEST_CASE(universeInOrder){
    BOOST_CHECK(2+2 == 4);
}

BOOST_AUTO_TEST_CASE(testDefaults){
  PinFixture fixture;
  loop();
  BOOST_CHECK(!resetIsHigh());
  BOOST_CHECK(!clockIsHigh());
}

BOOST_AUTO_TEST_CASE(testSetClock){
  PinFixture fixture;
  setClock(true);
  BOOST_CHECK(clockIsHigh());
  setClock(false);
  BOOST_CHECK(!clockIsHigh());
  setClock(true);
  BOOST_CHECK(clockIsHigh());
}

BOOST_AUTO_TEST_CASE(testOneTriggerSequence){
  PinFixture fixture;
  setTriggerModeA();
  setFillA(0.0);
  setTriggerModeB();
  setFillB(0.0);
  for(float len = 0.0; len < 1.0; len += 0.01){
    setStepA(len);
    setStepB(len);
    loop();
    BOOST_CHECK_EQUAL(countFills(seqA), 1);
    BOOST_CHECK_EQUAL(countFills(seqB), 1);
  }  
}

BOOST_AUTO_TEST_CASE(testOneToggleSequence){
  PinFixture fixture;
  setToggleModeA();
  setFillA(0.0);
  setToggleModeB();
  setFillB(0.0);
  for(float len = 0.0; len < 1.0; len += 0.01){
    setStepA(len);
    setStepB(len);
    loop();
    BOOST_CHECK_EQUAL(countFills(seqA), 1);
    BOOST_CHECK_EQUAL(countFills(seqB), 1);
  }  
}

BOOST_AUTO_TEST_CASE(testAllTriggerSequence){
  PinFixture fixture;
  setTriggerModeA();
  setFillA(1.0);
  setTriggerModeB();
  setFillB(1.0);
  for(float len = 0.0; len < 1.0; len += 0.05){
    setStepA(len);
    setStepB(len);
    loop();
    int steps = seqA.length;
    BOOST_CHECK_EQUAL(seqA.length, (int)(len*16+1));
    BOOST_CHECK_EQUAL(seqA.length, seqB.length);
    BOOST_CHECK_EQUAL(countFills(seqA), steps);
    BOOST_CHECK_EQUAL(countFills(seqB), steps);
    BOOST_CHECK_EQUAL(countHighsA(steps), steps);
    BOOST_CHECK_EQUAL(countHighsB(steps), steps);
  }  
}

BOOST_AUTO_TEST_CASE(testAllToggleSequence){
  PinFixture fixture;
  setToggleModeA();
  setFillA(1.0);
  setToggleModeB();
  setFillB(1.0);
  for(float len = 0.0; len < 1.0; len += 0.05){
    setStepA(len);
    setStepB(len);
    loop();
    int steps = seqA.length;
    BOOST_CHECK_EQUAL(seqA.length, (int)(len*16+1));
    BOOST_CHECK_EQUAL(seqA.length, seqB.length);
    BOOST_CHECK_EQUAL(countHighsA(steps*2), steps);
    BOOST_CHECK_EQUAL(countHighsB(steps*4), steps*2);
  }  
}

BOOST_AUTO_TEST_CASE(testRotateTriggerAndCount){
  PinFixture fixture;
  setTriggerModeA();
  setTriggerModeB();
  for(float len = 0.0; len < 1.0; len += 0.05){
    for(float fill = 0.0; fill < 1.0; fill += 0.05){
      setFillA(fill);
      setFillB(fill);
      setStepA(len);
      setStepB(len);
      loop();
      int steps = seqA.length*2;
      int a = countHighsA(steps);
      int b = countHighsB(steps);
      for(float rot = 0.0; rot < 1.0; rot += 0.05){
	setRotateA(rot);
	setRotateB(rot);
	loop();
	BOOST_CHECK_EQUAL(countHighsA(steps), a);
	BOOST_CHECK_EQUAL(countHighsB(steps), b);
      }
    }  
  }
}

BOOST_AUTO_TEST_CASE(testSimpleChainedMode){  
  PinFixture fixture;
  setTriggerModeA();
  setFillA(0.0);
  setTriggerModeB();
  setFillB(0.0);
  setChainedMode();
  for(float len = 0.0; len < 1.0; len += 0.025){
    setStepA(len);
    setStepB(len);
    loop();
    int steps = (seqA.length + seqB.length)*2;
    BOOST_CHECK_EQUAL(countHighsA(steps), 4);
    BOOST_CHECK_EQUAL(countHighsA(steps), countHighsB(steps));
  }  
}

BOOST_AUTO_TEST_CASE(testComplexChainedMode){  
  PinFixture fixture;
  setTriggerModeA();
  setFillA(0.0);
  setTriggerModeB();
  setFillB(0.0);
  setChainedMode();
  for(float len = 0.0; len < 1.0; len += 0.025){
    setStepA(len);
    setStepB(1.0 - len);
    loop();
    int steps = (seqA.length + seqB.length)*2;
    reset();
    int a = countHighsA(steps);
    reset();
    int b = countHighsB(steps);
    BOOST_CHECK_EQUAL(a, b);
    reset();
    a = firstHighA(steps);
    reset();
    b = firstHighB(steps);
    BOOST_CHECK_EQUAL(a, b);
  }  
}

BOOST_AUTO_TEST_CASE(testRotateToggleAndCount){
  PinFixture fixture;
  setToggleModeA();
  setToggleModeB();
  float fill = 0.0;
  for(float len = 0.0; len < 1.0; len += 0.05){
    setFillA(fill);
    setFillB(fill);
    setStepA(len);
    setStepB(len);
    setRotateA(0.5);
    setRotateB(0.5);
    loop();
    int steps = seqA.length * 2;
    reset();
    int a = countHighsA(steps);
    reset();
    int b = countHighsB(steps);
    for(float rot = 0.0; rot < 1.0; rot += 0.05){
      setRotateA(rot);
      setRotateB(rot);
      loop();
      reset();
      BOOST_CHECK_EQUAL(countHighsA(steps), a);
      reset();
      BOOST_CHECK_EQUAL(countHighsB(steps), b);
    }
  }  
}

// BOOST_AUTO_TEST_CASE(testRotateTriggerAndFindFirst){
//   PinFixture fixture;
//   setTriggerModeA();
//   setTriggerModeB();
//   setFillA(0.0);
//   setFillB(0.0);
//   for(float len = 0.0; len < 1.0; len += 0.05){
//     setStepA(len);
//     setStepB(len);
//     loop();
//     int steps = seqA.length;
//     reset();
//     int a = firstHighA(steps);
//     int b = firstHighB(steps);
//     for(float rot = 0.0; rot < 1.0; rot += 0.05){
//       setRotateA(rot);
//       setRotateB(1.0-rot);
//       loop();
//       reset();
//       int newA = firstHighA(steps);
//       reset();
//       int newB = firstHighB(steps);
// //       if(newA == 0)
// // 	BOOST_CHECK(a == seqA.length - 1);
// //       else
// // 	BOOST_CHECK(newA >= a);
// //       if(newB == seqB.length-1)
// // 	BOOST_CHECK(b == 0);
// //       else
// // 	BOOST_CHECK(newB <= b);
// //       reset();
//       newA = firstHighA(steps);
// //       reset();
//       newB = firstHighB(steps);
//       printString("a ");
//       printInteger(newA);
//       printString(" b ");
//       printInteger(newB);
//       printNewline();
//     }
//   }  
// }
