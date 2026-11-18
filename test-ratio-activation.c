#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "FVRatioActivationCore.h"
#include "FVKeyboardAvoidanceCore.h"
int main(void){
 int h=1,s=2,c=3,p=4,w=5,other=6;
 assert(fvRACurrent(&h,&s,&c,&h,&s,&c,&p,&p,&w,&w,1));
 assert(!fvRACurrent(&h,&s,&c,&h,&s,&c,&p,&p,&w,&w,0));
 assert(!fvRACurrent(&h,&s,&c,&other,&s,&c,&p,&p,&w,&w,1));
 assert(!fvRACurrent(&h,&s,&c,&h,&other,&c,&p,&p,&w,&w,1));
 assert(!fvRACurrent(&h,&s,&c,&h,&s,&other,&p,&p,&w,&w,1));
 assert(!fvRACurrent(&h,&s,&c,&h,&s,&c,&p,&other,&w,&w,1));
 assert(!fvRACurrent(&h,&s,&c,&h,&s,&c,&p,&p,&w,&other,1));
 assert(fvRAShouldApply(1,1,1,1,0));
 assert(!fvRAShouldApply(0,1,1,1,0));
 assert(!fvRAShouldApply(1,0,1,1,0));
 assert(!fvRAShouldApply(1,1,0,1,0));
 assert(!fvRAShouldApply(1,1,1,0,0));
 assert(!fvRAShouldApply(1,1,1,1,1));
 assert(fvKAAffineIsSimilarity(1,0,0,1));
 assert(fvKAAffineIsSimilarity(0,1,-1,0));
 assert(fvKAAffineIsSimilarity(.6,0,0,.6));
 assert(fvKAAffineIsSimilarity(0,.5882352941,-.5882352941,0));
 assert(!fvKAAffineIsSimilarity(1,0,0,.9));
 assert(!fvKAAffineIsSimilarity(1,0,.2,1));
 assert(!fvKAAffineIsSimilarity(NAN,0,0,1));
 puts("PASS ratio activation: post-mount identity, original transforms, default and fullscreen");
 return 0;
}
