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
 FVRAMount saved={&h,&s,&c,&p,&w,&h},live=saved;
 assert(fvRAMountCurrent(saved,live,1,0));
 live.token=&other;assert(!fvRAMountCurrent(saved,live,1,0));
 live=saved;live.scene=&other;assert(!fvRAMountCurrent(saved,live,1,0));
 live=saved;live.window=&other;assert(!fvRAMountCurrent(saved,live,1,0));
 assert(!fvRAMountCurrent(saved,saved,0,0));
 assert(!fvRAMountCurrent(saved,saved,1,1));
 assert(fvKAOriginalMode("inner")==FVKAOriginalModeInner);
 assert(fvKAOriginalMode("outer")==FVKAOriginalModeOuter);
 assert(fvKAOriginalMode("native")==FVKAOriginalModeUnknown);
 assert(fvKAOriginalMode(0)==FVKAOriginalModeUnknown);
 assert(fvRANextRetry(0,6)==1);
 assert(fvRANextRetry(5,6)==6);
 assert(fvRANextRetry(6,6)==-1);
 assert(fvRANextRetry(-1,6)==-1);
 assert(fvKAShouldAdapt(FVKAOriginalModeInner,1,1,1));
 assert(fvKAShouldAdapt(FVKAOriginalModeOuter,1,1,1));
 assert(!fvKAShouldAdapt(FVKAOriginalModeUnknown,1,1,1));
 assert(!fvKAShouldAdapt(FVKAOriginalModeInner,0,1,1));
 assert(!fvKAShouldAdapt(FVKAOriginalModeOuter,1,0,1));
 assert(!fvKAShouldAdapt(FVKAOriginalModeInner,1,1,2));
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
 assert(fvKAOriginalMode("inner")==FVKAOriginalModeInner);
 assert(fvKAOriginalMode("outer")==FVKAOriginalModeOuter);
 assert(fvKAOriginalMode("native")==FVKAOriginalModeUnknown);
 assert(fvKAOriginalMode(0)==FVKAOriginalModeUnknown);
 puts("PASS ratio activation: remount identities, original mode values, transforms and fullscreen");
 return 0;
}
