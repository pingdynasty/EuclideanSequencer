#include "bjorklund.h"

uint32_t Bjorklund::compute(int8_t slots, int8_t pulses){
  bits = 0;
  pos = 0;
  /* Figure 11 */
  int8_t divisor = slots - pulses;
  remainder[0] = pulses; 
  int8_t level = 0; 
  int8_t cycleLength = 1; 
  int8_t remLength = 1;
  do { 
    count[level] = divisor / remainder[level]; 
    remainder[level+1] = divisor % remainder[level]; 
    divisor = remainder[level]; 
    int8_t newLength = (cycleLength * count[level]) + remLength; 
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

void Bjorklund::build(int8_t level){
  if(level == -1){
//     pos++;
    bits &= ~(1UL<<pos++);
  }else if(level == -2){
    bits |= 1UL<<pos++;
  }else{ 
    for(int8_t i=0; i < count[level]; i++)
      build(level-1); 
    if(remainder[level] != 0)
      build(level-2); 
  }
}
