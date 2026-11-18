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
#ifdef FV_FIRST_MOUNT_MODE
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
 trace("first mount: scene settings accepted before addSubview; client rendering unverified");
 result=1;
 cleanup:
 SEND(parent,"release");SEND(remote,"release");SEND(cached,"release");
 SEND(scene,"release");SEND(state,"release");
 return result;
}
static void fvFMHostAddHook(id self,SEL cmd,id child){
 fvFMPrepare(self,child);
 if(fvFMHostAdd)((void(*)(id,SEL,id))fvFMHostAdd)(self,cmd,child);
 fvGRequest(self);
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
