#ifndef FV_RATIO_ACTIVATION_CORE_H
#define FV_RATIO_ACTIVATION_CORE_H
static inline int fvRACurrent(const void *host,const void *scene,const void *child,
 const void *liveHost,const void *liveScene,const void *liveChild,const void *parent,
 const void *liveParent,const void *window,const void *liveWindow,int enabled){
 return enabled&&host&&scene&&child&&parent&&window&&host==liveHost&&scene==liveScene&&
  child==liveChild&&parent==liveParent&&window==liveWindow;
}
static inline int fvRAShouldApply(int enabled,int liveGeometry,int parentSimilarity,int hostIdentity,int fullscreen){
 return enabled&&liveGeometry&&parentSimilarity&&hostIdentity&&!fullscreen;
}
#endif
