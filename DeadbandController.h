#ifndef _DEADBAND_CONTROLLER_H_
#define _DEADBAND_CONTROLLER_H_

#include <inttypes.h>

/** Deadband hysteresis */

template<int16_t threshold>
class DeadbandController {
public:
  int16_t value;
  virtual void hasChanged(uint16_t v){}
  void update(uint16_t v){
    int16_t delta = static_cast<int16_t>(v) - static_cast<int16_t>(value);
    if(delta < 0)
      delta = -delta;
    if(delta >= threshold){
      value = v;
      hasChanged(value);
    }
  }
};

#endif /* _DEADBAND_CONTROLLER_H_ */
