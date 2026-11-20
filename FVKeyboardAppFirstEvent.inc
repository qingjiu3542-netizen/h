/* Optional synchronous first-event evidence. The native preparation call is
 * forwarded unchanged once. Context lives on its stack and is restored on
 * nested dispatch; no persistent previous-keyboard cache or notification replay. */
#include "FVKeyboardAppFirstEventCore.h"
static int hook(Class cls,const char *selector,const char *types,IMP replacement,IMP *old);
typedef struct {
 int valid;id scene,screen,ticketScene;FVKARect source,target,keys;double entered;
} FVKAFirstContext;
static _Thread_local FVKAFirstContext *firstContext;
static _Thread_local unsigned firstCaptureDepth;
typedef struct {FVKAFirstContext context;FVKAFirstContext *previous;unsigned captureDepth;} FVKAFirstScope;
/* Built with -fexceptions: cleanup also restores TLS if native code unwinds. */
static void closeFirstScope(FVKAFirstScope *scope){
 firstContext=scope->previous;firstCaptureDepth=scope->captureDepth;
 if(scope->context.ticketScene)M0(scope->context.ticketScene,"release");
 if(scope->context.screen)M0(scope->context.screen,"release");
 if(scope->context.scene)M0(scope->context.scene,"release");
}
static IMP oldPrepareScreen;
static unsigned firstProbeBudget=6,firstSuccessBudget=6;
static int sameEventEvidence(id state,id object,id info,FVKARect source,FVKARect target,
 FVKARect incoming,FVKARect *keys,int *route){
 *route=0;
 Rect bounds;int hasBounds=readBox(get(info,"UIKeyboardBoundsUserInfoKey"),&bounds);
 FVKARect boundsKeys={0};int boundsOK=hasBounds&&fvKAFFirstEventKeys(source,target,core(bounds),incoming,&boundsKeys);
 FVKAFirstContext *context=firstContext;int contextOK=0;FVKARect liveSource,liveTarget;
 const char *why=0;
 if(context&&context->valid&&object==context->screen&&
    equal(get(state,"scene"),context->ticketScene)&&
    fvKAFSameRect(source,context->source)&&fvKAFSameRect(target,context->target)&&
    liveTargetForScene(object,context->scene,&liveSource,&liveTarget,&why)&&
    fvKAFSameRect(source,liveSource)&&fvKAFSameRect(target,liveTarget)){
  int shape=fvKAFMeasuredShape(source,target,context->keys,incoming);
  contextOK=shape==2;
 }
 /* Independent candidates may corroborate each other, never override a
  * contradictory full-size candidate that is itself recognized. */
 if(boundsOK&&contextOK&&!fvKAFSameRect(boundsKeys,context->keys))return 0;
 if(contextOK){*keys=context->keys;*route=2;return 1;}
 if(boundsOK){*keys=boundsKeys;*route=1;return 1;}
 if(firstProbeBudget){
  firstProbeBudget--;char line[320];double age=context?CFAbsoluteTimeGetCurrent()-context->entered:-1;
  snprintf(line,sizeof line,"first-event evidence unavailable boundsPresent=%d bounds=(%.2f,%.2f,%.2f,%.2f) context=%d age=%.3f",hasBounds,hasBounds?bounds.origin.x:0,hasBounds?bounds.origin.y:0,hasBounds?bounds.size.width:0,hasBounds?bounds.size.height:0,context?context->valid:0,age);appLog(line);
 }
 return 0;
}
static void captureFirstContext(FVKAFirstContext *context,Rect position,Rect withIAV,
 BOOL relevant,BOOL showing,id scene){
 if(!showing||!isKind(scene,"UIWindowScene")||
    !((BOOL(*)(id,SEL))objc_msgSend)(objc_getClass("NSThread"),S("isMainThread")))return;
 if(!appObjectABI(scene,"screen","@@:"))return;
 id screen=M0(scene,"screen");FVKARect source,target;const char *why=0;
 if(!liveTargetForScene(screen,scene,&source,&target,&why))return;
 int missing=0;id state=readState(mainBundleID(),&why,&missing);
 if(!state||!localTarget(state,screen,&source,&target,&why))return;
 FVKARect plain=core(position),iav=core(withIAV),keys;
 if(!relevant){
  /* A declared irrelevant IAV must not supply a different height. */
  iav=plain;
 }
 int valid=fvKAFFirstEventNativeKeys(source,target,plain,iav,showing,&keys);
 if(firstProbeBudget){
  firstProbeBudget--;char line[350];snprintf(line,sizeof line,"first-event native preparation valid=%d relevant=%d showing=%d frame=(%.2f,%.2f,%.2f,%.2f) iav=(%.2f,%.2f,%.2f,%.2f)",valid,relevant!=0,showing!=0,plain.x,plain.y,plain.width,plain.height,withIAV.origin.x,withIAV.origin.y,withIAV.size.width,withIAV.size.height);appLog(line);
 }
 if(!valid)return;
 context->source=source;context->target=target;context->keys=keys;
 context->scene=M0(scene,"retain");context->screen=M0(screen,"retain");
 context->ticketScene=M0(get(state,"scene"),"retain");context->valid=context->ticketScene!=0;
}
static id firstSceneForScreen(id screen){
 if(!isKind(screen,"UIScreen")||!((BOOL(*)(id,SEL))objc_msgSend)(objc_getClass("NSThread"),S("isMainThread")))return 0;
 id app=M0(objc_getClass("UIApplication"),"sharedApplication");if(!responds(app,"connectedScenes"))return 0;
 id scenes=M0(M0(app,"connectedScenes"),"allObjects");U n=(U)M0(scenes,"count");id chosen=0;
 for(U i=0;i<n;i++){
  id scene=((id(*)(id,SEL,U))objc_msgSend)(scenes,S("objectAtIndex:"),i);
  if(!isKind(scene,"UIWindowScene")||!responds(scene,"activationState")||((long(*)(id,SEL))objc_msgSend)(scene,S("activationState"))!=0)continue;
  if(chosen)return 0;chosen=scene;
 }
 return chosen&&responds(chosen,"screen")&&M0(chosen,"screen")==screen?chosen:0;
}
static void prepareScreenHook(id remote,SEL cmd,Rect position,Rect withIAV,BOOL showing,id screen){
 FVKAFirstScope scope __attribute__((cleanup(closeFirstScope)))={0};
 scope.previous=firstContext;scope.captureDepth=firstCaptureDepth;
 FVKAFirstContext *context=&scope.context;context->entered=CFAbsoluteTimeGetCurrent();firstContext=context;
 if(!firstCaptureDepth){firstCaptureDepth++;captureFirstContext(context,position,withIAV,1,showing,firstSceneForScreen(screen));firstCaptureDepth--;}
 ((void(*)(id,SEL,Rect,Rect,BOOL,id))oldPrepareScreen)(remote,cmd,position,withIAV,showing,screen);
}
static void installFirstEvent(void){
 Class cls=objc_getClass("_UIRemoteKeyboards");unsigned installed=0;
 const char *selector="prepareToMoveKeyboard:withIAV:showing:forScreen:";
 const char *r="{CGRect={CGPoint=dd}{CGSize=dd}}";
 if(cls)for(unsigned flags=0;flags<2;flags++){
  char types[160];snprintf(types,sizeof types,"v@:%s%s%c@",r,r,flags?'c':'B');
  if(hook(cls,selector,types,(IMP)prepareScreenHook,&oldPrepareScreen)){installed=1;break;}
 }
 char line[220];snprintf(line,sizeof line,"first-event adapter installed preparationSelectors=%u; same-dispatch evidence only; no waiting or replay",installed);appLog(line);
}
