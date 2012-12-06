/*
g++ -g -I../RebelTechnology/Libraries/wiring -I../RebelTechnology/Libraries/avrsim -I/opt/local/include -L/opt/local/lib -o SequenceTest -lboost_unit_test_framework  SequenceTest.cpp ../RebelTechnology/Libraries/avrsim/avr/io.c ../RebelTechnology/Libraries/wiring/serial.c  && ./SequenceTest
*/
#define SERIAL_DEBUG
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Test
#include <boost/test/unit_test.hpp>
#include "Sequence.h"

typedef Sequence<uint32_t> Sequence32;

BOOST_AUTO_TEST_CASE(universeInOrder){
    BOOST_CHECK(2+2 == 4);
}

BOOST_AUTO_TEST_CASE(testDefaults){
  Sequence32 seq;
  BOOST_CHECK_EQUAL(seq.pos, 0);
  BOOST_CHECK_EQUAL(seq.offset, 0);
//   BOOST_CHECK_EQUAL(seq.length, 0);
}

BOOST_AUTO_TEST_CASE(testOneStepOneFillSequence){
  Sequence32 seq;
  seq.length = 1;
  seq.calculate(1);
  int i;
  for(i=0; seq.next() == 1 && i<1000; ++i);
  BOOST_CHECK_EQUAL(i, 1000);  
}

BOOST_AUTO_TEST_CASE(testOneStepNoFillSequence){
  Sequence32 seq;
  seq.length = 1;
  seq.calculate(0);
  int i;
  for(i=0; seq.next() == 0 && i<1000; ++i);
  BOOST_CHECK_EQUAL(i, 1000);  
}

BOOST_AUTO_TEST_CASE(testAllFilled){
  Sequence32 seq;
  for(int n=1; n<32; ++n){
    seq.length = n;
    seq.calculate(n);
    int i;
    for(i=0; seq.next() == 1 && i<100; ++i);
    BOOST_CHECK_EQUAL(i, 100);
  }
}

BOOST_AUTO_TEST_CASE(testNoFills){
  Sequence32 seq;
  for(int n=1; n<32; ++n){
    seq.length = n;
    seq.calculate(0);
    int i;
    for(i=0; seq.next() == 0 && i<100; ++i);
    BOOST_CHECK_EQUAL(i, 100);
  }
}

int countFills(Sequence32 seq){
  int hits = 0;
  for(int i=0; i<seq.length; ++i){
    if(seq.next())
      hits++;
  }
  return hits;
}

void testFill(Sequence32 seq, uint32_t fills){
  for(int n=fills; n<32; ++n){
    seq.length = n;
    seq.calculate(fills);
    int hits = countFills(seq);
    BOOST_CHECK_EQUAL(hits, fills);
  }
}

BOOST_AUTO_TEST_CASE(testOneFills){
  Sequence32 seq;
  testFill(seq, 1);
}

BOOST_AUTO_TEST_CASE(testMoreFills){
  Sequence32 seq;
  for(int n=1; n<32; ++n)
    testFill(seq, n);
}

int getIndex(Sequence32 seq){
  int index;
  for(int i=0; i<seq.length; ++i)
    if(seq.next())
      index = i;
  return index;
}

void testRotate(Sequence32 seq){
  seq.calculate(1);
  int index = getIndex(seq);
  BOOST_CHECK(index < seq.length);
  for(int i=0; i<32; i+=++i){
    seq.rotate(i);
    seq.print();
    int expect = index - i;
    while(expect < 0)
      expect += seq.length;
    BOOST_CHECK_EQUAL(expect, getIndex(seq));
  }
}

BOOST_AUTO_TEST_CASE(testRotateOneFill){
  Sequence32 seq;
  for(int i=1; i<32; ++i){
    seq.length = i;
    testRotate(seq);
  }
}

BOOST_AUTO_TEST_CASE(testRotateAndRecalculate){
  Sequence32 seq;
  seq.length = 24;
  seq.calculate(3);
  seq.rotate(3);
  int index = getIndex(seq);
  seq.calculate(1);
  seq.calculate(3);
  BOOST_CHECK_EQUAL(index, getIndex(seq));
}

BOOST_AUTO_TEST_CASE(testReduceLengthAndRecalculate){
  Sequence32 seq;
  seq.length = 21;
  seq.calculate(3);
  BOOST_CHECK_EQUAL(3, countFills(seq));  
    seq.print();
  seq.length = 19;
  seq.calculate(3);
    seq.print();
  BOOST_CHECK_EQUAL(3, countFills(seq));  
  seq.length = 11;
  seq.calculate(3);
    seq.print();
  BOOST_CHECK_EQUAL(3, countFills(seq));  
  seq.length = 3;
  seq.calculate(15);
    seq.print();
  BOOST_CHECK_EQUAL(3, countFills(seq));  
}
