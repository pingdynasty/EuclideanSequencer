#ifndef _ANALOGREADER_H_
#define _ANALOGREADER_H_

#include <inttypes.h>

extern uint16_t volatile adc_values[ADC_CHANNELS];

void setup_adc();

// todo: revert hack which inverts values
/* #define getAnalogValue(i) adc_values[i] */
#define getAnalogValue(i) (adc_values[i])

#endif /* _ANALOGREADER_H_ */
