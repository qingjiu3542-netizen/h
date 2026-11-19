/* Synchronous first-mount preparation. This runs before the original
 * FVAppDispatchHostingView addSubview: implementation, after the original
 * Scene/appSetting/hostingView fields are known to exist in the binary path.
 * It uses only confirmed Objective-C selectors; no Swift-private calls. */
#ifndef FV_FIRST_MOUNT_INCLUDED
#define FV_FIRST_MOUNT_INCLUDED 1
#include "FVFirstMountCore.h"
static IMP fvFMHostAdd;
static int fvFMInstalled;
static BOOL fvFMCurrent(id host,id scene,id cached,id remote,id parent,id state,long orientation){
 if(!fvGHas(scene,"settings"))return 0;
 id live=SEND(scene,"settings");
 if(!fvGHas(live,"interfaceOrientation"))return 0;
 long current=((long(*)(id,SEL))objc_msgSend)(live,S("interfaceOrientation"));
 return fvFirstMountCurrent(scene,ivar(host,"appScene"),cached,ivar(host,"appSetting"),
  remote,ivar(host,"hostingView"),parent,SEND(host,"superview"),
  state,objc_getAssociatedObject(host,&fvGStateKey),orientation,current);
}
static BOOL fvFMEncoding(const char *actual){
 char out[96];unsigned n=0;int depth=0;
 if(!actual)return 0;
 for(;*actual;actual++){
  char c=*actual;
  if(!depth&&c>='0'&&c<='9')continue;
  if(n+1>=sizeof(out))return 0;
  out[n++]=c;
  if(c=='{')depth++;
  else if(c=='}'&&depth>0)depth--;
  else if(c=='}')return 0;
 }
 out[n]=0;
 return depth==0&&!__builtin_strcmp(out,"v@:@");
}
static BOOL fvFMPrepare(id host,id child){
#if defined(FV_FIRST_MOUNT_MODE) && !defined(FV_COMPLETE_MODE)
 if(!fvGMain()||!fvGKind(host,fvGHostClass)||!child)return 0;
#else
 if(!fvGMain()||!enabled()||!fvGKind(host,fvGHostClass)||!child)return 0;
#endif
 /* Only prepare the exact remote view the original host owns; do not touch
  * background/decoration subviews sent through the same selector. */
 if(ivar(host,"hostingView")!=child)return 0;
 /* Never replace an outstanding native baseline on repeated attachment. */
 if(objc_getAssociatedObject(host,&fvGStateKey))return 0;
 id scene=ivar(host,"appScene"),cached=ivar(host,"appSetting");
 if(!scene||!cached||!fvGHas(scene,"settings")||
    !fvGHas(scene,"updateSettings:withTransitionContext:completion:"))return 0;
 id settings=SEND(scene,"settings");
 if(!settings||!fvGHas(settings,"frame")||!fvGHas(settings,"mutableCopy")||
    !fvGHas(settings,"interfaceOrientation"))return 0;
 Rect sf=rect(settings,"frame");if(!fvGRect(sf))return 0;
 long orientation=((long(*)(id,SEL))objc_msgSend)(settings,S("interfaceOrientation"));
 if(orientation<1||orientation>4)return 0;
 Rect target=fvGTargetSceneFrame(sf,orientation);
 if(!fvGRect(target)||fvGSameRect(sf,target))return 0;
 /* Capture a reversible baseline before submitting anything. If the remote
  * surface has not reached a readable geometry yet, defer this experiment. */
 id parent=SEND(host,"superview"),remote=ivar(host,"hostingView");
 if(!fvGViews(host,parent,remote))return 0;
 Rect cb=rect(parent,"bounds");
 Transform identity={1,0,0,1,0,0};
 if(!fvGRect(cb)||!fvGSameTransform(trans(parent),identity)||fvGWindowCoverage(parent,cb)!=0)return 0;
 if(!fvGHas(cached,"frame")||!fvGRect(rect(cached,"frame")))return 0;
 id state=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 fvGIdentity(state,"parent",parent);fvGIdentity(state,"scene",scene);fvGIdentity(state,"remote",remote);
 fvGPut(state,"orientation",SEND(objc_getClass("NSNumber"),"numberWithLong:",(U)orientation));
 /* Original presentation runs after mounting. Capture only the native
  * scene frame here, and defer view snapshots to the presentation pass. */
 fvGPut(state,"sceneFrame",fvGBoxRect(sf));
 objc_setAssociatedObject(host,&fvGStateKey,state,1);
 SEND(state,"retain");SEND(scene,"retain");SEND(cached,"retain");
 SEND(remote,"retain");SEND(parent,"retain");
 BOOL result=0;
 id copy=SEND(settings,"mutableCopy");
 if(!copy||!fvGHas(copy,"setFrame:")){
  if(copy)SEND(copy,"release");
  if(objc_getAssociatedObject(host,&fvGStateKey)==state)objc_setAssociatedObject(host,&fvGStateKey,0,1);
  goto cleanup;
 }
 setrect(copy,"setFrame:",target);
 if(!fvFMCurrent(host,scene,cached,remote,parent,state,orientation)){
  SEND(copy,"release");trace("first mount: identity changed before submission");goto cleanup;
 }
 SEND(scene,"updateSettings:withTransitionContext:completion:",copy,(id)0,(id)0);
 SEND(copy,"release");
 if(!fvFMCurrent(host,scene,cached,remote,parent,state,orientation)){
  trace("first mount: identity changed during submission; cache unchanged");goto cleanup;
 }
 id accepted=SEND(scene,"settings");
 if(!accepted||!fvGHas(accepted,"frame")||!fvGSameRect(rect(accepted,"frame"),target)){
  trace("first mount: scene frame not accepted; baseline retained for safe restore");goto cleanup;
 }
 if(!fvFMCurrent(host,scene,cached,remote,parent,state,orientation)){
  trace("first mount: identity changed during readback; cache unchanged");goto cleanup;
 }
 if(fvGHas(cached,"setFrame:"))setrect(cached,"setFrame:",target);
 trace("first mount diagnostic: scene settings accepted; client rendering unverified");
 result=1;
 cleanup:
 SEND(parent,"release");SEND(remote,"release");SEND(cached,"release");
 SEND(scene,"release");SEND(state,"release");
 return result;
}
#if defined(FV_COMPLETE_MODE)
#include "FVRatioActivationCore.h"
/* A new mount replaces the token, including detach/re-attach of the same
 * objects. Saved window and parent identities prevent writes across sessions. */
static char fvFMMountTokenKey, fvFMMountRetryCountKey, fvFMMountWorkKey;
/* Persistent host-associated metadata must not retain host or its owning
 * parent. Timer work retains live objects only until its bounded callback. */
static void fvFMRememberMount(id host,id child,id scene,id parent,id window){
 id metadata=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 fvGIdentity(metadata,"child",child);fvGIdentity(metadata,"scene",scene);
 fvGIdentity(metadata,"parent",parent);fvGIdentity(metadata,"window",window);
 fvGIdentity(metadata,"token",objc_getAssociatedObject(host,&fvFMMountTokenKey));
 objc_setAssociatedObject(host,&fvFMMountWorkKey,metadata,1);
}
static id fvFMSceneFromWork(id work){
 id value=fvGGet(work,"scene");
 return value==SEND(objc_getClass("NSNull"),"null")?0:value;
}
static void fvFMRetryWork(void *context){
 id work=context,host=fvGGet(work,"host"),token=fvGGet(work,"token");
 id scene=fvFMSceneFromWork(work),child=fvGGet(work,"child");
 FVRAMount saved={.host=host,.scene=scene,.child=child,
  .parent=fvGGet(work,"parent"),.window=fvGGet(work,"window"),.token=token};
 FVRAMount live={.host=host,.scene=ivar(host,"appScene"),.child=ivar(host,"hostingView"),
  .parent=SEND(host,"superview"),.window=SEND(host,"window"),.token=objc_getAssociatedObject(host,&fvFMMountTokenKey)};
 if(!saved.scene&&live.scene){fvGPut(work,"scene",(id)live.scene);saved.scene=live.scene;}
 if(fvRAMountRetryCurrent(saved,live,enabled(),objc_getAssociatedObject(host,&fvGLeavingKey)!=0))fvGRequest(host);
 SEND(work,"release");
}
static void fvFMScheduleRetry(id host){
 if(!host||!enabled()||objc_getAssociatedObject(host,&fvGLeavingKey))return;
 id token=objc_getAssociatedObject(host,&fvFMMountTokenKey),child=ivar(host,"hostingView"),scene=ivar(host,"appScene");
 id parent=SEND(host,"superview"),window=SEND(host,"window");if(!token||!child||!parent||!window)return;
 id old=objc_getAssociatedObject(host,&fvFMMountRetryCountKey);long n=old?(long)SEND(old,"longValue"):0;
 int nextRetry=fvRANextRetry((int)n,6);if(nextRetry<0)return;
 id next=SEND(objc_getClass("NSNumber"),"numberWithLong:",(U)nextRetry);objc_setAssociatedObject(host,&fvFMMountRetryCountKey,next,1);
 id work=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 fvGPut(work,"host",host);fvGPut(work,"child",child);fvGPut(work,"scene",scene?scene:SEND(objc_getClass("NSNull"),"null"));fvGPut(work,"parent",parent);fvGPut(work,"window",window);fvGPut(work,"token",token);
 SEND(work,"retain");dispatch_after_f(dispatch_time(0,100000000LL),&_dispatch_main_q,work,fvFMRetryWork);
}
static void fvFMMarkApplied(id host){
 if(host)objc_setAssociatedObject(host,&fvFMMountRetryCountKey,SEND(objc_getClass("NSNumber"),"numberWithLong:",0),1);
}
static void fvFMMountRetry(void *context){
 id work=context,host=fvGGet(work,"host"),child=fvGGet(work,"child");
 FVRAMount saved={.host=host,.scene=fvFMSceneFromWork(work),.child=child,
  .parent=fvGGet(work,"parent"),.window=fvGGet(work,"window"),.token=fvGGet(work,"token")};
 FVRAMount live={.host=host,.scene=ivar(host,"appScene"),.child=ivar(host,"hostingView"),
  .parent=SEND(host,"superview"),.window=SEND(host,"window"),.token=objc_getAssociatedObject(host,&fvFMMountTokenKey)};
 if(!saved.scene&&live.scene){fvGPut(work,"scene",(id)live.scene);saved.scene=live.scene;}
 if(fvRAMountRetryCurrent(saved,live,enabled(),objc_getAssociatedObject(host,&fvGLeavingKey)!=0)&&
    SEND(child,"superview")==host&&SEND(child,"window")==live.window){
  if(!saved.scene&&live.scene)trace("ratio mount retry: Scene arrived after temporary nil");
  fvGRequest(host);
 }
 SEND(work,"release");
}
static void fvFMRequestMount(id host,id child){
 if(!fvGMain()||!enabled()||!child||ivar(host,"hostingView")!=child||SEND(child,"superview")!=host)return;
 id parent=SEND(host,"superview"),window=SEND(host,"window");if(!parent||!window)return;
 id scene=ivar(host,"appScene"),oldWork=objc_getAssociatedObject(host,&fvFMMountWorkKey);
 id oldToken=objc_getAssociatedObject(host,&fvFMMountTokenKey);
 if(oldWork&&oldToken&&fvGIdentical(oldWork,"token",oldToken)&&fvGIdentical(oldWork,"child",child)&&fvGIdentical(oldWork,"parent",parent)&&fvGIdentical(oldWork,"window",window)){
  id savedScene=SEND(fvGGet(oldWork,"scene"),"pointerValue");
  if((!savedScene&&!scene)||savedScene==scene)return;
  if(!savedScene&&scene){
   id newToken=SEND(objc_getClass("NSObject"),"new");objc_setAssociatedObject(host,&fvFMMountTokenKey,newToken,1);
   id work=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
   fvGPut(work,"host",host);fvGPut(work,"child",child);fvGPut(work,"scene",scene);
   fvGPut(work,"parent",parent);fvGPut(work,"window",window);fvGPut(work,"token",newToken);
   SEND(newToken,"release");fvFMRememberMount(host,child,scene,parent,window);
   fvGRequest(host);const long long delays[]={40000000LL,120000000LL,260000000LL,500000000LL,900000000LL,1400000000LL};
   for(unsigned i=0;i<6;i++)dispatch_after_f(dispatch_time(0,delays[i]),&_dispatch_main_q,SEND(work,"retain"),fvFMMountRetry);
   trace("ratio mount retry: Scene arrived after temporary nil");return;
  }
 }
 id token=SEND(objc_getClass("NSObject"),"new");objc_setAssociatedObject(host,&fvFMMountTokenKey,token,1);
 id work=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 fvGPut(work,"host",host);fvGPut(work,"child",child);
 fvGPut(work,"scene",scene?scene:SEND(objc_getClass("NSNull"),"null"));
 fvGPut(work,"parent",parent);fvGPut(work,"window",window);fvGPut(work,"token",token);
 SEND(token,"release");fvFMRememberMount(host,child,scene,parent,window);
 fvGRequest(host);const long long delays[]={40000000LL,120000000LL,260000000LL,500000000LL,900000000LL,1400000000LL};
 for(unsigned i=0;i<6;i++)dispatch_after_f(dispatch_time(0,delays[i]),&_dispatch_main_q,SEND(work,"retain"),fvFMMountRetry);
 trace("ratio mount: bounded scene and geometry retries armed");
}
#endif
static void fvFMHostAddHook(id self,SEL cmd,id child){
#if !defined(FV_COMPLETE_MODE)
 fvFMPrepare(self,child);
#endif
 /* Complete mode uses one coherent transaction only after attachment. A
  * scene-only prewrite can expose an old client surface at a new viewport. */
 if(fvFMHostAdd)((void(*)(id,SEL,id))fvFMHostAdd)(self,cmd,child);
#if defined(FV_COMPLETE_MODE)
 /* Commit the app host and remote presentation synchronously after the exact
  * child is attached; async layout alone allows the original frame to flash. */
 if(enabled()&&ivar(self,"hostingView")==child&&SEND(child,"superview")==self)fvGApply(self);
 fvFMRequestMount(self,child);
#else
 fvGRequest(self);
#endif
}
static void fvFMInstall(Class host){
 if(fvFMInstalled||!host)return;
 Method m=class_getInstanceMethod(host,S("addSubview:"));
 const char *enc=m?method_getTypeEncoding(m):0;
 if(!m||!fvFMEncoding(enc)){
  trace("first mount hook skipped: addSubview ABI unavailable");return;
 }
 fvGHook(host,"addSubview:",(IMP)fvFMHostAddHook,&fvFMHostAdd);
 fvFMInstalled=1;trace("first mount hook installed");
}
#endif
