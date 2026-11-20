#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "FVKeyboardAppFrameCore.h"
static int near(double a,double b){return fabs(a-b)<1e-6;}
int main(void){
 FVKARect source={0,0,430,932},target={0,0,430,430*16.0/9};double out;int n=0;
 const double heights[]={301,346,378};
 for(unsigned i=0;i<3;i++){
  double h=heights[i],raw=target.height-(source.height-h);FVKARect keyboard={0,source.height-h,430,h};
  assert(fvKAFNativeIntersection(source,target,keyboard,raw,0,&out));assert(near(out,h));n++;
  assert(!fvKAFNativeIntersection(source,target,keyboard,out,0,&out));assert(near(out,h));n++;
  assert(!fvKAFNativeIntersection(source,target,keyboard,raw,1,&out));assert(near(out,raw));n++;
  assert(!fvKAFNativeIntersection(source,target,keyboard,0,0,&out));assert(out==0);n++;
  assert(!fvKAFNativeIntersection(source,target,keyboard,raw-20,0,&out));assert(near(out,raw-20));n++;
 }
 FVKARect keyboard={0,554,430,378};double raw=target.height-554;
 assert(!fvKAFNativeIntersection(source,source,keyboard,raw,0,&out));n++;
 assert(!fvKAFNativeIntersection(source,target,(FVKARect){0,932,430,378},raw,0,&out));n++;
 assert(!fvKAFNativeIntersection(source,target,(FVKARect){20,554,300,378},raw,0,&out));n++;
 assert(!fvKAFNativeIntersection(source,target,(FVKARect){0,500,430,378},raw,0,&out));n++;
 assert(!fvKAFNativeIntersection(source,target,keyboard,NAN,0,&out));assert(isnan(out));n++;
 assert(!fvKAFNativeIntersection(source,target,keyboard,-1,0,&out));assert(out==-1);n++;
 assert(!fvKAFNativeIntersection(source,target,keyboard,raw,0,0));n++;
 assert(!fvKAFNativeIntersection((FVKARect){0,0,932,430},target,keyboard,raw,0,&out));n++;
 assert(!fvKAFNativeIntersection(source,(FVKARect){0,0,430,700},keyboard,raw,0,&out));n++;
 assert(!fvKAFNativeIntersection(source,target,(FVKARect){0,832,430,100},10,0,&out));n++;
 printf("PASS native keyboard layout: %d strict intersection and unchanged-value cases\n",n);
}
