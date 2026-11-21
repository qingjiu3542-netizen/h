/* Shared by first-mount runtime adapter and native tests. No ObjC messages.
 * Pointer identity alone does not extend lifetime: runtime must retain the
 * referenced scene, cache and baseline while calling out to scene APIs. */
#ifndef FV_FIRST_MOUNT_CORE_H
#define FV_FIRST_MOUNT_CORE_H
/* Scene dimensions are oriented; host dimensions are handled separately. */
static inline int fvRatioSceneSize(double width,double height,long orientation,
                                  double *outWidth,double *outHeight){
 if(orientation<1||orientation>4||!outWidth||!outHeight||
    !__builtin_isfinite(width)||!__builtin_isfinite(height)||width<=0||height<=0)return 0;
 int landscape=orientation==3||orientation==4;
 if((landscape&&width<height)||(!landscape&&height<width))return 0;
 double edge=landscape?height:width;
 double longEdge=edge*(16.0/9.0);
 if(!__builtin_isfinite(longEdge))return 0;
 *outWidth=landscape?longEdge:edge;
 *outHeight=landscape?edge:longEdge;
 return 1;
}
static inline int fvRatioSceneBottomAligned(double x,double y,double nativeWidth,double nativeHeight,
 long orientation,double *outX,double *outY,double *outWidth,double *outHeight){
 if(!outX||!outY||!fvRatioSceneSize(nativeWidth,nativeHeight,orientation,outWidth,outHeight))return 0;
 /* Preserve the native coordinate space, but align the virtual Scene's
  * bottom edge with the native Scene. This is not a global 320->378 fix. */
 *outX=x;
 *outY=y+(nativeHeight-*outHeight);
 return __builtin_isfinite(*outX)&&__builtin_isfinite(*outY);
}
static int fvRatioPresentationMatches(double parentWidth,double parentHeight,
 double hostWidth,double hostHeight,double remoteWidth,double remoteHeight,
 double centerX,double centerY,double a,double b,double c,double d,
 double targetSceneWidth,double targetSceneHeight,long orientation){
 const double eps=0.5;
 if(orientation<1||orientation>4||!__builtin_isfinite(parentWidth)||
    !__builtin_isfinite(parentHeight)||!__builtin_isfinite(hostWidth)||
    !__builtin_isfinite(hostHeight)||!__builtin_isfinite(remoteWidth)||
    !__builtin_isfinite(remoteHeight)||!__builtin_isfinite(centerX)||
    !__builtin_isfinite(centerY)||!__builtin_isfinite(a)||!__builtin_isfinite(b)||
    !__builtin_isfinite(c)||!__builtin_isfinite(d)||!__builtin_isfinite(targetSceneWidth)||
    !__builtin_isfinite(targetSceneHeight)||parentWidth<=0||targetSceneWidth<=0||targetSceneHeight<=0)return 0;
 double height=parentWidth*16.0/9.0;
 double scale=parentWidth/(orientation>=3?targetSceneHeight:targetSceneWidth);
 double ea=orientation==1?scale:(orientation==2?-scale:0);
 double eb=orientation==3?scale:(orientation==4?-scale:0);
 double ec=-eb,ed=ea;
 return __builtin_fabs(parentHeight-height)<=eps&&__builtin_fabs(hostWidth-parentWidth)<=eps&&
  __builtin_fabs(hostHeight-height)<=eps&&__builtin_fabs(remoteWidth-targetSceneWidth)<=eps&&
  __builtin_fabs(remoteHeight-targetSceneHeight)<=eps&&__builtin_fabs(centerX-parentWidth/2)<=eps&&
  __builtin_fabs(centerY-height/2)<=eps&&__builtin_fabs(a-ea)<=0.001&&
  __builtin_fabs(b-eb)<=0.001&&__builtin_fabs(c-ec)<=0.001&&__builtin_fabs(d-ed)<=0.001;
}
static int fvFirstMountCurrent(
 const void *scene,const void *liveScene,
 const void *cache,const void *liveCache,
 const void *remote,const void *liveRemote,
 const void *parent,const void *liveParent,
 const void *baseline,const void *liveBaseline,
 long orientation,long liveOrientation){
 return scene && cache && remote && parent && baseline &&
  scene==liveScene && cache==liveCache && remote==liveRemote &&
  parent==liveParent && baseline==liveBaseline &&
  orientation>=1 && orientation<=4 && orientation==liveOrientation;
}
#endif
