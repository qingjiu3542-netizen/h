#ifndef FV_KEYBOARD_APP_FIRST_EVENT_CORE_H
#define FV_KEYBOARD_APP_FIRST_EVENT_CORE_H
#include "FVKeyboardAppMeasuredCore.h"
/* Read-only candidates supplied by the SAME synchronous native dispatch.
 * This is not a previous-keyboard cache, an inferred height or a waiting loop. */
static inline int fvKAFFirstEventKeys(FVKARect source,FVKARect target,FVKARect bounds,
 FVKARect incoming,FVKARect *keys){
 if(!keys||!fvKAFLiveViewportAllowed(1,1,1,1,source,target)||!fvKAFRect(bounds)||
    !fvKAFNear(bounds.x,0)||!fvKAFNear(bounds.y,0)||!fvKAFNear(bounds.width,source.width)||
    bounds.height>=target.height-1)return 0;
 FVKARect full={source.x,source.y+source.height-bounds.height,source.width,bounds.height};
 int shape=fvKAFMeasuredShape(source,target,full,incoming);
 /* A bounds value equal to the incoming height may itself be reduced.
  * Never let it override the proven 0.1.18 host measurement path. */
 if(shape!=2)return 0;
 *keys=full;return 1;
}
static inline int fvKAFFirstEventNativeKeys(FVKARect source,FVKARect target,FVKARect frame,
 FVKARect withIAV,int showing,FVKARect *keys){
 if(showing!=1||!keys||!fvKAFLiveViewportAllowed(1,1,1,1,source,target)||
    !fvKAFRect(frame)||!fvKAFRect(withIAV)||withIAV.height+.6<frame.height)return 0;
 FVKARect unused;
 if(!fvKAFMap(source,target,frame,&unused)||!fvKAFMap(source,target,withIAV,&unused))return 0;
 *keys=withIAV;return 1;
}
static inline int fvKAFFirstEventBegin(FVKARect source,FVKARect target,FVKARect keys,FVKARect input,FVKARect *out){
 if(!out)return 0;
 *out=input;
 int shape=fvKAFMeasuredShape(source,target,keys,input);
 if(shape==1||shape==2)return fvKAFMap(source,target,keys,out);
 if(!fvKAFRect(input)||input.y<source.y+source.height-.6||
    !fvKAFNear(input.x,source.x)||!fvKAFNear(input.width,source.width))return 0;
 double reduced=keys.height-(source.height-target.height);
 if(!fvKAFNear(input.height,keys.height)&&!(reduced>.6&&fvKAFNear(input.height,reduced)))return 0;
 FVKARect mapped;if(!fvKAFMap(source,target,keys,&mapped))return 0;
 *out=(FVKARect){target.x,target.y+target.height+input.y-source.y-source.height,keys.width,keys.height};
 return fvKAFRect(*out);
}
#endif
