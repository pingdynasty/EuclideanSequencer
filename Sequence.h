#include <inttypes.h>
#include "bjorklund.h"

#ifdef SERIAL_DEBUG
#include "serial.h"
#endif // SERIAL_DEBUG

template<typename T>
class Sequence {
public:
  Sequence() : pos(0), offset(0) {}
  void calculate(int fills){
    Bjorklund<T, 10> algo;
    T newbits;
    newbits = algo.compute(length, fills);
    // rotate 
    if(offset > 0)
      newbits = (newbits >> offset) | (newbits << (length-offset));
    else
      newbits = (newbits << -offset) | (newbits >> (length+offset));
    bits = newbits;
  }
#ifdef SERIAL_DEBUG
  void print(){
    for(int i=0; i<length; ++i)
      serialWrite((bits & (1UL<<i)) ? 'x' : '-');
    printNewline();
  }
#endif
  /* Rotate Left */
  void ror(uint8_t steps){
    bits = (bits << steps) | (bits >> (length-steps));
    offset += steps;
  }
  /* Rotate Right */
  void rol(uint8_t steps){
    bits = (bits >> steps) | (bits << (length-steps));
    offset -= steps;
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
