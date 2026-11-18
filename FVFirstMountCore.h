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
