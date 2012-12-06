#include <inttypes.h>
#include "bjorklund.h"

#ifdef SERIAL_DEBUG
#include "serial.h"
#endif // SERIAL_DEBUG

template<typename T>
class Sequence {
public:
  Sequence() : pos(0) {}

  void calculate(int fills){
    Bjorklund<T, 10> algo;
    T newbits;
    newbits = algo.compute(length, fills);
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
    int8_t nudge = (steps - offset) % length;
    if(pos + nudge > length)
      pos = pos + nudge - length;
    else if(pos + nudge < 0)
      pos = pos + nudge + length;
    else
      pos = pos + nudge;
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
