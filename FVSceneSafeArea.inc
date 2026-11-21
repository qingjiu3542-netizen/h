#ifdef FV_SCENE_SAFEAREA_EXPERIMENT
#if !defined(FV_KEYBOARD_HOST_ROUTE_PROBE) || !defined(FV_KEYBOARD_DOCK_EXPERIMENT) || defined(FV_KEYBOARD_APP_AVOIDANCE_BRIDGE)
#error Host safe-area experiment requires host-only route/dock, never an app bridge
#endif
#include "FVSceneSafeAreaCore.h"
/* Host-only, single-use A(2s)/B(+12 logical pt,4s)/A experiment.
 * Field 15 is NEVER trusted just because an iOS17 disassembly uses it:
 * prove the mapping with a disposable UI mutable copy on this runtime.
 * Missing backing object is NOT explicit zero: refuse it, never materialize it.
 * Deliberately DO NOT call updateUISettingsWithBlock: metadata only says @?
 * and does not establish its callback's full ABI. uiSettings is a read facade;
 * publish a one-field mutation on a FRESH ordinary scene.settings copy. */
static FVSAState fvSAS;
static id fvSAHost,fvSAScene,fvSAReceiver,fvSAItem,fvSAWindow,fvSASource,fvSABundle,fvSAConsumed;
static double fvSAStart;static unsigned fvSATicks;static int fvSAStopping,fvSAAttempted,fvSAInTick;
static const char *fvSAKey="sceneSafeAreaAB29";
static int fvSAFieldProved;static unsigned fvSALogBudget=80;
static void fvSALog(const char *s){if(fvSALogBudget){fvSALogBudget--;trace(s);}}
static int fvSAABI(id o,const char *sel,const char *encoding){
 if(!o)return 0;Method m=class_getInstanceMethod(object_getClass(o),S(sel));
 unsigned n=2;for(const char *p=sel;*p;p++)if(*p==':')n++;
 return m&&method_getNumberOfArguments(m)==n&&fvKHEncoding(method_getTypeEncoding(m),encoding);
}
static id fvSAObject(id o,const char *s){return fvSAABI(o,s,"@@:")?SEND(o,s):0;}
static int fvSAString(id o){return fvGKind(o,objc_getClass("NSString"))&&(U)SEND(o,"length")>0;}
static int fvSAEqualString(id a,id b){return fvSAString(a)&&fvSAString(b)&&((BOOL(*)(id,SEL,id))objc_msgSend)(a,S("isEqualToString:"),b);}
static FVSAInsets fvSAGetInsets(id o,const char *sel){return ((FVSAInsets(*)(id,SEL))objc_msgSend)(o,S(sel));}
static id fvSAField(id settings){
 id other=fvSAObject(settings,"otherSettings");
 return fvSAABI(other,"objectForSetting:","@@:Q")?SEND(other,"objectForSetting:",(U)15):0;
}
static int fvSAUnbox(id value,FVSAInsets *out){
 if(!fvGKind(value,objc_getClass("NSValue"))||!fvSAABI(value,"objCType","r*@:")||
    !fvSAABI(value,"UIEdgeInsetsValue","{UIEdgeInsets=dddd}@:"))return 0;
 const char *t=((const char*(*)(id,SEL))objc_msgSend)(value,S("objCType"));
 if(!t||!fvKHEncoding(t,"{UIEdgeInsets=dddd}"))return 0;
 *out=fvSAGetInsets(value,"UIEdgeInsetsValue");return fvSAValid(*out);
}
static int fvSARead(void *unused,FVSASnapshot *out){
 (void)unused;if(!fvGMain()||!fvSAScene)return 0;
 id base=SEND(fvSAObject(fvSAScene,"settings"),"retain");
 id ui=SEND(fvSAObject(fvSAScene,"uiSettings"),"retain");
 FVSAInsets a,b;int ok=0;
 if(base&&ui&&fvSAABI(ui,"safeAreaInsetsPortrait","{UIEdgeInsets=dddd}@:")&&
    fvSAUnbox(fvSAField(base),&a)&&fvSAUnbox(fvSAField(ui),&b)&&fvSAEqual(a,b)&&
    fvSAEqual(a,fvSAGetInsets(ui,"safeAreaInsetsPortrait"))&&
    fvSAObject(fvSAScene,"settings")==base){
  *out=(FVSASnapshot){(U)fvSAScene,1,1,a};ok=1;
 }
 SEND(ui,"release");SEND(base,"release");return ok;
}
static int fvSAProveField(void){
 id ui=SEND(fvSAObject(fvSAScene,"uiSettings"),"retain"),copy=0;int ok=0;
 FVSAInsets original,boxed;FVSAInsets probe={3.125,4.25,5.375,6.5};
 if(!ui||!fvSAABI(ui,"safeAreaInsetsPortrait","{UIEdgeInsets=dddd}@:")||
    !fvSAUnbox(fvSAField(ui),&original)||!fvSAABI(ui,"mutableCopy","@@:"))goto done;
 copy=SEND(ui,"mutableCopy");
 if(!copy||copy==ui||fvSAObject(copy,"otherSettings")==fvSAObject(ui,"otherSettings")||
    !fvGKind(copy,objc_getClass("UIMutableApplicationSceneSettings"))||
    !fvSAABI(copy,"setSafeAreaInsetsPortrait:","v@:{UIEdgeInsets=dddd}")||
    !fvSAABI(copy,"safeAreaInsetsPortrait","{UIEdgeInsets=dddd}@:"))goto done;
 ((void(*)(id,SEL,FVSAInsets))objc_msgSend)(copy,S("setSafeAreaInsetsPortrait:"),probe);
 ok=fvSAUnbox(fvSAField(copy),&boxed)&&fvSAEqual(boxed,probe)&&
    fvSAEqual(fvSAGetInsets(copy,"safeAreaInsetsPortrait"),probe)&&
    fvSAUnbox(fvSAField(ui),&boxed)&&fvSAEqual(boxed,original)&&
    fvSAEqual(fvSAGetInsets(ui,"safeAreaInsetsPortrait"),original);
 done:SEND(copy,"release");SEND(ui,"release");return ok;
}
static int fvSAFocus(id host,id receiver){
 id item=SEND(host,"superview"),window=SEND(host,"window"),scene=ivar(host,"appScene");
 id settings=fvSAObject(scene,"settings"),remote=ivar(host,"hostingView");
 id state=fvSAObject(receiver,"currentState"),required=fvSAObject(receiver,"requiredScene");
 id source=fvSAObject(state,"sourceSceneIdentityString"),bundle=fvSAObject(state,"sourceBundleIdentifier");
 id hostID=fvSAObject(scene,"identifier"),appBundle=fvSAObject(ivar(host,"currentActivityApp"),"bundleIdentifier");
 if(!fvGMain()||!enabled()||!host||!item||!scene||!window||!remote||!state||
    fvGTarget(item)!=host||SEND(remote,"superview")!=host||SEND(remote,"window")!=window||
    fvKHBoolProperty(item,"isHidden")!=0||fvKHBoolProperty(window,"isHidden")!=0||
    !objc_getAssociatedObject(host,&fvGStateKey)||objc_getAssociatedObject(host,&fvGLeavingKey)||
    objc_getAssociatedObject(host,&fvGFullscreenKey)||fvGGet(objc_getAssociatedObject(host,&fvGStateKey),"restorePending")||
    !fvSAABI(settings,"interfaceOrientation","q@:")||((long(*)(id,SEL))objc_msgSend)(settings,S("interfaceOrientation"))!=1||
    fvKDMode(1)!=FVKAOriginalModeInner||fvKDFlag(item,"isSideCard")!=0||
    fvKDFlag(item,"innerKeyboardIsDisplay")!=1||fvKDFlag(item,"recordOnlyInnerKeyboardIsShow")!=1||
    fvGWindowCoverage(item,rect(item,"bounds"))!=0||fvKHBoolProperty(state,"keyboardOnScreen")!=1||
    !fvSAEqualString(source,hostID)||!fvSAEqualString(bundle,appBundle)||
    fvSAObject(receiver,"_activeScreen")!=SEND(window,"screen"))return 0;
 /* Require exact live Scene identity as well as full strings, not hashes. */
 if(required!=scene&&fvSAObject(required,"_scene")!=scene&&fvSAObject(required,"_FBSScene")!=scene)return 0;
 return 1;
}
static id fvSAPreference(void){
 id p=prefs(),v=SEND(SEND(p,"objectForKey:",str(fvSAKey)),"retain");SEND(p,"release");
 if(!fvSAString(v)){SEND(v,"release");return 0;}return SEND(v,"autorelease");
}
static int fvSAUnique(void){
 id hosts=SEND(SEND(fvKHHosts,"allObjects"),"retain");U n=(U)SEND(hosts,"count");unsigned matches=0;
 if(n>32){SEND(hosts,"release");return 0;}
 for(U i=0;i<n;i++)if(fvSAFocus(SEND(hosts,"objectAtIndex:",i),fvSAReceiver))matches++;
 SEND(hosts,"release");return matches==1;
}
static int fvSACurrent(void *unused,int restore){
 (void)unused;if(!fvGMain()||!fvSAScene||(U)fvSAScene!=fvSAS.scene||fvKHBoolProperty(fvSAScene,"isValid")!=1)return 0;
 if(restore)return 1; /* Exact RETAINED old Scene only, even after host replacement. */
 id state=fvSAObject(fvSAReceiver,"currentState");
 return !fvSAStopping&&ivar(fvSAHost,"appScene")==fvSAScene&&
  SEND(fvSAHost,"superview")==fvSAItem&&SEND(fvSAHost,"window")==fvSAWindow&&
  fvSAEqualString(fvSAObject(state,"sourceSceneIdentityString"),fvSASource)&&
  fvSAEqualString(fvSAObject(state,"sourceBundleIdentifier"),fvSABundle)&&
  fvSAEqualString(fvSAPreference(),fvSAConsumed)&&fvSAFocus(fvSAHost,fvSAReceiver)&&fvSAUnique();
}
static id fvSABox(FVSAInsets value){
 return ((id(*)(id,SEL,FVSAInsets))objc_msgSend)(objc_getClass("NSValue"),S("valueWithUIEdgeInsets:"),value);
}
/* This is the only publication function. Production and extracted-function
 * mocks exercise its fresh copy / identity / reentry checks verbatim. */
static int fvSASubmit(void *unused,FVSASnapshot expected,FVSAInsets value,int restore){
 (void)unused;id base=0,copy=0,other=0,box=0;int result=0;FVSASnapshot now={0};
 if(!fvSAFieldProved||!fvSACurrent(0,restore)||!fvSARead(0,&now)||!fvSAEqual(now.value,expected.value)||
    now.scene!=expected.scene)return 0;
 if(!restore&&(value.top!=expected.value.top||value.left!=expected.value.left||
    value.right!=expected.value.right||value.bottom!=expected.value.bottom+12))return 0;
 base=SEND(fvSAObject(fvSAScene,"settings"),"retain");
 if(!base||fvGKind(base,objc_getClass("FBSMutableSceneSettings"))||
    !fvSAABI(base,"mutableCopy","@@:")||
    !fvSAABI(fvSAScene,"updateSettings:withTransitionContext:completion:","v@:@@@?")||
    !fvSAABI((id)objc_getClass("NSValue"),"valueWithUIEdgeInsets:","@@:{UIEdgeInsets=dddd}"))goto done;
 copy=SEND(base,"mutableCopy");other=fvSAObject(copy,"otherSettings");
 if(!copy||copy==base||!other||other==fvSAObject(base,"otherSettings")||
    !fvSAABI(other,"setObject:forSetting:","v@:@Q")||
    fvSAObject(fvSAScene,"settings")!=base||!fvSACurrent(0,restore))goto done;
 box=fvSABox(value);
 if(!box)goto done;
 SEND(other,"setObject:forSetting:",box,(U)15);
 FVSAInsets copied;
 if(!fvSAUnbox(fvSAField(copy),&copied)||!fvSAEqual(copied,value)||
    !fvSACurrent(0,restore)||fvSAObject(fvSAScene,"settings")!=base||
    !fvSARead(0,&now)||now.scene!=expected.scene||!fvSAEqual(now.value,expected.value)||
    (!restore&&fvSAS.cancel)||fvSAObject(fvSAScene,"settings")!=base)goto done;
 SEND(fvSAScene,"updateSettings:withTransitionContext:completion:",copy,(id)0,(id)0);
 result=1;
 done:SEND(copy,"release");SEND(base,"release");return result;
}
static FVSAOps fvSAOperations(void){return (FVSAOps){0,fvSACurrent,fvSARead,fvSASubmit};}
static void fvSARelease(void){
 if(fvSAS.owned)fvSALog("SA29 BLOCKER cleanup-unconfirmed; no stale replay; disable and close Scene/respring before uninstall");
 id *objects[]={&fvSAHost,&fvSAScene,&fvSAReceiver,&fvSAItem,&fvSAWindow,&fvSASource,&fvSABundle};
 for(U i=0;i<sizeof objects/sizeof objects[0];i++){SEND(*objects[i],"release");*objects[i]=0;}
 fvSAS=(FVSAState){0};fvSAStopping=0;fvSAFieldProved=0;
}
static void fvSAStopForHost(id host){
 if(!fvGMain()||!fvSAHost||(host&&host!=fvSAHost))return;
 fvSAStopping=1;fvSAS.cancel=1;
 if(fvSAS.busy)return;
 FVSAOps ops=fvSAOperations();int r=fvSATransact(&fvSAS,&ops,1,(FVSAInsets){0});
 if(!fvSAS.owned){fvSALog(r==FVSASuperseded?"SA29 superseded; native value preserved, request spent":"SA29 A2 restored/or untouched; request spent");}
}
static void fvSATick(void *unused){
 (void)unused;if(!fvGMain()||!fvSAHost||fvSAInTick)return;fvSAInTick=1;
 double elapsed=CFAbsoluteTimeGetCurrent()-fvSAStart;fvSATicks++;
 if(elapsed<0||elapsed>8||fvSATicks>100||!fvSACurrent(0,0))fvSAStopping=1;
 if(fvSAStopping)fvSAStopForHost(0);
 else if(elapsed>=2&&!fvSAAttempted){
  fvSAAttempted=1;FVSASnapshot initial={0};
  if(!fvSARead(0,&initial)||!fvSAProveField()){
   fvSALog("SA29 REJECT absent/unproven field or ABI: unset is not explicit zero; NO Scene write");fvSAStopping=1;
  }else{
   fvSAFieldProved=1;FVSAInsets target=initial.value;target.bottom+=12;
   FVSAOps ops=fvSAOperations();int r=fvSATransact(&fvSAS,&ops,0,target);
   fvSALog(r==FVSAApplied?"SA29 B host-readback accepted bottom+12 logical pt; client/keyboard effect UNPROVEN":"SA29 B rejected/pending/superseded; no automatic reapply");
   if(r!=FVSAApplied||fvSAS.cancel)fvSAStopping=1;
  }
 }else if(fvSAS.owned){
  FVSASnapshot current={0};
  if(!fvSARead(0,&current)||!fvSAEqual(current.value,fvSAS.last)||elapsed>=6)fvSAStopping=1;
 }
 if(fvSAStopping)fvSAStopForHost(0);
 int finished=fvSAStopping&&(!fvSAS.owned||elapsed>=10||fvSATicks>=120);
 if(finished){fvSARelease();fvSAInTick=0;return;}
 fvSAInTick=0;dispatch_after_f(dispatch_time(0,100000000),&_dispatch_main_q,0,fvSATick);
}
/* A genuine native remote-keyboard callback is a candidate trigger, NOT
 * authorization. Next main-queue tick authenticates fresh currentState,
 * full source identity strings, requiredScene, active screen and ONE host. */
static void fvSAEvent(unsigned kind,id receiver,id state){
 if(!fvGMain()||kind!=6||!state)return;
 if(fvSAHost){if(receiver==fvSAReceiver&&fvKHBoolProperty(state,"keyboardOnScreen")==0)fvSAStopForHost(0);return;}
 id token=fvSAPreference();if(!token||fvSAEqualString(token,fvSAConsumed))return;
 /* Only consume when a unique fully matched candidate exists. */
 id hosts=SEND(SEND(fvKHHosts,"allObjects"),"retain"),chosen=0;U n=(U)SEND(hosts,"count");unsigned matches=0;
 if(n<=32)for(U i=0;i<n;i++){id host=SEND(hosts,"objectAtIndex:",i);if(fvSAFocus(host,receiver)){chosen=host;matches++;}}
 if(matches!=1){SEND(hosts,"release");return;}
 SEND(fvSAConsumed,"release");fvSAConsumed=SEND(token,"copy");
 fvSAHost=SEND(chosen,"retain");fvSAScene=SEND(ivar(chosen,"appScene"),"retain");
 fvSAItem=SEND(SEND(chosen,"superview"),"retain");fvSAWindow=SEND(SEND(chosen,"window"),"retain");
 fvSAReceiver=SEND(receiver,"retain");id current=fvSAObject(receiver,"currentState");
 fvSASource=SEND(fvSAObject(current,"sourceSceneIdentityString"),"copy");
 fvSABundle=SEND(fvSAObject(current,"sourceBundleIdentifier"),"copy");
 SEND(hosts,"release");fvSAS=(FVSAState){.scene=(U)fvSAScene};
 fvSAStopping=fvSAAttempted=0;fvSATicks=0;fvSAStart=CFAbsoluteTimeGetCurrent();
 fvSALog("SA29 A1 armed single-scene A2s/B4s/A; no keyboard-height coupling, no app injection");
 dispatch_after_f(dispatch_time(0,100000000),&_dispatch_main_q,0,fvSATick);
}
#endif
