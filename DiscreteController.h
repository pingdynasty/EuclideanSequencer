#ifndef _DISCRETE_CONTROLLER_H_
#define _DISCRETE_CONTROLLER_H_

#include "adc_freerunner.h"

class DiscreteController {
public:
  int8_t range;
  int8_t value;
  virtual void hasChanged(int8_t v){}
  void update(uint16_t x){
    uint16_t m = ADC_VALUE_RANGE / range;
    if(x > m)
      x -= m/4;
    else
      x = 0;
    int8_t v = (int8_t)(x/m);
    if(value == v-1 && (x % m) < range/2)
      v = value; // suppress change: reading too close to previous value
    if(value != v){
      value = v;
      hasChanged(value);
    }
  }
};

#endif /* _DISCRETE_CONTROLLER_H_ */
