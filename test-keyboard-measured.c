#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "FVKeyboardAppMeasuredCore.h"
static int near(double a,double b){return fabs(a-b)<1e-6;}
int main(void){
 FVKARect src={0,0,430,932},target={0,0,430,430*16.0/9},out;
 double now=816842874,stamp=now-.8,packet=now-.7;int n=0;
 const double heights[]={346,378,301};
 for(unsigned i=0;i<3;i++){
  double h=heights[i],delta=src.height-target.height;
  FVKARect full={0,932-h,430,h},small={0,932-(h-delta-.1144444444),430,h-delta-.1144444444};
  assert(fvKAFMapMeasured(src,target,full,small,now,stamp,packet,8,&out)==2);
  assert(near(out.height,h)&&near(out.y,target.height-h));n++;
  FVKARect once=out;assert(!fvKAFMapMeasured(src,target,full,once,now,stamp,packet,8,&out));assert(fvKAFSameRect(out,once));n++;
  assert(fvKAFMapMeasured(src,target,full,full,now,stamp,packet,8,&out)==1);assert(near(out.height,h));n++;
  small.y=target.height-small.height;
  assert(fvKAFMapMeasured(src,target,full,small,now,stamp,packet,8,&out)==2);assert(near(out.y,target.height-h));n++;
  small.y=932;
  assert(!fvKAFMapMeasured(src,target,full,small,now,stamp,packet,8,&out));n++;
  assert(fvKAFMapMeasuredBegin(src,target,full,small,now,stamp,packet,8,&out));assert(near(out.y,target.height)&&near(out.height,h));n++;
 }
 FVKARect full={0,554,430,378},small={0,721.67,430,210.33};
 assert(!fvKAFMapMeasured(src,target,full,small,now,now-3.01,packet,8,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,small,now,now+.01,now+.01,8,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,small,now,stamp,stamp-.01,8,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,small,now,stamp,now+.01,8,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,small,now,NAN,packet,8,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,small,NAN,stamp,packet,8,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,small,now,stamp,packet,0,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,small,now,stamp,packet,1.5,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,small,now,stamp,packet,INFINITY,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,(FVKARect){0,720,430,212},now,stamp,packet,8,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,(FVKARect){0,700,430,210.33},now,stamp,packet,8,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,(FVKARect){20,721.67,400,210.33},now,stamp,packet,8,&out));n++;
 assert(!fvKAFMapMeasured(src,target,(FVKARect){0,586,430,346},small,now,stamp,packet,8,&out));n++;
 assert(!fvKAFMapMeasured(src,src,full,small,now,stamp,packet,8,&out));n++;
 assert(!fvKAFMapMeasured(src,(FVKARect){0,0,430,700},full,small,now,stamp,packet,8,&out));n++;
 assert(!fvKAFMapMeasured(src,target,full,small,now,stamp,packet,8,0));n++;
 assert(!fvKAFMapMeasuredBegin(src,target,full,(FVKARect){0,932,430,200},now,stamp,packet,8,&out));n++;
 assert(!fvKAFMapMeasuredBegin(src,target,full,(FVKARect){0,932,430,210.33},now,now-4,packet,8,&out));n++;
 assert(!fvKAFMapMeasuredBegin(src,target,full,small,now,stamp,packet,8,0));n++;
 /* Native query: measured source keys restore the partial intersection only. */
 double repaired;assert(fvKAFNativeIntersection(src,target,full,210.33,0,&repaired));assert(near(repaired,378));n++;
 assert(!fvKAFNativeIntersection(src,target,full,210.33,1,&repaired));n++;
 assert(!fvKAFNativeIntersection(src,target,full,378,0,&repaired));n++;
 assert(!fvKAFNativeIntersection(src,target,full,0,0,&repaired));n++;
 printf("PASS measured keyboard: %d full/reduced rect, exact device samples, freshness, idempotency and hide cases\n",n);
}
