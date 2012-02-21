#include "bjorklund.h"
#include <inttypes.h>
#include <string.h>

void append(bool value);

/*
  the size of the count and remainder arrays is given by:
  l+1=logτ( m * sqrt(5) )−3
  where τ = (1 + sqrt(5) ) / 2 
  (the Golden Ratio)
  by logarithmic base change rule
  logb(x) = logc(x) / logc(b) 
  we have
  l+1 = ln( m * sqrt(5) ) / ln( (1 + sqrt(5) ) / 2 )
  for m=128, l+1 < 12
*/

int remainder[32];
int count[32];

void build_string(int level) {
  if(level == -1) 
    // append a “0” to the end of the bitmap;
    append(0);
  else if (level == -2) 
    // append a “1” to the end of the bitmap;
    append(1);
  else{ 
    for(int i=0; i < count[level]; i++)
      build_string(level-1); 
    if(remainder[level] != 0)
      build_string(level-2); 
  } /*end else*/
}

void compute_bitmap(int num_slots, int num_pulses){
  /** assuming n ≤ m / 2 */
  /*--------------------- 
   * First, compute the count and remainder arrays 
   */
  int divisor = num_slots - num_pulses;
  remainder[0] = num_pulses;
  int level = 0;
  do{
    count[level] = divisor / remainder[level];
    remainder[level+1] = divisor % remainder[level];
  }while(remainder[++level] > 1);
  count[level] = divisor;
  build_string (level);
}
