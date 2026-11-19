#ifndef FV_KEYBOARD_MODE_CORE_H
#define FV_KEYBOARD_MODE_CORE_H
#include "FVRatioActivationCore.h"
#include "FVKeyboardAvoidanceCore.h"
static inline const char *fvKAOriginalModeKey(int orientation){
 return orientation==1||orientation==2 ? "wayKeyboardShowIntoFloatingView_1_2" :
        orientation==3||orientation==4 ? "wayKeyboardShowIntoFloatingView_3_4" : 0;
}
static inline int fvKAKeyboardAdaptEnabled(int ratio,FVKAOriginalMode mode,
 int sceneMatch,unsigned targetCount){
 return ratio&&sceneMatch&&targetCount==1&&
  (mode==FVKAOriginalModeInner||mode==FVKAOriginalModeOuter);
}
static inline int fvKATargetViewport(double logicalWidth,double logicalHeight,
 double hostWidth,double ratioEnabled,double *width,double *height,double *scale){
 if(!ratioEnabled||!fvKAFinite(logicalWidth)||!fvKAFinite(logicalHeight)||
    !fvKAFinite(hostWidth)||logicalWidth<=0||logicalHeight<=0||hostWidth<=0||!width||!height||!scale)return 0;
 *width=logicalWidth;*height=logicalHeight;*scale=hostWidth/logicalWidth;
 return fvKAFinite(*scale)&&*scale>0;
}
/* Intersect axis-aligned measurements already expressed in the same space.
 * Output may alias an input. An empty intersection has no visible surface. */
static inline int fvKAClipRect(FVRect a,FVRect b,FVRect *out){
 if(!out)return 0;
 *out=(FVRect){0};
 if(!fvKAValidRect(a)||!fvKAValidRect(b))return 0;
 double x=fvKACoreMax(a.x,b.x),y=fvKACoreMax(a.y,b.y);
 FVRect clipped={x,y,fvKACoreMin(a.x+a.width,b.x+b.width)-x,
  fvKACoreMin(a.y+a.height,b.y+b.height)-y};
 if(!fvKAValidRect(clipped))return 0;
 *out=clipped;return 1;
}
/* The measured surface and host are already in the same, oriented axes.
 * The returned frame is presentation geometry. reservedLogicalHeight is
 * the bottom occlusion to deliver to the owning app for inner presentation. */
typedef struct {
 FVRect keyboard;
 double keyboardScale;
 double reservedLogicalHeight;
} FVKAPlacement;
static inline int fvKAPlanPlacement(int ratio, FVKAOriginalMode mode,
 int sceneMatch, unsigned targetCount, FVRect floatHost, FVRect screen,
 double surfaceWidth, double surfaceHeight, double appScale,
 FVKAPlacement *out){
 if(!out)return 0;
 *out=(FVKAPlacement){0};
 if(!fvKAKeyboardAdaptEnabled(ratio,mode,sceneMatch,targetCount)||
    !fvKAValidRect(floatHost)||!fvKAValidRect(screen)||
    !fvKAFinite(surfaceWidth)||!fvKAFinite(surfaceHeight)||!fvKAFinite(appScale)||
    surfaceWidth<=0||surfaceHeight<=0||appScale<=0)return 0;
 double bottom=floatHost.y+floatHost.height;
 double screenBottom=screen.y+screen.height;
 double availableHeight=mode==FVKAOriginalModeInner ? floatHost.height-1 : screenBottom-bottom;
 if(availableHeight<=0 || floatHost.x<screen.x || floatHost.y<screen.y ||
    floatHost.x+floatHost.width>screen.x+screen.width || bottom>screenBottom)return 0;
 double fit=fvKACoreMin(1,fvKACoreMin(floatHost.width/surfaceWidth,availableHeight/surfaceHeight));
 double width=surfaceWidth*fit,height=surfaceHeight*fit;
 FVRect keyboard={floatHost.x+(floatHost.width-width)/2,
                  mode==FVKAOriginalModeInner ? bottom-height : bottom,width,height};
 double reserved=mode==FVKAOriginalModeInner ? height/appScale : 0;
 if(!fvKAValidRect(keyboard)||!fvKAFinite(fit)||fit<=0||!fvKAFinite(reserved))return 0;
 *out=(FVKAPlacement){keyboard,fit,reserved};
 return 1;
}
#endif
