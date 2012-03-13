#ifndef _ANALOGREADER_H_
#define _ANALOGREADER_H_

#include <inttypes.h>

#define ADC_CHANNELS 6
#define ADC_OVERSAMPLING 4
#define ADC_VALUE_RANGE (1024*ADC_OVERSAMPLING)

extern uint16_t volatile adc_values[ADC_CHANNELS];

void setup_adc();

// todo: revert hack which inverts values
/* #define getAnalogValue(i) adc_values[i] */
#define getAnalogValue(i) (ADC_VALUE_RANGE-1-adc_values[i])

#endif /* _ANALOGREADER_H_ */
