/*
g++ -I../RebelTechnology/Libraries/wiring -I../RebelTechnology/Libraries/avrsim -I/opt/local/include -L/opt/local/lib -o VoltageControlledEuclideanSequencerTest -lboost_unit_test_framework  VoltageControlledEuclideanSequencerTest.cpp ../RebelTechnology/Libraries/avrsim/avr/io.c ../RebelTechnology/Libraries/wiring/serial.c ../RebelTechnology/Libraries/avrsim/avr/interrupt.c && ./VoltageControlledEuclideanSequencerTest
*/
// #define mcu atmega168

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test
#include <boost/test/unit_test.hpp>

#include "VoltageControlledEuclideanSequencer.cpp"

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

void setCountMode(){
  PIND &= ~_BV(PORTD6);
}

void setDelayMode(){
  PIND |= _BV(PORTD6);
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

void setDivide(float value){
  adc_values[0] = ADC_VALUE_RANGE-1-value*1023*4;
}

void setDelay(float value){
  adc_values[1] = ADC_VALUE_RANGE-1-value*1023*4;
}

bool divideIsHigh(){
  return !(PORTB & _BV(PORTB0));
}

bool delayIsHigh(){
  return !(PORTB & _BV(PORTB1));
}

bool combinedIsHigh(){
  return !(PORTB & _BV(PORTB2));
}

struct DefaultFixture {
  PinFixture pins;
  DefaultFixture(){
    loop();
    setDivide(0.0);
    setDelay(0.0);
    loop();
  }
};

BOOST_AUTO_TEST_CASE(universeInOrder)
{
    BOOST_CHECK(2+2 == 4);
}

BOOST_AUTO_TEST_CASE(testDefaults){
  PinFixture fixture;
  loop();
  BOOST_CHECK(!resetIsHigh());
  BOOST_CHECK(!clockIsHigh());
}
