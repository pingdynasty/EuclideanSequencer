#include <inttypes.h>

/* Algorithm based on SNS-NOTE-CNTRL-99
   The Theory of Rep-Rate Pattern Generation in the SNS Timing System
   E. Bjorklund
*/

/*
  the size of the count and remainder arrays is given by:
  l+1=logτ( m * sqrt(5) )−3
  where τ = (1 + sqrt(5) ) / 2 
  (the Golden Ratio)
  by logarithmic base change rule logb(x) = logc(x) / logc(b) 
  we have
  l+1 = ln( m * sqrt(5) ) / ln( (1 + sqrt(5) ) / 2 )
  for m=128, l+1 < 12
  for m=32, l+1 < 9
*/

#define BJORKLUND_ARRAY_SIZE 10

/**
   Implementation of the Bjorklund algorithm, 
   capable of calculating up to 32 steps.
 */
class Bjorklund {
public:
  uint32_t compute(int8_t slots, int8_t pulses);

private:
  uint32_t bits;
  uint8_t pos;
  int8_t remainder[BJORKLUND_ARRAY_SIZE];
  int8_t count[BJORKLUND_ARRAY_SIZE];

  void build(int8_t level);
};
