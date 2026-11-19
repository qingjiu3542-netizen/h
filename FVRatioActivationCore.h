#ifndef FV_RATIO_ACTIVATION_CORE_H
#define FV_RATIO_ACTIVATION_CORE_H
static inline int fvRACurrent(const void *host,const void *scene,const void *child,
 const void *liveHost,const void *liveScene,const void *liveChild,const void *parent,
 const void *liveParent,const void *window,const void *liveWindow,int enabled){
 return enabled&&host&&scene&&child&&parent&&window&&host==liveHost&&scene==liveScene&&
  child==liveChild&&parent==liveParent&&window==liveWindow;
}
typedef enum { FVKAOriginalModeUnknown=-1, FVKAOriginalModeInner=0, FVKAOriginalModeOuter=1 } FVKAOriginalMode;
static inline FVKAOriginalMode fvKAOriginalMode(const char *raw){
 if(!raw)return FVKAOriginalModeUnknown;
 if(raw[0]=='i'&&raw[1]=='n'&&raw[2]=='n'&&raw[3]=='e'&&raw[4]=='r'&&!raw[5])return FVKAOriginalModeInner;
 if(raw[0]=='o'&&raw[1]=='u'&&raw[2]=='t'&&raw[3]=='e'&&raw[4]=='r'&&!raw[5])return FVKAOriginalModeOuter;
 return FVKAOriginalModeUnknown;
}
typedef struct {
 const void *host,*scene,*child,*parent,*window,*token;
} FVRAMount;
static inline int fvRAMountCurrent(FVRAMount saved,FVRAMount live,int enabled,int leaving){
 return enabled&&!leaving&&saved.host&&saved.scene&&saved.child&&saved.parent&&saved.window&&saved.token&&
  saved.host==live.host&&saved.scene==live.scene&&saved.child==live.child&&
  saved.parent==live.parent&&saved.window==live.window&&saved.token==live.token;
}
static inline int fvKAShouldAdapt(FVKAOriginalMode mode,int ratioEnabled,int sourceMatches,int targetCount){
 return ratioEnabled&&sourceMatches&&targetCount==1&&
  (mode==FVKAOriginalModeInner||mode==FVKAOriginalModeOuter);
}
static inline int fvRAShouldApply(int enabled,int liveGeometry,int parentSimilarity,int hostIdentity,int fullscreen){
 return enabled&&liveGeometry&&parentSimilarity&&hostIdentity&&!fullscreen;
}
#endif
