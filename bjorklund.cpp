#include "bjorklund.h"

uint32_t Bjorklund::compute(int slots, int pulses){
  bits = 0;
  pos = 0;
  /* Figure 11 */
  int divisor = slots - pulses;
  remainder[0] = pulses; 
  int level = 0; 
  int cycleLength = 1; 
  int remLength = 1;
  do { 
    count[level] = divisor / remainder[level]; 
    remainder[level+1] = divisor % remainder[level]; 
    divisor = remainder[level]; 
    int newLength = (cycleLength * count[level]) + remLength; 
    remLength = cycleLength; 
    cycleLength = newLength; 
    level = level + 1;
  }while(remainder[level] > 1);
  count[level] = divisor; 
  if(remainder[level] > 0)
    cycleLength = (cycleLength * count[level]) + remLength;
  build(level);
  return bits;
}

void Bjorklund::build(int level){
  if(level == -1){
//     pos++;
    bits &= ~(1<<pos++);
  }else if(level == -2){
    bits |= 1<<pos++;
  }else{ 
    for(int i=0; i < count[level]; i++)
      build(level-1); 
    if(remainder[level] != 0)
      build(level-2); 
  }
}
