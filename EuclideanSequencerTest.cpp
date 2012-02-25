/* 
   g++ bjorklund.cpp EuclideanSequencerTest.cpp -o test
*/
#include "bjorklund.h"
#include <inttypes.h>
#include <iostream>

void dump(uint32_t bits, uint8_t length){
  for(uint8_t i=0; i<length; ++i)
    std::cout << ((bits & (1<<i)) ? 'x' : '.');
  std::cout << std::endl;
}

void test(int8_t fills, int8_t steps){
  Bjorklund algo;
  uint32_t bits = algo.compute(steps, fills);
  std::cout << "E(" << (int)fills << ", " << (int)steps << ")\t";
  dump(bits, steps);
//   for(int i=0; i<5; ++i){
//     seq.calculate(fills, steps);
//     seq.rol(i);
//     dump(seq);
//   }
}

int main(int argc, const char* argv[] ){
  int fills = 4;
  int steps = 9;
  if(argc > 1)
    fills = atoi(argv[1]);
  if(argc > 2)
    steps = atoi(argv[2]);
  test(fills, steps);
}


void setup() {
}

void loop() {
}
