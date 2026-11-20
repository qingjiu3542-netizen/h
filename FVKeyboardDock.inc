/* Opt-in visual keyboard docking. No layer migration, Scene mutation, fake
 * notification, app inset claim, or Swift-private function calls. */
#ifndef FV_KEYBOARD_DOCK_INCLUDED
#define FV_KEYBOARD_DOCK_INCLUDED
#ifdef FV_KEYBOARD_DOCK_EXPERIMENT
#include "FVKeyboardDockCore.h"
static id fvKDObserver,fvKDLayers,fvKDScreen;
static Rect fvKDKeys;
static BOOL fvKDVisible,fvKDWriting;
static char fvKDStateKey;
static U fvKDVersion;
static unsigned fvKDBudget=24;
static void fvKDReport(const char *message);
static int fvKDOwns(id layer,id state){
 Point c=fvGCenter(layer),last=fvGUnboxPoint(state,"lastCenter");
 return fvGIdentical(state,"parent",SEND(layer,"superview"))&&
  fvGIdentical(state,"window",SEND(layer,"window"))&&
  fvGSameTransform(trans(layer),fvGUnboxTransform(state,"lastTransform"))&&
  fvGNear(c.x,last.x)&&fvGNear(c.y,last.y);
}
static void fvKDRestore(id layer){
 if(!layer||fvKDWriting)return;
 id state=objc_getAssociatedObject(layer,&fvKDStateKey);if(!state)return;
 SEND(state,"retain");objc_setAssociatedObject(layer,&fvKDStateKey,0,1);
 if(fvGIdentical(state,"parent",SEND(layer,"superview"))&&fvGIdentical(state,"window",SEND(layer,"window"))){
  fvKDWriting=1;
  if(fvGSameTransform(trans(layer),fvGUnboxTransform(state,"lastTransform")))
   fvGSetTransform(layer,fvGUnboxTransform(state,"nativeTransform"));
  Point now=fvGCenter(layer),last=fvGUnboxPoint(state,"lastCenter");
  if(fvGIdentical(state,"parent",SEND(layer,"superview"))&&fvGIdentical(state,"window",SEND(layer,"window"))&&
     fvGNear(now.x,last.x)&&fvGNear(now.y,last.y))fvGSetCenter(layer,fvGUnboxPoint(state,"nativeCenter"));
  fvKDWriting=0;
 }
 SEND(state,"release");
}
static int fvKDIsWriting(void){return fvKDWriting;}
static void fvKDNativeFrame(id layer){if(fvGMain()&&!fvKDWriting)fvKDRestore(layer);}
static void fvKDClear(void){
 fvKDVersion++;fvKDVisible=0;
 id layers=SEND(SEND(fvKDLayers,"allObjects"),"retain");U n=(U)SEND(layers,"count");
 for(U i=0;i<n;i++)fvKDRestore(SEND(layers,"objectAtIndex:",i));
 SEND(layers,"release");SEND(fvKDScreen,"release");fvKDScreen=0;
}
static void fvKDNote(id self,SEL cmd,id note){
 (void)self;(void)cmd;if(!fvGMain()||fvKDWriting)return;
 id source=SEND(note,"object");
 BOOL known=fvGKind(source,objc_getClass("UIScreen"));
 BOOL hide=((BOOL(*)(id,SEL,id))objc_msgSend)(SEND(note,"name"),S("isEqualToString:"),str("UIKeyboardWillHideNotification"));
 if(hide){
  if(!known||source==fvKDScreen)fvKDClear();
  return;
 }
 if(!known){fvKDReport("KB dock skipped: keyboard notification source is not an authenticated UIScreen");return;}
 id box=SEND(SEND(note,"userInfo"),"objectForKey:",str("UIKeyboardFrameEndUserInfoKey"));
 if(!fvGKind(box,objc_getClass("NSValue"))||!fvGHas(box,"CGRectValue")||!fvGHas(box,"objCType"))return;
 const char *encoding=((const char*(*)(id,SEL))objc_msgSend)(box,S("objCType"));
 if(!encoding||__builtin_strcmp(encoding,"{CGRect={CGPoint=dd}{CGSize=dd}}"))return;
 Rect keys=rect(box,"CGRectValue");if(!fvGRect(keys))return;
 if(fvKDVisible&&fvKDScreen!=source)return;
 Rect screen=rect(source,"bounds");
 FVRect a={keys.origin.x,keys.origin.y,keys.size.width,keys.size.height};
 FVRect b={screen.origin.x,screen.origin.y,screen.size.width,screen.size.height};
 if(!fvKAIntersects(a,b)){if(source==fvKDScreen)fvKDClear();return;}
 if(!fvKDVisible)fvKDReport("KB dock notification: source-screen key rectangle received");
 SEND(source,"retain");SEND(fvKDScreen,"release");fvKDScreen=source;fvKDKeys=keys;fvKDVersion++;fvKDVisible=1;
}
static FVKAOriginalMode fvKDMode(long orientation){
 const char *key=fvKAOriginalModeKey((int)orientation);if(!key)return FVKAOriginalModeUnknown;
 id prefs=SEND(SEND(objc_getClass("NSUserDefaults"),"alloc"),"initWithSuiteName:",str("com.be-huge.floating-view-prefs"));
 id value=SEND(prefs,"objectForKey:",str(key));FVKAOriginalMode mode=FVKAOriginalModeInner;
 if(value){
  mode=FVKAOriginalModeUnknown;
  if(fvGKind(value,objc_getClass("NSString")))mode=fvKAOriginalMode(((const char*(*)(id,SEL))objc_msgSend)(value,S("UTF8String")));
 }
 SEND(prefs,"release");return mode;
}
static void fvKDReport(const char *message){if(fvKDBudget){fvKDBudget--;trace(message);}}
#include "FVKeyboardNativeFlagCore.h"
static int fvKDFlag(id item,const char *name){
 extern const char *ivar_getTypeEncoding(void *);
 extern long ivar_getOffset(void *);
 extern U class_getInstanceSize(Class);
 extern Class class_getSuperclass(Class);
 extern int snprintf(char *,U,const char *,...);
 const char *names[]={"isSideCard","innerKeyboardIsDisplay","recordOnlyInnerKeyboardIsShow"};
 Class cls=item?object_getClass(item):0;
 FVKBNativeLayout layout={0};unsigned requested=3;
 layout.exactClass=cls&&cls==fvGItemClass;
 layout.allPresent=1;
 if(!layout.exactClass)return -1;
 layout.instanceSize=class_getInstanceSize(cls);
 Class superclass=class_getSuperclass(cls);if(superclass)layout.superSize=class_getInstanceSize(superclass);
 long offsets[3]={-1,-1,-1};
 for(unsigned i=0;i<3;i++){
  void *v=class_getInstanceVariable(cls,names[i]);
  if(!__builtin_strcmp(name,names[i]))requested=i;
  if(!v){layout.allPresent=0;continue;}
  offsets[i]=ivar_getOffset(v);layout.encoding[i]=ivar_getTypeEncoding(v);
 }
 layout.side=offsets[0];layout.inner=offsets[1];layout.record=offsets[2];
 int value=fvKBReadNativeFlag((const unsigned char*)item,layout,requested);
 static unsigned reportBudget=12;
 if(reportBudget){
  reportBudget--;char message[320];
  snprintf(message,sizeof message,"KB nativeFlag name=%s status=%s value=%d layoutValid=%d offsets=%ld/%ld/%ld super=%lu size=%lu encoding=%.20s",
   name,value<0?"rejected":"read",value,fvKBLayoutValid(layout),layout.side,layout.inner,layout.record,
   layout.superSize,layout.instanceSize,requested<3&&layout.encoding[requested]?layout.encoding[requested]:"<null>");
  trace(message);
 }
 return value;
}
static int fvKDCurrent(id work,id parent,id window){
 id layer=fvGGet(work,"layer"),host=fvGGet(work,"host"),item=fvGGet(work,"item");
 id ratioState=objc_getAssociatedObject(host,&fvGStateKey);
 return enabled()&&fvKDVisible&&ratioState&&!fvGGet(ratioState,"restorePending")&&host&&item&&layer&&layer!=ivar(host,"hostingView")&&
  fvGTarget(item)==host&&ivar(host,"appScene")==fvGGet(work,"scene")&&
  fvGGet(work,"mountToken")&&fvGGet(work,"mountToken")==objc_getAssociatedObject(host,&fvFMMountTokenKey)&&
  !objc_getAssociatedObject(host,&fvGLeavingKey)&&!objc_getAssociatedObject(host,&fvGFullscreenKey)&&
  SEND(layer,"superview")==parent&&SEND(layer,"window")==window&&window&&
  SEND(window,"screen")==fvKDScreen&&SEND(SEND(item,"window"),"screen")==fvKDScreen;
}
static void fvKDDock(id work){
 if(!fvGMain()||fvKDWriting)return;
 id layer=fvGGet(work,"layer"),host=fvGGet(work,"host"),item=fvGGet(work,"item");
 id parent=SEND(layer,"superview"),window=SEND(layer,"window");
 if(!fvKDCurrent(work,parent,window))return;
 fvKDRestore(layer);
 if(!fvKDCurrent(work,parent,window))return;
 U version=fvKDVersion;
 id scene=fvGGet(work,"scene"),settings=SEND(scene,"settings");
 if(!fvGHas(settings,"interfaceOrientation")||!fvGHas(settings,"frame"))return;
 long orientation=((long(*)(id,SEL))objc_msgSend)(settings,S("interfaceOrientation"));
 /* Rotation is deliberately left native in this first runtime experiment. */
 if(orientation!=1){fvKDReport("KB dock skipped: portrait-only runtime experiment");return;}
 FVKAOriginalMode mode=fvKDMode(orientation);
#ifdef FV_KEYBOARD_APP_AVOIDANCE_BRIDGE
 if(mode!=FVKAOriginalModeInner)fvKDClearHostTicket(host);
#endif
 if(mode==FVKAOriginalModeUnknown)return;
 if(mode==FVKAOriginalModeOuter){
  Class owner=objc_getClass("_TtC12floatingView16FVKeyboardWindow");
  if(!fvGKind(window,owner)||ivar(window,"keyboardView")!=parent){
   fvKDReport("KB dock skipped: outer keyboard not in original owned container");return;
  }
 }else{
  int sideCard=fvKDFlag(item,"isSideCard"),innerDisplayed=fvKDFlag(item,"innerKeyboardIsDisplay");
  int recorded=fvKDFlag(item,"recordOnlyInnerKeyboardIsShow");
  if(!fvKBPortraitInnerEligible(sideCard,innerDisplayed,recorded,orientation)){
   fvKDReport("KB dock skipped: original matched-app keyboard record not confirmed");return;
  }
  id ancestor=parent;unsigned depth=0;
  while(ancestor&&ancestor!=host&&depth++<12)ancestor=SEND(ancestor,"superview");
  if(ancestor!=host)return;
 }
 if(!fvGHas(item,"convertRect:toView:")||!fvGHas(parent,"convertRect:toView:"))return;
 Rect nativeBounds=rect(layer,"bounds"),screenBounds=rect(fvKDScreen,"bounds");
 /* Notification keys refer to the source screen. Do not inverse-transform
  * them through the already-scaled floating presentation. */
 if(!fvGRect(nativeBounds)||!fvGRect(screenBounds)||
    fabs(nativeBounds.size.width-screenBounds.size.width)>.5||
    fabs(nativeBounds.size.height-screenBounds.size.height)>.5){
  fvKDReport("KB dock skipped: remote canvas does not match notification source");return;
 }
 Rect target=((Rect(*)(id,SEL,Rect,id))objc_msgSend)(item,S("convertRect:toView:"),rect(item,"bounds"),parent);
 Rect logical=rect(settings,"frame");Transform nativeTransform=trans(layer);Point nativeCenter=fvGCenter(layer);
 if(!fvGRect(target)||!fvGRect(logical)||!fvGTransform(nativeTransform)||!fvGPoint(nativeCenter)||
    nativeTransform.a<=0||nativeTransform.d<=0||fabs(nativeTransform.b)>.001||fabs(nativeTransform.c)>.001||
    fabs(nativeTransform.a-nativeTransform.d)>.001)return;
 if(fabs(logical.size.height-logical.size.width*16.0/9.0)>.5||
    fabs(target.size.height-target.size.width*16.0/9.0)>1)return;
 if(!fvGHas(parent,"convertPoint:toView:"))return;
 Point p0=((Point(*)(id,SEL,Point,id))objc_msgSend)(parent,S("convertPoint:toView:"),(Point){0,0},window);
 Point px=((Point(*)(id,SEL,Point,id))objc_msgSend)(parent,S("convertPoint:toView:"),(Point){1,0},window);
 Point py=((Point(*)(id,SEL,Point,id))objc_msgSend)(parent,S("convertPoint:toView:"),(Point){0,1},window);
 if(!fvGPoint(p0)||!fvGPoint(px)||!fvGPoint(py)||px.x<=p0.x||py.y<=p0.y||
    fabs(px.y-p0.y)>.001||fabs(py.x-p0.x)>.001)return;
 FVRect source={nativeBounds.origin.x,nativeBounds.origin.y,nativeBounds.size.width,nativeBounds.size.height};
 FVRect keys={nativeBounds.origin.x+fvKDKeys.origin.x-screenBounds.origin.x,
  nativeBounds.origin.y+fvKDKeys.origin.y-screenBounds.origin.y,fvKDKeys.size.width,fvKDKeys.size.height};
 /* User requested removal of the experimental extra bottom clearance.
  * Native keyboard artwork/safe-area padding remains unchanged. */
 double bottomGap=0;
 FVKeyboardDockPlan plan;
 if(!fvKeyboardDockPlanWithGap(source,keys,(FVRect){target.origin.x,target.origin.y,target.size.width,target.size.height},
    target.size.width/logical.size.width,mode==FVKAOriginalModeInner,bottomGap,&plan)){
  fvKDReport("KB dock skipped: source keys not docked/full-width or insufficient space");return;
 }
 Rect proposedKeys={{plan.keys.x,plan.keys.y},{plan.keys.width,plan.keys.height}};
 Rect visible=((Rect(*)(id,SEL,Rect,id))objc_msgSend)(parent,S("convertRect:toView:"),proposedKeys,window);
 Rect windowBounds=rect(window,"bounds");
 if(!fvGRect(visible)||!fvGRect(windowBounds)||visible.origin.x<windowBounds.origin.x-.5||
    visible.origin.y<windowBounds.origin.y-.5||visible.origin.x+visible.size.width>windowBounds.origin.x+windowBounds.size.width+.5||
    visible.origin.y+visible.size.height>windowBounds.origin.y+windowBounds.size.height+.5)return;
 Transform targetTransform={plan.scale,0,0,plan.scale,0,0};
 Point targetCenter={plan.surface.x+plan.surface.width/2,plan.surface.y+plan.surface.height/2};
 id state=SEND(objc_getClass("NSMutableDictionary"),"new");
 fvGIdentity(state,"parent",parent);fvGIdentity(state,"window",window);fvGIdentity(state,"host",host);
 fvGPut(state,"nativeTransform",fvGBoxTransform(nativeTransform));fvGPut(state,"nativeCenter",fvGBoxPoint(nativeCenter));
 fvGPut(state,"lastTransform",fvGBoxTransform(targetTransform));fvGPut(state,"lastCenter",fvGBoxPoint(targetCenter));
 SEND(layer,"retain");SEND(parent,"retain");SEND(window,"retain");
 if(fvKDCurrent(work,parent,window)&&version==fvKDVersion){
  fvKDWriting=1;
  fvGSetTransform(layer,targetTransform);
  if(fvKDCurrent(work,parent,window)&&version==fvKDVersion&&fvGSameTransform(trans(layer),targetTransform))fvGSetCenter(layer,targetCenter);
  fvKDWriting=0;
  if(fvKDCurrent(work,parent,window)&&version==fvKDVersion&&fvKDOwns(layer,state)){
   objc_setAssociatedObject(layer,&fvKDStateKey,state,1);SEND(fvKDLayers,"addObject:",layer);
#ifdef FV_KEYBOARD_APP_AVOIDANCE_BRIDGE
   fvKDPublishHost(host);
#endif
   extern int snprintf(char *,U,const char *,...);
   char report[320];
   snprintf(report,sizeof report,"KB dock applied: mode=%s sourceKeys=%.1fx%.1f scale=%.4f keys=%.1f,%.1f,%.1f,%.1f reservedLogical=%.1f inputInsetImplemented=0",
    mode==FVKAOriginalModeInner?"inner":"outer",source.width,keys.height,plan.scale,
    plan.keys.x,plan.keys.y,plan.keys.width,plan.keys.height,plan.reservedLogicalHeight);
   fvKDReport(report);
  }else if(SEND(layer,"superview")==parent&&SEND(layer,"window")==window&&fvGSameTransform(trans(layer),targetTransform)){
   fvKDWriting=1;fvGSetTransform(layer,nativeTransform);fvGSetCenter(layer,nativeCenter);fvKDWriting=0;
   fvKDReport("KB dock rolled back: mount changed or geometry rejected");
  }
 }
 SEND(window,"release");SEND(parent,"release");SEND(layer,"release");SEND(state,"release");
}
static void fvKDRestoreLayersForHost(id host){
 if(!fvGMain()||!host||fvKDWriting||!fvKDLayers)return;
 id layers=SEND(SEND(fvKDLayers,"allObjects"),"retain");U n=(U)SEND(layers,"count");
 for(U i=0;i<n;i++){
  id layer=SEND(layers,"objectAtIndex:"),state=objc_getAssociatedObject(layer,&fvKDStateKey);
  if(state&&fvGIdentical(state,"host",host))fvKDRestore(layer);
 }
 SEND(layers,"release");
}
static void fvKDDropForHost(id host){
#ifdef FV_KEYBOARD_APP_AVOIDANCE_BRIDGE
 fvKDClearHostTicket(host);
#endif
 fvKDRestoreLayersForHost(host);
}
static void fvKDDeferred(void *context){id work=context;fvKDDock(work);SEND(work,"release");}
static void fvKDQueue(id work){
 if(!fvGMain())return;
 const long long delays[]={180000000LL,360000000LL};
 for(unsigned i=0;i<2;i++)dispatch_after_f(dispatch_time(0,delays[i]),&_dispatch_main_q,SEND(work,"retain"),fvKDDeferred);
}
static void fvKDInstall(void){
 if(fvKDObserver)return;
 Class c=objc_allocateClassPair(objc_getClass("NSObject"),"FVIndependentKeyboardDockObserverV1",0);if(!c)return;
 class_addMethod(c,S("keyboardChanged:"),(IMP)fvKDNote,"v@:@");objc_registerClassPair(c);
 fvKDObserver=SEND(c,"new");fvKDLayers=SEND(SEND(objc_getClass("NSHashTable"),"weakObjectsHashTable"),"retain");
 id nc=SEND(objc_getClass("NSNotificationCenter"),"defaultCenter");
 const char *names[]={"UIKeyboardWillShowNotification","UIKeyboardWillChangeFrameNotification","UIKeyboardDidChangeFrameNotification","UIKeyboardWillHideNotification"};
 for(unsigned i=0;i<4;i++)SEND(nc,"addObserver:selector:name:object:",fvKDObserver,S("keyboardChanged:"),str(names[i]),(id)0);
 trace("KB dock experiment installed: measured source region required; portrait visual fit only");
}
#else
static int fvKDIsWriting(void){return 0;}
static void fvKDNativeFrame(id layer){(void)layer;}
static void fvKDDock(id work){(void)work;}
static void fvKDQueue(id work){(void)work;}
static void fvKDInstall(void){}
#endif
#endif
