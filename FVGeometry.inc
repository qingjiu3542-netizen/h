/* Experimental geometry replacement for FVAspect.c's oldLayout..install block.
 * Include after ivar(); keep existing settings declarations/helpers below it.
 * Binary evidence: geometry-audit.md, original helper 0x3df0c. This does not
 * call that Swift-only helper or prove that an application will relayout.
 * Only the two named floatingView classes receive ObjC overrides.
 */
extern id objc_getAssociatedObject(id,const void*);
extern void objc_setAssociatedObject(id,const void*,id,U);
static char fvGStateKey, fvGBusyKey, fvGPendingKey, fvGFullscreenKey, fvGLeavingKey;
static Class fvGHostClass, fvGItemClass;
static void fvGRequest(id o);
static void fvKARefresh(void);
static void fvKAObserveItem(id item);
static void fvKADetach(id item);
#ifdef FV_KEYBOARD_DOCK_EXPERIMENT
static void fvKDDropForHost(id host);
#endif
#ifdef FV_COMPLETE_MODE
static void fvFMRequestMount(id host,id child);
static void fvFMMarkApplied(id host);
#endif
static Rect fvKANativeRect(id item,Rect visible,id window);
static IMP fvGHostLayout, fvGHostFrame, fvGHostBounds;
static IMP fvGItemLayout, fvGItemFrame, fvGItemBounds;
static int fvGInstalled, fvGAttempts;

static BOOL fvGHas(id,const char*);
static BOOL fvGHas(id o,const char *s){
 return o&&((BOOL(*)(id,SEL,SEL))objc_msgSend)(o,S("respondsToSelector:"),S(s));
}
static BOOL fvGKind(id o,Class c){
 return o&&c&&((BOOL(*)(id,SEL,Class))objc_msgSend)(o,S("isKindOfClass:"),c);
}
static BOOL fvGMain(void){return (BOOL)(U)SEND(objc_getClass("NSThread"),"isMainThread");}
static BOOL fvGNumber(double x){return __builtin_isfinite(x)&&fabs(x)<1000000.0;}
static BOOL fvGRect(Rect r){
 return fvGNumber(r.origin.x)&&fvGNumber(r.origin.y)&&fvGNumber(r.size.width)&&
  fvGNumber(r.size.height)&&r.size.width>0.0&&r.size.height>0.0;
}
static BOOL fvGPoint(Point p){return fvGNumber(p.x)&&fvGNumber(p.y);}
static BOOL fvGTransform(Transform t){
 return fvGNumber(t.a)&&fvGNumber(t.b)&&fvGNumber(t.c)&&fvGNumber(t.d)&&
  fvGNumber(t.tx)&&fvGNumber(t.ty);
}
static BOOL fvGNear(double a,double b){return fabs(a-b)<0.001;}
static BOOL fvGSameRect(Rect a,Rect b){
 return fvGNear(a.origin.x,b.origin.x)&&fvGNear(a.origin.y,b.origin.y)&&
  fvGNear(a.size.width,b.size.width)&&fvGNear(a.size.height,b.size.height);
}
static BOOL fvGSameTransform(Transform a,Transform b){
 return fvGNear(a.a,b.a)&&fvGNear(a.b,b.b)&&fvGNear(a.c,b.c)&&
  fvGNear(a.d,b.d)&&fvGNear(a.tx,b.tx)&&fvGNear(a.ty,b.ty);
}
static Point fvGCenter(id o){return ((Point(*)(id,SEL))objc_msgSend)(o,S("center"));}
static void fvGSetCenter(id o,Point p){((void(*)(id,SEL,Point))objc_msgSend)(o,S("setCenter:"),p);}
static void fvGSetTransform(id o,Transform t){((void(*)(id,SEL,Transform))objc_msgSend)(o,S("setTransform:"),t);}
static id fvGGet(id d,const char *key){return SEND(d,"objectForKey:",str(key));}
static void fvGPut(id d,const char *key,id value){SEND(d,"setObject:forKey:",value,str(key));}
static id fvGBoxRect(Rect r){return ((id(*)(id,SEL,Rect))objc_msgSend)(objc_getClass("NSValue"),S("valueWithCGRect:"),r);}
static Rect fvGUnboxRect(id d,const char *key){return rect(fvGGet(d,key),"CGRectValue");}
static id fvGBoxPoint(Point p){return ((id(*)(id,SEL,Point))objc_msgSend)(objc_getClass("NSValue"),S("valueWithCGPoint:"),p);}
static Point fvGUnboxPoint(id d,const char *key){return ((Point(*)(id,SEL))objc_msgSend)(fvGGet(d,key),S("CGPointValue"));}
static id fvGBoxTransform(Transform t){return ((id(*)(id,SEL,Transform))objc_msgSend)(objc_getClass("NSValue"),S("valueWithCGAffineTransform:"),t);}
static Transform fvGUnboxTransform(id d,const char *key){return ((Transform(*)(id,SEL))objc_msgSend)(fvGGet(d,key),S("CGAffineTransformValue"));}
/* Identity values avoid retaining the parent (which owns self) or old scenes.
 * They are compared only with live ivars; never messaged/dereferenced. */
static void fvGIdentity(id d,const char *key,id o){fvGPut(d,key,SEND(objc_getClass("NSValue"),"valueWithPointer:",o));}
static BOOL fvGIdentical(id d,const char *key,id o){return SEND(fvGGet(d,key),"pointerValue")==o;}
static id fvGTarget(id o){
 if(fvGKind(o,fvGHostClass))return o;
 if(fvGKind(o,fvGItemClass)){
  id h=ivar(o,"hostingView");
  if(fvGKind(h,fvGHostClass)&&SEND(h,"superview")==o)return h;
 }
 return 0;
}
static BOOL fvGViews(id h,id p,id r){
 return fvGKind(h,fvGHostClass)&&fvGKind(p,fvGItemClass)&&
  ivar(p,"hostingView")==h&&SEND(h,"superview")==p&&r&&r!=h&&r!=p&&
  fvGHas(p,"bounds")&&fvGHas(p,"setBounds:")&&
  fvGHas(h,"frame")&&fvGHas(h,"setFrame:")&&fvGHas(h,"transform")&&
  fvGHas(r,"bounds")&&fvGHas(r,"setBounds:")&&fvGHas(r,"center")&&
  fvGHas(r,"setCenter:")&&fvGHas(r,"transform")&&fvGHas(r,"setTransform:");
}
/* appSetting is the mutable ivar reused by the original foreground path.
 * Require its frame setter instead of replacing a Swift strong ivar using
 * object_setIvar (which would require proven ownership semantics). */
static BOOL fvGSettings(id scene,id cached,id *settings){
 if(!fvGHas(scene,"settings")||!fvGHas(scene,"updateSettings:withTransitionContext:completion:")||
    !fvGHas(cached,"frame")||!fvGHas(cached,"setFrame:"))return 0;
 *settings=SEND(scene,"settings");
 return fvGHas(*settings,"frame")&&fvGHas(*settings,"interfaceOrientation")&&fvGHas(*settings,"mutableCopy");
}
#include "FVFirstMountCore.h"
static Rect fvGTargetSceneFrame(Rect nativeFrame,long orientation);
static BOOL fvGFirstPresentationMatches(id h,id scene,id cached,id r,id p,id state,long orientation){
 if(!fvGMain()||!h||!scene||!cached||!r||!p||!state||
    objc_getAssociatedObject(h,&fvGStateKey)!=state||ivar(h,"appScene")!=scene||
    ivar(h,"appSetting")!=cached||ivar(h,"hostingView")!=r||SEND(h,"superview")!=p||
    SEND(r,"superview")!=h||!fvGHas(scene,"settings"))return 0;
 Rect sf=rect(SEND(scene,"settings"),"frame"),cache=rect(cached,"frame");
 Rect native=fvGUnboxRect(state,"sceneFrame"),target=fvGTargetSceneFrame(native,orientation);
 Rect pb=rect(p,"bounds"),hf=rect(h,"frame"),rb=rect(r,"bounds");
 Transform t=trans(r);Point center=fvGCenter(r);
 return fvGSameRect(sf,target)&&fvGSameRect(cache,target)&&
  fvRatioPresentationMatches(pb.size.width,pb.size.height,hf.size.width,hf.size.height,
   rb.size.width,rb.size.height,center.x,center.y,t.a,t.b,t.c,t.d,
   target.size.width,target.size.height,orientation);
}
static BOOL fvGSceneFrame(id scene,id settings,id cached,Rect frame){
 BOOL changed=!fvGSameRect(rect(settings,"frame"),frame);
 id copy=changed?SEND(settings,"mutableCopy"):0;
 if(changed&&!fvGHas(copy,"setFrame:")){if(copy)SEND(copy,"release");return 0;}
 if(changed){
  setrect(copy,"setFrame:",frame);
  SEND(scene,"updateSettings:withTransitionContext:completion:",copy,(id)0,(id)0);
  SEND(copy,"release");
 }
 /* Read back scene settings before changing the local presentation. This
  * confirms only server-side settings, not the client's rendered surface. */
 id accepted=SEND(scene,"settings");
 if(!fvGHas(accepted,"frame")||!fvGSameRect(rect(accepted,"frame"),frame)){
  trace("scene frame not yet accepted; keeping local geometry unchanged");
  return 0;
 }
 if(!fvGSameRect(rect(cached,"frame"),frame))setrect(cached,"setFrame:",frame);
 return 1;
}
typedef struct {Size sceneSize;Size hostSize;double visualScale;} FVVirtualGeometry;
#include "FVFirstMountCore.h"
static Rect fvGTargetSceneFrame(Rect nativeFrame,long orientation){
 Size result={0};
 fvRatioSceneSize(nativeFrame.size.width,nativeFrame.size.height,orientation,
                  &result.width,&result.height);
 nativeFrame.size=result;
 return nativeFrame;
}
static BOOL fvGComputeVirtual(double baseWidth,double hostWidth,double ratio,FVVirtualGeometry*out){
 if(!out||!fvGNumber(baseWidth)||!fvGNumber(hostWidth)||!fvGNumber(ratio)||baseWidth<=0||hostWidth<=0||ratio<=0)return 0;
 double sh=baseWidth/ratio,hh=hostWidth/ratio,scale=hostWidth/baseWidth;
 if(!fvGNumber(sh)||!fvGNumber(hh)||!fvGNumber(scale)||sh<=0||hh<=0||scale<=0)return 0;
 out->sceneSize=(Size){baseWidth,sh};out->hostSize=(Size){hostWidth,hh};out->visualScale=scale;return 1;
}
#include "FVRestore.inc"
/* Returns -1 when window coverage cannot be measured. */
static int fvGWindowCoverage(id item,Rect itemBounds){
 id window=SEND(item,"window");
 if(!fvGHas(window,"bounds")||!fvGHas(item,"convertRect:toView:"))return -1;
 Rect windowBounds=rect(window,"bounds");
 Rect visible=((Rect(*)(id,SEL,Rect,id))objc_msgSend)(item,S("convertRect:toView:"),itemBounds,window);
 visible=fvKANativeRect(item,visible,window);
 if(!fvGRect(windowBounds)||!fvGRect(visible))return -1;
 double insetX=windowBounds.size.width*0.05,insetY=windowBounds.size.height*0.05;
 return visible.origin.x<=windowBounds.origin.x+insetX&&
  visible.origin.y<=windowBounds.origin.y+insetY&&
  visible.origin.x+visible.size.width>=windowBounds.origin.x+windowBounds.size.width-insetX&&
  visible.origin.y+visible.size.height>=windowBounds.origin.y+windowBounds.size.height-insetY;
}
#include "FVKeyboardAvoidanceCore.h"
static void fvGDecision(unsigned code,const char *message){
 static unsigned reported;
 unsigned bit=1u<<code;
 if(!(reported&bit)){reported|=bit;trace(message);}
}
#ifdef FV_COMPLETE_MODE
static void fvFMScheduleRetry(id host);
#endif
#include "FVPresentationCommitCore.h"
typedef struct {
 id host,scene,cached,remote,parent,state;
 long orientation;
 Rect parentBounds,hostFrame,remoteBounds;
 Transform remoteTransform;
 Point remoteCenter;
} FVPresentationContext;
static int fvGPCurrent(void *v){
 FVPresentationContext *c=v;
 if(!enabled()||objc_getAssociatedObject(c->host,&fvGLeavingKey)||
    objc_getAssociatedObject(c->host,&fvGStateKey)!=c->state||
    ivar(c->host,"appScene")!=c->scene||ivar(c->host,"appSetting")!=c->cached||
    ivar(c->host,"hostingView")!=c->remote||SEND(c->host,"superview")!=c->parent||
    SEND(c->remote,"superview")!=c->host||!fvGViews(c->host,c->parent,c->remote))return 0;
 id window=SEND(c->host,"window");
 if(!window||SEND(c->remote,"window")!=window)return 0;
 id settings=SEND(c->scene,"settings");
 return fvGHas(settings,"interfaceOrientation")&&
  ((long(*)(id,SEL))objc_msgSend)(settings,S("interfaceOrientation"))==c->orientation;
}
static int fvGPMatches(void *v,unsigned field){
 FVPresentationContext *c=v;
 switch(field){
 case 0:return fvGSameRect(rect(c->parent,"bounds"),c->parentBounds);
 case 1:return fvGSameRect(rect(c->host,"frame"),c->hostFrame);
 case 2:return fvGSameRect(rect(c->remote,"bounds"),c->remoteBounds);
 case 3:return fvGSameTransform(trans(c->remote),c->remoteTransform);
 case 4:{Point p=fvGCenter(c->remote);return fvGNear(p.x,c->remoteCenter.x)&&fvGNear(p.y,c->remoteCenter.y);}
 default:return 0;
 }
}
static void fvGPAssign(void *v,unsigned field){
 FVPresentationContext *c=v;
 switch(field){
 case 0:setrect(c->parent,"setBounds:",c->parentBounds);break;
 case 1:setrect(c->host,"setFrame:",c->hostFrame);break;
 case 2:setrect(c->remote,"setBounds:",c->remoteBounds);break;
 case 3:fvGSetTransform(c->remote,c->remoteTransform);break;
 case 4:fvGSetCenter(c->remote,c->remoteCenter);break;
 }
}
static void fvGApply(id h){
 if(!fvGMain()||objc_getAssociatedObject(h,&fvGBusyKey))return;
 id p=SEND(h,"superview"),scene=ivar(h,"appScene"),r=ivar(h,"hostingView"),cached=ivar(h,"appSetting");
 id state=objc_getAssociatedObject(h,&fvGStateKey),settings=0;
 int active=enabled();
#ifdef FV_KEYBOARD_DOCK_EXPERIMENT
 if(!active)fvKDDropForHost(h);
#endif
 /* A replacement Scene owns its own native baseline. Never send the old
  * snapshot to it, or keep retrying an identity check that cannot succeed. */
 if(state&&!scene)return; /* A temporarily empty ivar is not a new Scene. */
 if(state&&!fvGIdentical(state,"scene",scene)){
  objc_setAssociatedObject(h,&fvGStateKey,0,1);state=0;
  objc_setAssociatedObject(h,&fvGFullscreenKey,0,1);
  trace("ratio 0.1.4: replaced scene; retired old snapshot without replay");
 }
 BOOL rebound=state&&(!fvGIdentical(state,"parent",p)||!fvGIdentical(state,"remote",r));
 if(state&&(!active||fvGGet(state,"restorePending"))){
  int mode=fvGGet(state,"restorePending")?(int)(long)SEND(fvGGet(state,"restoreMode"),"longValue"):0;
  if(rebound)mode=2;
  if(fvGRestore(h,mode)==FVRestoreDone&&active)fvGDecision(5,"ratio 0.1.4: restored before next mount pass");
  return;
 }
 if(!active){fvGDecision(0,"ratio 0.1.2: preference off in SpringBoard");return;}
 if(!fvGViews(h,p,r)){fvGDecision(1,"ratio 0.1.2: waiting for item/host/remote");return;}
 if(SEND(r,"superview")!=h||!SEND(h,"window")||SEND(r,"window")!=SEND(h,"window")){
  fvGDecision(18,"ratio: waiting for exact remote attachment and window");return;
 }
 if(!fvGSettings(scene,cached,&settings)){fvGDecision(2,"ratio 0.1.2: scene settings unavailable");return;}
 Rect pb=rect(p,"bounds"),hf=rect(h,"frame"),rb=rect(r,"bounds"),sf=rect(settings,"frame");
 Transform ht=trans(h),rt=trans(r);Transform pt=trans(p);Point rc=fvGCenter(r);
 if(!fvGRect(pb)||!fvGRect(hf)||!fvGRect(rb)||!fvGRect(sf)||
    !fvGTransform(ht)||!fvGTransform(rt)||!fvGTransform(pt)||!fvGPoint(rc)){
  fvGDecision(4,"ratio 0.1.2: invalid geometry values");return;
 }
 /* The original keyboard path scales/moves FVAppDispatchItem (the parent),
  * not the remote scene host. Keep that state instead of rejecting it. */
 /* The item bounds are local coordinates, even when the item is scaled
  * or rotated. The host frame, however, requires an untransformed host. */
 Transform identity={1,0,0,1,0,0};
 if(!fvKAAffineIsSimilarity(pt.a,pt.b,pt.c,pt.d)||!fvGSameTransform(ht,identity)){
  fvGDecision(3,"ratio 0.1.2: unsupported item/host transform");return;
 }
 long sceneOrientation=((long(*)(id,SEL))objc_msgSend)(settings,S("interfaceOrientation"));
 /* Device orientation and scene orientation are separate contracts. A phone
  * may be physically landscape while this app scene remains portrait; that
  * case still needs a portrait logical scene rendered into a 9:16 host. */
 id device=SEND(objc_getClass("UIDevice"),"currentDevice");
 long deviceOrientation=device&&fvGHas(device,"orientation")?
   ((long(*)(id,SEL))objc_msgSend)(device,S("orientation")):1;
 BOOL deviceLandscape=deviceOrientation==3||deviceOrientation==4;
 long orientation=sceneOrientation;
 objc_setAssociatedObject(h,&fvGBusyKey,SEND(objc_getClass("NSNumber"),"numberWithInt:",1),1);
 if(orientation<1||orientation>4){fvGDecision(6,"ratio 0.1.4: interface orientation unavailable");goto retry;}
 BOOL landscape=orientation==3||orientation==4;
 /* A landscape device or landscape scene does not cancel the virtual
  * portrait scene. The scene orientation is retained in the transform below;
  * the logical scene remains 9:16 and the host remains independently sized. */
 (void)deviceLandscape;
 int coverage=fvGWindowCoverage(p,pb);
 if(coverage<0){fvGDecision(7,"ratio 0.1.4: fullscreen coverage unavailable");goto retry;}
 if(coverage){
#ifdef FV_KEYBOARD_DOCK_EXPERIMENT
  fvKDDropForHost(h);
#endif
  if(!state){
   objc_setAssociatedObject(h,&fvGFullscreenKey,SEND(objc_getClass("NSNumber"),"numberWithInt:",1),1);
   goto done;
  }
  fvGRestore(h,1);goto done;
 }
 if(state&&(long)SEND(fvGGet(state,"orientation"),"longValue")!=orientation){
  fvGRestore(h,0);goto done;
 }
 if(objc_getAssociatedObject(h,&fvGFullscreenKey))
  objc_setAssociatedObject(h,&fvGFullscreenKey,0,1);
 /* Read the native logical width once, then keep a separate virtual Scene
  * size and visual scale. The outer item width is presentation geometry. */
 /* The scene frame may itself be landscape. Its short edge is the
  * portrait logical width; the long edge is produced by the 16:9 ratio. */
 Rect nativeFrame=state?fvGUnboxRect(state,"sceneFrame"):sf;
 if(!fvGRect(nativeFrame)){fvGDecision(8,"ratio 0.1.4: native Scene frame unavailable");goto retry;}
 Rect targetScene=fvGTargetSceneFrame(nativeFrame,orientation);
 if(!fvGRect(targetScene)){fvGDecision(9,"ratio 0.1.4: target Scene frame rejected");goto retry;}
 double baseWidth=landscape?targetScene.size.height:targetScene.size.width;
 double displayWidth=pb.size.width;
 FVVirtualGeometry vg={0};
 if(!fvGComputeVirtual(baseWidth,displayWidth,9.0/16.0,&vg))goto retry;
 double displayHeight=vg.hostSize.height;
 double scale=vg.visualScale;
 if(!fvGNumber(scale)||scale<=0||!fvGNumber(displayHeight)){fvGDecision(10,"ratio 0.1.4: display geometry rejected");goto retry;}
 /* Preserve the original helper's rotation. Accept only an existing uniform,
  * orthogonal, orientation-compatible transform; wait for original geometry
  * when orientation and remote transform disagree. Translation is retained. */
 double norm=sqrt(rt.a*rt.a+rt.b*rt.b);
 if(!fvGNumber(norm)||norm<0.000001){fvGDecision(11,"ratio 0.1.4: remote transform unavailable");goto retry;}
 Transform rotation=rt;rotation.a/=norm;rotation.b/=norm;rotation.c/=norm;rotation.d/=norm;
 if(fabs(rotation.a*rotation.c+rotation.b*rotation.d)>0.001||
    fabs(rotation.c*rotation.c+rotation.d*rotation.d-1)>0.001||
    fabs(rotation.a*rotation.d-rotation.b*rotation.c-1)>0.001)goto done;
 if(landscape?(fabs(rotation.a)>0.001||fabs(rotation.d)>0.001):
              (fabs(rotation.b)>0.001||fabs(rotation.c)>0.001)){
  fvGDecision(12,"ratio 0.1.4: waiting for native rotation");goto retry;
 }
 /* Original arm64 helper 0x3df0c uses table 0x7fc68: 0, pi,
  * +pi/2, -pi/2 for interface orientations 1..4. Do not retain a stale
  * sign when rotating between two landscape or two portrait orientations. */
 rotation.a=orientation==1?1:(orientation==2?-1:0);
 rotation.b=orientation==3?1:(orientation==4?-1:0);
 rotation.c=-rotation.b;rotation.d=rotation.a;
 rotation.tx=0;rotation.ty=0;
 Rect targetParent=pb;
 targetParent.size.height=displayHeight;
 Rect targetHost={{hf.origin.x,hf.origin.y},{displayWidth,displayHeight}};
 /* A landscape scene is rotated into the portrait floating host. The
  * remote bounds must therefore be the pre-rotation dimensions; using the
  * portrait logical size here makes the rotated visible box 16:9 instead
  * of the host's 9:16 box. */
 Rect targetRemote=rb;
 targetRemote.size=targetScene.size;
 Transform targetTransform=rotation;
 targetTransform.a*=scale;targetTransform.b*=scale;targetTransform.c*=scale;targetTransform.d*=scale;
 Point targetCenter={displayWidth/2.0,displayHeight/2.0};
 if(!state){
  if(!fvGRect(rect(cached,"frame")))goto done;
  state=SEND(objc_getClass("NSMutableDictionary"),"dictionary");
  fvGIdentity(state,"parent",p);fvGIdentity(state,"scene",scene);fvGIdentity(state,"remote",r);
  fvGPut(state,"orientation",SEND(objc_getClass("NSNumber"),"numberWithLong:",(U)orientation));
  fvGPut(state,"sceneFrame",fvGBoxRect(sf));
  objc_setAssociatedObject(h,&fvGStateKey,state,1);
 }
 if(!fvGSceneFrame(scene,settings,cached,targetScene)){
  fvGDecision(13,"ratio 0.1.4: Scene frame not accepted; retry pending");goto retry;
 }
 if(objc_getAssociatedObject(h,&fvGStateKey)!=state||ivar(h,"appScene")!=scene||
    ivar(h,"appSetting")!=cached||ivar(h,"hostingView")!=r||SEND(h,"superview")!=p){
  fvGDecision(14,"ratio 0.1.4: mount identity changed during layout");goto retry;
 }
 if(!fvGGet(state,"presentationCaptured")){
  fvGPut(state,"parentBounds",fvGBoxRect(pb));
  fvGPut(state,"hostFrame",fvGBoxRect(hf));fvGPut(state,"remoteBounds",fvGBoxRect(rb));
  fvGPut(state,"remoteTransform",fvGBoxTransform(rt));fvGPut(state,"remoteCenter",fvGBoxPoint(rc));
  fvGPut(state,"presentationCaptured",SEND(objc_getClass("NSNumber"),"numberWithInt:",1));
 }
 /* Re-read after each preceding setter: native layout may have reset remote
  * bounds/transform since the original samples. Never use those stale samples
  * to skip a write. Keep live objects retained through reentrant callbacks. */
 FVPresentationContext presentation={h,scene,cached,r,p,state,orientation,
  targetParent,targetHost,targetRemote,targetTransform,targetCenter};
 SEND(h,"retain");SEND(scene,"retain");SEND(cached,"retain");SEND(r,"retain");SEND(p,"retain");SEND(state,"retain");
 FVPresentationOps ops={&presentation,fvGPCurrent,fvGPMatches,fvGPAssign};
 int committed=fvPresentationCommit(&ops);
#ifdef FV_COMPLETE_MODE
 if(committed)committed=fvGFirstPresentationMatches(h,scene,cached,r,p,state,orientation);
#endif
 SEND(state,"release");SEND(p,"release");SEND(r,"release");SEND(cached,"release");SEND(scene,"release");SEND(h,"release");
 if(!committed){fvGDecision(17,"ratio: live presentation commit interrupted or unaccepted");goto retry;}
#ifdef FV_COMPLETE_MODE
 fvGDecision(16,"ratio: first presentation geometry verified");
 fvFMMarkApplied(h);
#endif
 done:objc_setAssociatedObject(h,&fvGBusyKey,0,1);
 return;
 retry:
 objc_setAssociatedObject(h,&fvGBusyKey,0,1);
#ifdef FV_COMPLETE_MODE
 fvFMScheduleRetry(h);
#endif
}
/* One outstanding trailing update per host, at most 20 geometry passes/sec.
 * Retain until the main-queue callback; no recurring timer or global busy flag.
 * Original IMPs always execute, including reentrant setters during our pass. */
static void fvGDrain(void *context){
 id work=context,h=fvGGet(work,"host"),token=fvGGet(work,"token");
 if(fvGWorkCurrent(work,&fvGPendingKey)&&!objc_getAssociatedObject(h,&fvGLeavingKey))fvGApply(h);
 BOOL current=fvGWorkCurrent(work,&fvGPendingKey);
 if(objc_getAssociatedObject(h,&fvGPendingKey)==token)objc_setAssociatedObject(h,&fvGPendingKey,0,1);
#ifdef FV_COMPLETE_MODE
 if(!current&&!objc_getAssociatedObject(h,&fvGLeavingKey)&&!objc_getAssociatedObject(h,&fvGPendingKey)){
  id item=SEND(h,"superview");if(fvGKind(item,fvGItemClass)&&SEND(item,"window"))fvFMRequestMount(h,ivar(h,"hostingView"));
 }
#endif
 id item=SEND(h,"superview");
 if(fvGKind(item,fvGItemClass)&&SEND(item,"window"))fvKAObserveItem(item);
 fvKARefresh();
 SEND(work,"release");
}
static void fvGRequest(id o){
 if(!fvGMain())return;
 id h=fvGTarget(o);
 if(!h||objc_getAssociatedObject(h,&fvGLeavingKey)||objc_getAssociatedObject(h,&fvGBusyKey)||objc_getAssociatedObject(h,&fvGPendingKey))return;
 id token=SEND(objc_getClass("NSObject"),"new");
 objc_setAssociatedObject(h,&fvGPendingKey,token,1);
 id work=fvGWork(h,token);SEND(token,"release");
 dispatch_async_f(&_dispatch_main_q,work,fvGDrain);
}
static void fvGHostLayoutHook(id self,SEL cmd){
 if(fvGHostLayout)((void(*)(id,SEL))fvGHostLayout)(self,cmd);
 fvGRequest(self);
}
static void fvGHostFrameHook(id self,SEL cmd,Rect r){
 if(fvGHostFrame)((void(*)(id,SEL,Rect))fvGHostFrame)(self,cmd,r);
 fvGRequest(self);
}
static void fvGHostBoundsHook(id self,SEL cmd,Rect r){
 if(fvGHostBounds)((void(*)(id,SEL,Rect))fvGHostBounds)(self,cmd,r);
 fvGRequest(self);
}
static void fvGItemLayoutHook(id self,SEL cmd){
 if(fvGItemLayout)((void(*)(id,SEL))fvGItemLayout)(self,cmd);
 fvGRequest(self);
}
static void fvGItemFrameHook(id self,SEL cmd,Rect r){
 if(fvGItemFrame)((void(*)(id,SEL,Rect))fvGItemFrame)(self,cmd,r);
 fvGRequest(self);
}
static void fvGItemBoundsHook(id self,SEL cmd,Rect r){
 if(fvGItemBounds)((void(*)(id,SEL,Rect))fvGItemBounds)(self,cmd,r);
 fvGRequest(self);
}
static void fvGHook(Class c,const char *name,IMP replacement,IMP *original){
 Method m=class_getInstanceMethod(c,S(name));
 *original=method_getImplementation(m);
 /* Adding an inherited method to this class avoids changing UIView itself. */
 if(!class_addMethod(c,S(name),replacement,method_getTypeEncoding(m)))
  class_replaceMethod(c,S(name),replacement,method_getTypeEncoding(m));
}
static BOOL fvGCanHook(Class c){
 return c&&class_getInstanceMethod(c,S("layoutSubviews"))&&
  class_getInstanceMethod(c,S("setFrame:"))&&class_getInstanceMethod(c,S("setBounds:"));
}
#include "FVFirstMount.inc"
#include "FVKeyboardAvoidance.inc"
#include "FVKeyboardGeometryProbe.inc"
#include "FVLifecycle.inc"
static void fvKAItemWindowHook(id self,SEL cmd){
 fvGItemWindowHook(self,cmd);
 if(SEND(self,"window")){fvKAObserveItem(self);fvKARefresh();}
 else fvKADetach(self);
}
static void install(void *unused){
 (void)unused;
 if(fvGInstalled)return;
 fvGHostClass=objc_getClass("_TtC12floatingView24FVAppDispatchHostingView");
 fvGItemClass=objc_getClass("_TtC12floatingView17FVAppDispatchItem");
 /* Keep scene-only writes confined to the first-mount diagnostic. The
  * integrated build waits for the live host/item/remote geometry, then
  * applies the Scene and presentation together in fvGApply. */
#if defined(FV_FIRST_MOUNT_MODE) && !defined(FV_COMPLETE_MODE)
 fvFMInstall(fvGHostClass);
 fvGInstalled=1;
 trace("first-mount-only mode: legacy layout geometry hooks disabled");
 return;
#endif
 if(!fvGCanHook(fvGHostClass)||!fvGCanHook(fvGItemClass)){
  /* Classes are registered asynchronously by the host bundle. Poll briefly
   * so the first visible frame does not remain native-sized for a full second. */
  if(fvGAttempts++<50)dispatch_after_f(dispatch_time(0,10000000),&_dispatch_main_q,0,install);
  return;
 }
 fvGHook(fvGHostClass,"layoutSubviews",(IMP)fvGHostLayoutHook,&fvGHostLayout);
 fvGHook(fvGHostClass,"setFrame:",(IMP)fvGHostFrameHook,&fvGHostFrame);
 fvGHook(fvGHostClass,"setBounds:",(IMP)fvGHostBoundsHook,&fvGHostBounds);
 fvGHook(fvGItemClass,"layoutSubviews",(IMP)fvGItemLayoutHook,&fvGItemLayout);
 fvGHook(fvGItemClass,"setFrame:",(IMP)fvGItemFrameHook,&fvGItemFrame);
 fvGHook(fvGItemClass,"setBounds:",(IMP)fvGItemBoundsHook,&fvGItemBounds);
 /* Restore before scene teardown/fullscreen transitions. These selectors are
  * installed only on the two floatingView classes, never on UIView globally. */
 Method ws=class_getInstanceMethod(fvGHostClass,S("willMoveToSuperview:"));
 if(ws)fvGHook(fvGHostClass,"willMoveToSuperview:",(IMP)fvGHostSuperviewHook,&fvGHostWillSuperview);
 Method is=class_getInstanceMethod(fvGItemClass,S("willMoveToSuperview:"));
 if(is)fvGHook(fvGItemClass,"willMoveToSuperview:",(IMP)fvGItemSuperviewHook,&fvGItemWillSuperview);
 Method ww=class_getInstanceMethod(fvGHostClass,S("didMoveToWindow"));
 if(ww)fvGHook(fvGHostClass,"didMoveToWindow",(IMP)fvGHostWindowHook,&fvGHostWillWindow);
#ifdef FV_KEYBOARD_AVOIDANCE
 Method kwi=class_getInstanceMethod(fvGItemClass,S("willMoveToWindow:"));
 if(kwi&&fvFMEncoding(method_getTypeEncoding(kwi)))fvGHook(fvGItemClass,"willMoveToWindow:",(IMP)fvKAItemWillWindowHook,&fvKAItemWillMoveWindow);
#endif
 Method iw=class_getInstanceMethod(fvGItemClass,S("didMoveToWindow"));
 if(iw)fvGHook(fvGItemClass,"didMoveToWindow",(IMP)fvKAItemWindowHook,&fvGItemWillWindow);
#ifdef FV_COMPLETE_MODE
 fvFMInstall(fvGHostClass);
#endif
 fvKAInstall(fvGItemClass);
#ifdef FV_KEYBOARD_GEOMETRY_PROBE
 fvKGPInstall();
#endif
 fvGInstalled=1;
 trace("geometry hooks installed with bounded native restoration");
}
