/* 
   g++ bjorklund.cpp EuclideanSequencerTest.cpp -o test
*/
#include "bjorklund.h"
#include <iostream>

void dump(uint32_t bits, int length){
  for(int i=0; i<length; ++i)
    std::cout << ((bits & (1<<i)) ? " x" : " .");
  std::cout << std::endl;
}

void test(int fills, int steps){
  Bjorklund algo;
  uint32_t bits = algo.compute(steps, fills);
  dump(bits, steps);
  std::cout << std::endl;
  std::cout << "E(" << fills << ", " << steps << ")" << std::endl;
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
