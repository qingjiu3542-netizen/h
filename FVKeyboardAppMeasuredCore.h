#ifndef FV_KEYBOARD_APP_MEASURED_CORE_H
#define FV_KEYBOARD_APP_MEASURED_CORE_H
#include "FVKeyboardAppFrameCore.h"
/* Supplemental publisher evidence, never inferred from a hard-coded keyboard
 * height. Source/target Scene authorization is required separately by callers. */
static inline int fvKAFSameRect(FVKARect a,FVKARect b){
 return fvKAFRect(a)&&fvKAFRect(b)&&fvKAFNear(a.x,b.x)&&fvKAFNear(a.y,b.y)&&
  fvKAFNear(a.width,b.width)&&fvKAFNear(a.height,b.height);
}
static inline int fvKAFMeasuredFresh(FVKARect source,FVKARect target,FVKARect keys,
 double now,double measuredStamp,double packetStamp,double sequence){
 FVKARect mapped;
 return fvKAFLiveViewportAllowed(1,1,1,1,source,target)&&fvKAFMap(source,target,keys,&mapped)&&
  __builtin_isfinite(now)&&now>=0&&__builtin_isfinite(measuredStamp)&&
  __builtin_isfinite(packetStamp)&&__builtin_isfinite(sequence)&&sequence>0&&sequence<9007199254740992.0&&
  __builtin_floor(sequence)==sequence&&now>=measuredStamp&&measuredStamp>=0&&
  now-measuredStamp<=3&&packetStamp>=measuredStamp&&packetStamp<=now;
}
/* Return 1 for a full source rect, 2 for a provably truncated rect, 3 for
 * already-correct target geometry, or 0 for unknown/hidden/split geometry. */
static inline int fvKAFMeasuredShape(FVKARect source,FVKARect target,FVKARect keys,FVKARect input){
 FVKARect mapped;if(!fvKAFMap(source,target,keys,&mapped)||!fvKAFRect(input))return 0;
 if(fvKAFSameRect(input,mapped))return 3;
 if(fvKAFSameRect(input,keys))return 1;
 double delta=source.height-target.height,reduced=keys.height-delta;
 if(delta<=.6||reduced<=.6||!fvKAFNear(input.x,source.x)||!fvKAFNear(input.width,keys.width)||
    !fvKAFNear(input.height,reduced))return 0;
 if(fvKAFNear(input.y+input.height,source.y+source.height)||
    fvKAFNear(input.y+input.height,target.y+target.height))return 2;
 return 0;
}
static inline int fvKAFMapMeasured(FVKARect source,FVKARect target,FVKARect keys,FVKARect input,
 double now,double measuredStamp,double packetStamp,double sequence,FVKARect *out){
 if(!out)return 0;
 *out=input;
 if(!fvKAFMeasuredFresh(source,target,keys,now,measuredStamp,packetStamp,sequence))return 0;
 int shape=fvKAFMeasuredShape(source,target,keys,input);
 if(shape!=1&&shape!=2)return 0;
 if(!fvKAFMap(source,target,keys,out))return 0;
 return shape;
}
static inline int fvKAFMapMeasuredBegin(FVKARect source,FVKARect target,FVKARect keys,FVKARect input,
 double now,double measuredStamp,double packetStamp,double sequence,FVKARect *out){
 int mapped=fvKAFMapMeasured(source,target,keys,input,now,measuredStamp,packetStamp,sequence,out);
 if(mapped)return mapped;
 if(!out||!fvKAFMeasuredFresh(source,target,keys,now,measuredStamp,packetStamp,sequence)||!fvKAFRect(input)||
    !fvKAFNear(input.x,source.x)||!fvKAFNear(input.width,keys.width)||input.y<source.y+source.height-.6)return 0;
 double reduced=keys.height-(source.height-target.height);
 if(!fvKAFNear(input.height,keys.height)&&!(reduced>.6&&fvKAFNear(input.height,reduced)))return 0;
 *out=(FVKARect){target.x,target.y+target.height+input.y-source.y-source.height,keys.width,keys.height};
 return fvKAFRect(*out);
}
#endif
