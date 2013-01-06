#include <inttypes.h>
#include "bjorklund.h"

#ifdef SERIAL_DEBUG
#include "serial.h"
#endif // SERIAL_DEBUG

#define SEQUENCE_ALGORITHM_ARRAY_SIZE 10

template<typename T>
class Sequence {
public:
 Sequence() : pos(0), length(1) {}

  void calculate(uint8_t steps, uint8_t fills){
    Bjorklund<T, SEQUENCE_ALGORITHM_ARRAY_SIZE> algo;
    T newbits;
    newbits = algo.compute(steps, fills);
    length = steps;
    bits = newbits;
  }

#ifdef SERIAL_DEBUG
  void print(){
    for(int i=0; i<length; ++i)
      serialWrite(next() ? 'x' : '-');
//       serialWrite((bits & (1UL<<i)) ? 'x' : '-');
    printNewline();
  }
#endif

  void reset(){
    pos = offset % length;
  }

  void rotate(int8_t steps){
    pos = (length + pos + steps - offset) % length;
    offset = steps;
  }

  bool next(){
    if(pos >= length)
      pos = 0;
    return bits & (1UL << pos++);
  }

// private:
  T bits;
  uint8_t length;
  int8_t offset;
  volatile uint8_t pos;
};
