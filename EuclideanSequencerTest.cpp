
#include "EuclideanSequencer.cpp"

#include <iostream>

void dump(Sequence seq){
  for(int i=0; i<seq.length; ++i)
    std::cout << ((seq.bits & (1<<i)) ? " x" : " .");
  std::cout << std::endl;
}

void test(int fills, int steps){
  Sequence seq;
  std::cout << std::endl;
  std::cout << "E(" << fills << ", " << steps << ")" << std::endl;
  for(int i=0; i<5; ++i){
    seq.calculate(fills, steps);
    seq.rol(i);
    dump(seq);
  }
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
