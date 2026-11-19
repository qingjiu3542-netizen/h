#ifndef FV_KEYBOARD_DOCK_CORE_H
#define FV_KEYBOARD_DOCK_CORE_H
#include "FVKeyboardModeCore.h"
/* All source rectangles are in the keyboard surface's unscaled source space.
 * Destination is the target host coordinate space. The full remote canvas is
 * NOT the key region. Preserve uniform scale; dock using the observed keys. */
typedef struct {
 FVRect surface,keys;
 double scale,reservedLogicalHeight;
} FVKeyboardDockPlan;
static inline int fvKeyboardDockPlanWithGap(FVRect sourceSurface,FVRect sourceKeys,
 FVRect host,double appScale,int inner,double bottomGap,FVKeyboardDockPlan *out){
 if(!out)return 0;
 *out=(FVKeyboardDockPlan){0};
 if(!fvKAValidRect(sourceSurface)||!fvKAValidRect(sourceKeys)||!fvKAValidRect(host)||
    !fvKAFinite(appScale)||appScale<=0||!fvKAFinite(bottomGap)||bottomGap<0)return 0;
 const double eps=0.5;
 /* Require a docked, full-width key region. Floating/split keyboards must
  * keep their native path until their separate geometry is implemented. */
 if(__builtin_fabs(sourceKeys.x-sourceSurface.x)>eps||
    __builtin_fabs(sourceKeys.width-sourceSurface.width)>eps||
    sourceKeys.y<sourceSurface.y-eps||
    __builtin_fabs(sourceKeys.y+sourceKeys.height-sourceSurface.y-sourceSurface.height)>eps)return 0;
 double scale=host.width/sourceSurface.width,height=sourceKeys.height*scale;
 if(!fvKAFinite(scale)||scale<=0||!fvKAFinite(height)||height<=0||
    (inner&&height+bottomGap>=host.height-1))return 0;
 double keyTop=inner?host.y+host.height-bottomGap-height:host.y+host.height;
 FVRect surface={host.x,keyTop-(sourceKeys.y-sourceSurface.y)*scale,
  host.width,sourceSurface.height*scale};
 FVRect keys={host.x,keyTop,host.width,height};
 if(!fvKAValidRect(surface)||!fvKAValidRect(keys))return 0;
 double reserve=inner?(height+bottomGap)/appScale:0;
 if(!fvKAFinite(reserve))return 0;
 *out=(FVKeyboardDockPlan){surface,keys,scale,reserve};return 1;
}
/* Backward-compatible zero-gap planner for the original core tests. */
static inline int fvKeyboardDockPlan(FVRect sourceSurface,FVRect sourceKeys,
 FVRect host,double appScale,int inner,FVKeyboardDockPlan *out){
 return fvKeyboardDockPlanWithGap(sourceSurface,sourceKeys,host,appScale,inner,0,out);
}
#endif
