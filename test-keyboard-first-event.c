#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "FVKeyboardAppFirstEventCore.h"
int main(void){
 FVKARect source={0,0,430,932},target={0,0,430,430*16.0/9},keys,out;
 unsigned n=0;
 double heights[]={301,346,378};
 for(unsigned i=0;i<3;i++){
  double h=heights[i],small=h-(source.height-target.height);
  FVKARect b={0,0,430,h},input={0,932-small,430,small};
  assert(fvKAFFirstEventKeys(source,target,b,input,&keys));assert(fvKAFNear(keys.height,h));n++;
  assert(fvKAFMap(source,target,keys,&out));assert(fvKAFNear(out.y,target.height-h));n++;
  assert(!fvKAFFirstEventKeys(source,target,b,out,&keys));n++;
  assert(!fvKAFFirstEventKeys(source,target,(FVKARect){0,0,430,small},input,&keys));n++;
  assert(fvKAFFirstEventKeys(source,target,b,input,&keys));
  assert(fvKAFFirstEventBegin(source,target,keys,(FVKARect){0,932,430,small},&out));assert(fvKAFNear(out.y,target.height)&&fvKAFNear(out.height,h));n++;
  assert(!fvKAFFirstEventBegin(source,target,keys,(FVKARect){0,target.height-h,430,h},&out));n++;
  FVKARect full={0,932-h,430,h};assert(fvKAFFirstEventNativeKeys(source,target,full,full,1,&keys));n++;
  assert(!fvKAFFirstEventNativeKeys(source,target,full,full,0,&keys));n++;
 }
 FVKARect b={0,0,430,378},small={0,721.67,430,210.33};
 assert(!fvKAFFirstEventKeys(source,target,(FVKARect){0,0,430,320},small,&keys));n++;
 assert(!fvKAFFirstEventKeys(source,target,(FVKARect){1,0,430,378},small,&keys));n++;
 assert(!fvKAFFirstEventKeys(source,target,(FVKARect){0,1,430,378},small,&keys));n++;
 assert(!fvKAFFirstEventKeys(source,target,(FVKARect){0,0,300,378},small,&keys));n++;
 assert(!fvKAFFirstEventKeys(source,target,(FVKARect){0,0,430,NAN},small,&keys));n++;
 assert(!fvKAFFirstEventKeys(source,target,b,(FVKARect){0,932,430,210.33},&keys));n++;
 assert(!fvKAFFirstEventKeys(source,target,b,(FVKARect){0,700,430,210.33},&keys));n++;
 assert(!fvKAFFirstEventKeys(source,target,b,(FVKARect){0,720,430,212},&keys));n++;
 assert(!fvKAFFirstEventKeys(source,source,b,small,&keys));n++;
 assert(!fvKAFFirstEventKeys(source,(FVKARect){0,0,430,700},b,small,&keys));n++;
 assert(!fvKAFFirstEventKeys(source,target,b,small,0));n++;
 assert(!fvKAFFirstEventNativeKeys(source,target,(FVKARect){0,0,430,320},(FVKARect){0,0,430,378},1,&keys));n++;
 assert(!fvKAFFirstEventNativeKeys(source,target,(FVKARect){0,554,430,378},(FVKARect){0,612,430,320},1,&keys));n++;
 assert(fvKAFFirstEventNativeKeys(source,target,(FVKARect){0,612,430,320},(FVKARect){0,554,430,378},1,&keys));n++;
 assert(!fvKAFFirstEventNativeKeys(source,target,(FVKARect){0,612,430,320},(FVKARect){20,554,400,378},1,&keys));n++;
 assert(!fvKAFFirstEventBegin(source,target,keys,(FVKARect){0,932,430,100},&out));n++;
 assert(!fvKAFFirstEventBegin(source,target,keys,small,0));n++;
 printf("PASS first-event core: %u same-dispatch sizes, reduced-bounds rejection, idempotency and hidden/unknown cases\n",n);
}
