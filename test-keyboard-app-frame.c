#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "FVKeyboardAppFrameCore.h"
static int near(double a,double b){return fabs(a-b)<1e-6;}
int main(void){
 FVKARect src={0,0,430,932},target={0,0,430,430*16.0/9.0},out;int n=0;
 for(int k=0;k<5;k++){
  double h=301+k*19.25;FVKARect frame={0,932-h,430,h};
  assert(fvKAFMap(src,target,frame,&out));
  assert(near(out.y,target.height-h)&&near(out.height,h));
  assert(near(out.y+out.height,target.height));n++;
  FVKARect once=out;assert(!fvKAFMap(src,target,once,&out));assert(near(out.y,once.y));n++;
 }
 assert(!fvKAFMap(src,target,(FVKARect){20,620,200,312},&out));n++;
 assert(!fvKAFMap(src,target,(FVKARect){0,500,430,301},&out));n++;
 assert(!fvKAFMap(src,target,(FVKARect){0,0,430,932},&out));n++;
 assert(!fvKAFMap(src,target,(FVKARect){0,NAN,430,378},&out));n++;
 assert(!fvKAFMap(src,target,(FVKARect){0,554,430,378},0));n++;
 assert(!fvKAFMap((FVKARect){0,0,932,430},target,(FVKARect){0,230,932,200},&out));n++;
 double now=817000000;
 FVKARect liveSource={0,0,430,932},liveTarget={0,0,430,764.4444444444};
 assert(fvKAFLiveViewportAllowed(1,1,1,1,liveSource,liveTarget));n++;
 assert(!fvKAFLiveViewportAllowed(0,1,1,1,liveSource,liveTarget));n++;
 assert(!fvKAFLiveViewportAllowed(1,0,1,1,liveSource,liveTarget));n++;
 assert(!fvKAFLiveViewportAllowed(1,1,0,1,liveSource,liveTarget));n++;
 assert(!fvKAFLiveViewportAllowed(1,1,1,0,liveSource,liveTarget));n++;
 assert(!fvKAFLiveViewportAllowed(1,1,1,1,liveSource,liveSource));n++;
 FVKARect wrongTarget=liveTarget;wrongTarget.height=700;
 assert(!fvKAFLiveViewportAllowed(1,1,1,1,liveSource,wrongTarget));n++;
 wrongTarget=liveTarget;wrongTarget.x=5;
 assert(!fvKAFLiveViewportAllowed(1,1,1,1,liveSource,wrongTarget));n++;
 assert(fvKAFAuthorize(1,1,1,1,1,now,now-5,target,target));n++;
 for(int field=0;field<5;field++){
  int flags[5]={1,1,1,1,1};flags[field]=0;
  assert(!fvKAFAuthorize(flags[0],flags[1],flags[2],flags[3],flags[4],now,now,target,target));n++;
 }
 assert(fvKAFEventScreenMatch(0,0,0));n++;
 assert(fvKAFEventScreenMatch(1,1,1));n++;
 assert(!fvKAFEventScreenMatch(1,0,0));n++;
 assert(!fvKAFEventScreenMatch(1,1,0));n++;
 assert(!fvKAFEventScreenMatch(1,0,1));n++;
 assert(!fvKAFAuthorize(1,1,1,1,1,now,now-121,target,target));n++;
 assert(!fvKAFAuthorize(1,1,1,1,1,now,now+1,target,target));n++;
 assert(!fvKAFAuthorize(1,1,1,1,1,NAN,now,target,target));n++;
 assert(!fvKAFAuthorize(1,1,1,1,-1,now,now,target,target));n++;
 assert(!fvKAFAuthorize(1,1,1,1,1,now,now,src,target));n++;
 FVKARect shifted=target;shifted.y=10;
 assert(!fvKAFAuthorize(1,1,1,1,1,now,now,shifted,target));n++;
 assert(fvKAFMapBegin(src,target,(FVKARect){0,932,430,378},&out));
 assert(near(out.y,target.height)&&near(out.height,378));n++;
 assert(!fvKAFMap(src,target,(FVKARect){0,932,430,378},&out));n++;
 assert(!fvKAFMapBegin(src,target,(FVKARect){0,932,430,378},0));n++;
 assert(!fvKAFMapBegin(src,target,(FVKARect){30,932,200,378},&out));n++;
 assert(!fvKAFMapBegin(src,target,(FVKARect){0,NAN,430,378},&out));n++;
 printf("PASS app keyboard frame: %d real-event geometry, idempotency and Scene-authorization cases\n",n);
}
