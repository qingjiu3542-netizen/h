/* Native UIKit layout query adapter. Does not move views, rewrite Scene sizes,
 * synthesize notifications or replace keyboard state objects. Runtime ABI gates
 * every optional private selector; unsupported systems retain native behavior. */
static IMP oldIntersectionShort,oldIntersectionLong;
static _Thread_local unsigned nativeLayoutDepth;
static unsigned nativeLayoutBudget=8,nativeMappedBudget=4;
static _Thread_local unsigned nativeLocalMinimumSeen;
static int nativeMethodABI(id object,const char *selector,const char *types){
 if(!object)return 0;
 Method m=class_getInstanceMethod(object_getClass(object),S(selector));
 return m&&encoding(method_getTypeEncoding(m),types);
}
static int nativeBoolABI(id object,const char *selector){
 return nativeMethodABI(object,selector,"B@:")||nativeMethodABI(object,selector,"c@:");
}
static void nativeLayoutReport(const char *message){
 if(nativeLayoutBudget){nativeLayoutBudget--;appLog(message);}
}
static int nativeLayoutEvidence(id remote,id scene,FVKARect *source,FVKARect *target,FVKARect *keyboard,const char **reason){
#define NATIVEFAIL(message) do{if(reason)*reason=message;return 0;}while(0)
 if(!scene||!isKind(scene,"UIWindowScene"))NATIVEFAIL("query has no UIWindowScene");
 if(!((BOOL(*)(id,SEL))objc_msgSend)(objc_getClass("NSThread"),S("isMainThread")))NATIVEFAIL("query is not on main thread");
 if(!nativeMethodABI(remote,"_activeScreen","@@:"))NATIVEFAIL("native active-screen getter unavailable or ABI mismatch");
 id screen=M0(remote,"_activeScreen");if(!isKind(screen,"UIScreen"))NATIVEFAIL("native keyboard has no active UIScreen");
 if(!liveTargetForScene(screen,scene,source,target,reason))return 0;
 int missing=0;const char *ticketReason=0;id state=readState(mainBundleID(),&ticketReason,&missing);
 /* A 9:16 window alone cannot distinguish inner from outer keyboard mode.
  * Native layout writes therefore require the publisher's enabled inner ticket. */
 if(!state)NATIVEFAIL(ticketReason?ticketReason:"enabled inner-mode Scene ticket required for native layout");
 if(!localTarget(state,screen,source,target,reason))return 0;
 if(!nativeMethodABI(remote,"currentState","@@:"))NATIVEFAIL("native keyboard state getter unavailable or ABI mismatch");
 id native=M0(remote,"currentState");if(!native)NATIVEFAIL("native keyboard state is absent");
 if(!nativeBoolABI(native,"keyboardOnScreen"))NATIVEFAIL("native visibility getter unavailable or ABI mismatch");
 if(!((BOOL(*)(id,SEL))objc_msgSend)(native,S("keyboardOnScreen")))NATIVEFAIL("native keyboard is offscreen");
 if(!nativeMethodABI(native,"keyboardPositionWithIAV","{CGRect={CGPoint=dd}{CGSize=dd}}@:"))NATIVEFAIL("native keyboard rectangle getter unavailable or ABI mismatch");
 *keyboard=core(rect(native,"keyboardPositionWithIAV"));
 FVKARect measuredKeys;double keyStamp=0,packetStamp=0,sequence=0;
 if(measuredEvidence(state,*source,*target,&measuredKeys,&keyStamp,&packetStamp,&sequence)){
  int shape=fvKAFMeasuredShape(*source,*target,measuredKeys,*keyboard);
  if(shape)*keyboard=measuredKeys;
  else{
   static unsigned rectBudget=4;if(rectBudget){rectBudget--;char details[300];
    snprintf(details,sizeof details,"native layout state unrecognized rect=(%.2f,%.2f,%.2f,%.2f) measuredKeys=(%.2f,%.2f,%.2f,%.2f)",keyboard->x,keyboard->y,keyboard->width,keyboard->height,measuredKeys.x,measuredKeys.y,measuredKeys.width,measuredKeys.height);appLog(details);
   }
   NATIVEFAIL("native rectangle does not match fresh measured key geometry");
  }
 }
 FVKARect mapped;if(!fvKAFMap(*source,*target,*keyboard,&mapped)){
  static unsigned rawBudget=4;if(rawBudget){rawBudget--;char details[240];
   snprintf(details,sizeof details,"native layout raw rect=(%.2f,%.2f,%.2f,%.2f) source=(%.2f,%.2f) target=(%.2f,%.2f)",keyboard->x,keyboard->y,keyboard->width,keyboard->height,source->width,source->height,target->width,target->height);appLog(details);
  }
  NATIVEFAIL("native keyboard is not full-width source-docked geometry");
 }
 if(reason)*reason="native state and explicit Scene authorized";return 1;
#undef NATIVEFAIL
}
static double nativeLayoutResult(id remote,id scene,double original,int localMinimum){
 FVKARect source,target,keyboard;const char *reason=0;double result=original;
 if(!nativeLayoutEvidence(remote,scene,&source,&target,&keyboard,&reason)){
  char line[240];snprintf(line,sizeof line,"native layout query unchanged: %s",reason?reason:"evidence rejected");nativeLayoutReport(line);return original;
 }
 int changed=fvKAFNativeIntersection(source,target,keyboard,original,localMinimum,&result);
 char line[256];snprintf(line,sizeof line,"native layout intersection %s original=%.2f result=%.2f keys=(%.2f,%.2f,%.2f,%.2f) localMinimum=%d",changed?"mapped":"unchanged",original,result,keyboard.x,keyboard.y,keyboard.width,keyboard.height,localMinimum);
 if(changed){if(nativeMappedBudget){nativeMappedBudget--;appLog(line);}}
 else nativeLayoutReport(line);
 return result;
}
static double intersectionShortHook(id remote,SEL cmd,id scene){
 if(nativeLayoutDepth)return ((double(*)(id,SEL,id))oldIntersectionShort)(remote,cmd,scene);
 nativeLayoutDepth++;nativeLocalMinimumSeen=0;
 double original=((double(*)(id,SEL,id))oldIntersectionShort)(remote,cmd,scene);
 double result=nativeLayoutResult(remote,scene,original,nativeLocalMinimumSeen!=0);
 nativeLayoutDepth--;return result;
}
static double intersectionLongHook(id remote,SEL cmd,id scene,BOOL *localMinimum,BOOL ignoreHorizontalOffset){
 if(nativeLayoutDepth){
  double value=((double(*)(id,SEL,id,BOOL*,BOOL))oldIntersectionLong)(remote,cmd,scene,localMinimum,ignoreHorizontalOffset);
  if(localMinimum&&*localMinimum)nativeLocalMinimumSeen=1;
  return value;
 }
 nativeLayoutDepth++;nativeLocalMinimumSeen=0;
 double original=((double(*)(id,SEL,id,BOOL*,BOOL))oldIntersectionLong)(remote,cmd,scene,localMinimum,ignoreHorizontalOffset);
 double result=nativeLayoutResult(remote,scene,original,nativeLocalMinimumSeen||(localMinimum&&*localMinimum));
 nativeLayoutDepth--;return result;
}
static void installNativeLayout(void){
 Class cls=objc_getClass("_UIRemoteKeyboards");if(!cls){appLog("native layout adapter not installed: _UIRemoteKeyboards unavailable");return;}
 unsigned installed=0;
 if(hook(cls,"intersectionHeightForWindowScene:","d@:@",(IMP)intersectionShortHook,&oldIntersectionShort))installed++;
 const char *longSelector="intersectionHeightForWindowScene:isLocalMinimumHeightOut:ignoreHorizontalOffset:";
 const char *types[]={"d@:@^BB","d@:@^cc","d@:@^Bc","d@:@^cB"};
 for(unsigned i=0;i<4;i++)if(hook(cls,longSelector,types[i],(IMP)intersectionLongHook,&oldIntersectionLong)){installed++;break;}
 char line[200];snprintf(line,sizeof line,"native layout adapter installed selectors=%u; exact Scene, native state and original intersection required",installed);appLog(line);
}
