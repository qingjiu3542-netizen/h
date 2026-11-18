#include "FVRestoreCore.h"
typedef struct { id h,state,scene,cached,settings; int mode; Rect original; } FVRestoreContext;
/* mode 0: disabled/rotation, 1: fullscreen, 2: teardown (scene only). */
static int fvGRIdentity(void *v){
 FVRestoreContext *c=v;
 return objc_getAssociatedObject(c->h,&fvGStateKey)==c->state &&
  ivar(c->h,"appScene")==c->scene && fvGIdentical(c->state,"scene",c->scene) &&
  ivar(c->h,"appSetting")==c->cached;
}
static int fvGROrientation(void *v){
 FVRestoreContext *c=v; id s=SEND(c->scene,"settings");
 return fvGHas(s,"interfaceOrientation") &&
  (long)SEND(fvGGet(c->state,"orientation"),"longValue")==
  ((long(*)(id,SEL))objc_msgSend)(s,S("interfaceOrientation"));
}
static int fvGRAvailable(void *v){
 FVRestoreContext *c=v;
 return fvGRect(c->original)&&fvGSettings(c->scene,c->cached,&c->settings);
}
static int fvGRSubmit(void *v){
 FVRestoreContext *c=v;
 if(fvGSameRect(rect(c->settings,"frame"),c->original))return 1;
 id copy=SEND(c->settings,"mutableCopy");
 if(!fvGHas(copy,"setFrame:")){if(copy)SEND(copy,"release");return 0;}
 setrect(copy,"setFrame:",c->original);
 if(!fvGRIdentity(c)||!fvGROrientation(c)){SEND(copy,"release");return 0;}
 SEND(c->scene,"updateSettings:withTransitionContext:completion:",copy,(id)0,(id)0);
 SEND(copy,"release");return 1;
}
static int fvGRAccepted(void *v){
 FVRestoreContext *c=v;id s=SEND(c->scene,"settings");
 return fvGHas(s,"frame")&&fvGSameRect(rect(s,"frame"),c->original);
}
static void fvGRCommit(void *v){
 FVRestoreContext *c=v;
 setrect(c->cached,"setFrame:",c->original);
 id p=SEND(c->h,"superview"),r=ivar(c->h,"hostingView");
 /* Never replay old presentation into replacement views. Outer transforms
  * are left intact; they must not suppress the scene restoration above. */
 if(c->mode!=2 && fvGGet(c->state,"presentationCaptured") &&
    fvGIdentical(c->state,"parent",p)&&fvGIdentical(c->state,"remote",r)&&fvGViews(c->h,p,r)){
  if(c->mode==0){
   setrect(p,"setBounds:",fvGUnboxRect(c->state,"parentBounds"));
   setrect(c->h,"setFrame:",fvGUnboxRect(c->state,"hostFrame"));
  }
  setrect(r,"setBounds:",fvGUnboxRect(c->state,"remoteBounds"));
  fvGSetTransform(r,fvGUnboxTransform(c->state,"remoteTransform"));
  fvGSetCenter(r,fvGUnboxPoint(c->state,"remoteCenter"));
 }
 if(fvGRIdentity(c)){
  objc_setAssociatedObject(c->h,&fvGStateKey,0,1);
  if(c->mode==1)objc_setAssociatedObject(c->h,&fvGFullscreenKey,SEND(objc_getClass("NSNumber"),"numberWithInt:",1),1);
 }
}
static FVRestoreResult fvGRestore(id h,int mode){
 id state=objc_getAssociatedObject(h,&fvGStateKey);
 if(!state)return FVRestoreDone;
 SEND(state,"retain");
 FVRestoreContext c={h,state,ivar(h,"appScene"),ivar(h,"appSetting"),0,mode,fvGUnboxRect(state,"sceneFrame")};
 SEND(c.scene,"retain");SEND(c.cached,"retain");
 id priorBusy=objc_getAssociatedObject(h,&fvGBusyKey);SEND(priorBusy,"retain");
 objc_setAssociatedObject(h,&fvGBusyKey,SEND(objc_getClass("NSNumber"),"numberWithInt:",1),1);
 FVRestoreOps ops={&c,fvGRIdentity,fvGROrientation,fvGRAvailable,fvGRSubmit,fvGRAccepted,fvGRCommit};
 FVRestoreResult result=fvRestoreExecute(&ops);
 if(result!=FVRestoreDone){
  fvGPut(state,"restoreMode",SEND(objc_getClass("NSNumber"),"numberWithInt:",(U)mode));
  fvGPut(state,"restorePending",SEND(objc_getClass("NSNumber"),"numberWithInt:",1));
  fvGPut(state,"restoreResult",SEND(objc_getClass("NSNumber"),"numberWithInt:",(U)result));
  trace(result==FVRestoreOrientation?"restore pending: orientation differs; snapshot retained":
   result==FVRestoreIdentity?"restore blocked: scene identity differs; snapshot retained":
   "restore pending: settings unavailable or unaccepted; snapshot retained");
 }else trace("saved scene frame accepted; client rendering not verified");
 objc_setAssociatedObject(h,&fvGBusyKey,priorBusy,1);SEND(priorBusy,"release");
 SEND(c.cached,"release");SEND(c.scene,"release");SEND(state,"release");return result;
}
/* Retained dictionary contexts isolate queued work from a later scene/session. */
static id fvGWork(id h,id token){
 id work=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
 fvGPut(work,"host",h);fvGPut(work,"token",token);
 id scene=ivar(h,"appScene"),state=objc_getAssociatedObject(h,&fvGStateKey);
 if(scene)fvGPut(work,"scene",scene);if(state)fvGPut(work,"state",state);
 return SEND(work,"retain");
}
static BOOL fvGWorkCurrent(id work,const void *key){
 id h=fvGGet(work,"host");
 return fvRestoreCallbackCurrent(fvGGet(work,"token"),objc_getAssociatedObject(h,key),
  fvGGet(work,"scene"),ivar(h,"appScene"),fvGGet(work,"state"),objc_getAssociatedObject(h,&fvGStateKey));
}
