#ifndef _ANALOGREADER_H_
#define _ANALOGREADER_H_

#include <inttypes.h>

#define ADC_CHANNELS 6
#define ADC_OVERSAMPLING 4

extern uint16_t volatile adc_values[ADC_CHANNELS];

void setup_adc();

#define getAnalogValue(i) adc_values[i]

#endif /* _ANALOGREADER_H_ */
