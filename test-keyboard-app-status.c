#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "FVKeyboardAppStatusCore.h"
int main(void){
 double now=816847000;unsigned n=0;
 for(unsigned stage=1;stage<=4;stage++){assert(fvKAStatusValid(1,123,now-1,stage,now));n++;}
 assert(fvKAStatusValid(1,123,now-120,2,now));n++;
 assert(!fvKAStatusValid(1,123,now-120.01,2,now));n++;
 assert(!fvKAStatusValid(1,123,now+.01,2,now));n++;
 assert(!fvKAStatusValid(2,123,now,2,now));n++;
 assert(!fvKAStatusValid(1,0,now,2,now));n++;
 assert(!fvKAStatusValid(1,-1,now,2,now));n++;
 assert(!fvKAStatusValid(1,12.5,now,2,now));n++;
 assert(!fvKAStatusValid(1,2147483648.0,now,2,now));n++;
 assert(!fvKAStatusValid(1,NAN,now,2,now));n++;
 assert(!fvKAStatusValid(1,123,NAN,2,now));n++;
 assert(!fvKAStatusValid(1,123,now,2,NAN));n++;
 assert(!fvKAStatusValid(1,123,now,0,now));n++;
 assert(!fvKAStatusValid(1,123,now,5,now));n++;
 assert(!fvKAStatusValid(1,123,now,1.5,now));n++;
 assert(!fvKAStatusValid(1,123,now,INFINITY,now));n++;
 assert(!fvKAStatusValid(1,123,-1,2,now));n++;
 printf("PASS app diagnostic receipt: %u stage, identity range and timestamp cases\n",n);
}
