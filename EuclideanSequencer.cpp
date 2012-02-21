#include <inttypes.h>
#include "bjorklund.h"

class Sequence {
public:
  uint32_t bits;
  uint8_t length;
  uint8_t pos;

  void calculate(int fills, int steps){
    Bjorklund algo;
    bits = algo.compute(steps, fills);
    length = steps;
  }

  /* Rotate Left */
  void rol(uint8_t steps){
//     steps = std::min(steps, length);
    bits = (bits << steps) | (bits >> (length-steps));
  }
};

void setup(){
}

void loop(){
}
