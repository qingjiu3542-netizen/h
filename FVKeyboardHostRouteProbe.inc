#ifdef FV_KEYBOARD_HOST_ROUTE_PROBE
/* System-side transport comparison ONLY. Never changes a CGRect, placement,
 * state object, focus, scene settings, callback, fence or original return.
 * Target-app code and receipt/ticket bridge are not compiled in this mode. */
#include "FVKeyboardHostRouteCore.h"
extern const char *class_getName(Class);
extern double CFAbsoluteTimeGetCurrent(void);
extern int snprintf(char *,U,const char *,...);
static id fvKHHosts;
static volatile unsigned fvKHCurrentMode;
static unsigned fvKHActiveCounts[10][2],fvKHOtherCounts[10][2],fvKHMissingBudget=5,fvKHSequence;
static _Thread_local unsigned fvKHDepth;
static IMP fvKHShowOriginal,fvKHArbiterOriginal,fvKHPrepareOriginal,fvKHDidHandleOriginal,fvKHRemoteChangedOriginal,fvKHIAVHeightOriginal;
static int fvKHABI(id object,const char *selector,const char *expected){
 if(!object)return 0;
 Method method=class_getInstanceMethod(object_getClass(object),S(selector));
 return method&&fvKHEncoding(method_getTypeEncoding(method),expected);
}
static id fvKHObject(id object,const char *selector){
 return fvKHABI(object,selector,"@@:")?SEND(object,selector):0;
}
static int fvKHBoolProperty(id object,const char *selector){
 if(!object)return -1;Method m=class_getInstanceMethod(object_getClass(object),S(selector));if(!m)return -1;
 const char *types=method_getTypeEncoding(m);if(!fvKHEncoding(types,"B@:")&&!fvKHEncoding(types,"c@:"))return -1;
 return ((BOOL(*)(id,SEL))objc_msgSend)(object,S(selector))?1:0;
}
static double fvKHDoubleProperty(id object,const char *selector){
 if(!object)return -1;Method m=class_getInstanceMethod(object_getClass(object),S(selector));
 if(!m||!fvKHEncoding(method_getTypeEncoding(m),"d@:"))return -1;
 return ((double(*)(id,SEL))objc_msgSend)(object,S(selector));
}
static unsigned fvKHSceneHash(id scene){
 if(!scene)return 0;
 id identifier=fvKHObject(scene,"identifier");
 if(!fvGKind(identifier,objc_getClass("NSString"))){
  id backing=fvKHObject(scene,"_scene");identifier=fvKHObject(backing,"identifier");
 }
 if(!fvGKind(identifier,objc_getClass("NSString")))return 0;
 const char *s=((const char*(*)(id,SEL))objc_msgSend)(identifier,S("UTF8String"));
 return fvKHIdentifierHash(s);
}
static int fvKHTake(unsigned *slot,unsigned cap){
 unsigned old=__atomic_load_n(slot,__ATOMIC_RELAXED);
 while(old<cap){if(__atomic_compare_exchange_n(slot,&old,old+1,0,__ATOMIC_RELAXED,__ATOMIC_RELAXED))return 1;}
 return 0;
}
static void fvKHObserveHost(id host){
 if(!host||!fvGMain()||!fvGKind(host,fvGHostClass))return;
 if(!fvKHHosts)fvKHHosts=SEND(SEND(objc_getClass("NSHashTable"),"weakObjectsHashTable"),"retain");
 if(fvKHHosts)SEND(fvKHHosts,"addObject:",host);
 __atomic_store_n(&fvKHCurrentMode,enabled()?1u:0u,__ATOMIC_RELAXED);
}
typedef struct{unsigned previous;} FVKHScope;
static void fvKHClose(FVKHScope *scope){fvKHDepth=scope->previous;}
static void fvKHRecord(unsigned kind,id receiver,id eventScene,Rect plain,Rect iav,
 int relevant,int showing,int notify,id state,id inputViews,id placement){
 if(kind>=10)return;
 int isMain=fvGMain();unsigned mode=isMain?(enabled()?1u:0u):__atomic_load_n(&fvKHCurrentMode,__ATOMIC_RELAXED);
 int empty=-1,placeKeyboard=-1,placeInputs=-1,undocked=-1,interactive=-1;double extended=-1;
 if(kind==0){
  if(fvKHABI(inputViews,"isEmpty","B@:")||fvKHABI(inputViews,"isEmpty","c@:"))empty=fvKHBoolProperty(inputViews,"isEmpty");
  placeKeyboard=fvKHBoolProperty(placement,"showsKeyboard");
  placeInputs=fvKHBoolProperty(placement,"showsInputViews");
  undocked=fvKHBoolProperty(placement,"isUndocked");interactive=fvKHBoolProperty(placement,"isInteractive");
  extended=fvKHDoubleProperty(placement,"extendedHeight");
 }
 Rect fullKeyboard={{0,0},{0,0}};
 int hasFull=(kind==2||kind==6||kind==7||kind==8)&&fvKHABI(receiver,"keyboardFrameIncludingRemoteIAV","{CGRect={CGPoint=dd}{CGSize=dd}}@:");
 if(hasFull)fullKeyboard=rect(receiver,"keyboardFrameIncludingRemoteIAV");
 int active=(kind==0&&(empty==0||placeKeyboard==1||placeInputs==1))||
  ((kind==1||kind==2)&&(showing==1||plain.size.width>0||plain.size.height>0||iav.size.height>0||fullKeyboard.size.height>0))||
  ((kind>=3)&&state&&((fvKHBoolProperty(state,"keyboardOnScreen")==1)||(fvKHABI(state,"keyboardPositionWithIAV","{CGRect={CGPoint=dd}{CGSize=dd}}@:")&&rect(state,"keyboardPositionWithIAV").size.height>0)))||
  (kind==9&&(relevant==1||plain.size.height>0));
 unsigned *counter=active?&fvKHActiveCounts[kind][mode]:&fvKHOtherCounts[kind][mode];
 unsigned cap=active?((kind<=2||kind==5||kind==6)?8:4):2;
 if(!fvKHTake(counter,cap))return;
 unsigned seq=++fvKHSequence,sceneHash=fvKHSceneHash(eventScene);
 char line[1100];
 const char *routeNames[]={"show-window-scene","update-arbiter-scene","prepare-keyboard-screen","signal-arbiter-state","queue-arbiter-state","arbiter-client-change","remote-keyboard-change","queue-client-change","other","input-destination-change"};
 const char *name=receiver?class_getName(object_getClass(receiver)):"nil";
 const char *eventName=eventScene?class_getName(object_getClass(eventScene)):"nil";
 snprintf(line,sizeof line,"KHROUTE v0.1.22 seq=%u stamp=%.6f mode=%s route=%s receiver=%.90s active=%d context=%08x contextClass=%.70s inputSetEmpty=%d placeShowsKeyboard=%d placeShowsViews=%d undocked=%d interactive=%d extended=%.3f relevant=%d showing=%d notify=%d plain=(%.3f,%.3f,%.3f,%.3f) iav=(%.3f,%.3f,%.3f,%.3f) remoteFull=(%.3f,%.3f,%.3f,%.3f)",seq,CFAbsoluteTimeGetCurrent(),mode?"ratio9x16":"native",routeNames[kind],name?name:"unknown",active,sceneHash,eventName?eventName:"unknown",empty,placeKeyboard,placeInputs,undocked,interactive,extended,relevant,showing,notify,plain.origin.x,plain.origin.y,plain.size.width,plain.size.height,iav.origin.x,iav.origin.y,iav.size.width,iav.size.height,fullKeyboard.origin.x,fullKeyboard.origin.y,fullKeyboard.size.width,fullKeyboard.size.height);trace(line);
 if(kind==2||kind==6){
  id screen=fvKHObject(receiver,"_activeScreen"),required=fvKHObject(receiver,"requiredScene");
  id current=fvKHObject(receiver,"currentState");
  Rect currentRect=fvKHABI(current,"keyboardPositionWithIAV","{CGRect={CGPoint=dd}{CGSize=dd}}@:")?rect(current,"keyboardPositionWithIAV"):(Rect){{0,0},{0,0}};
  Rect screenBounds=fvKHABI(screen,"bounds","{CGRect={CGPoint=dd}{CGSize=dd}}@:")?rect(screen,"bounds"):(Rect){{0,0},{0,0}};
  snprintf(line,sizeof line,"KHREMOTE seq=%u screen=%08x screenBounds=(%.2f,%.2f,%.2f,%.2f) requiredScene=%08x currentState=(%.2f,%.2f,%.2f,%.2f) remoteFrame=(%.2f,%.2f,%.2f,%.2f)",seq,fvKHSceneHash(screen),screenBounds.origin.x,screenBounds.origin.y,screenBounds.size.width,screenBounds.size.height,fvKHSceneHash(required),currentRect.origin.x,currentRect.origin.y,currentRect.size.width,currentRect.size.height,fullKeyboard.origin.x,fullKeyboard.origin.y,fullKeyboard.size.width,fullKeyboard.size.height);trace(line);
 }
 if(state){
  int hasPlain=fvKHABI(state,"keyboardPosition","{CGRect={CGPoint=dd}{CGSize=dd}}@:");
  int hasIAV=fvKHABI(state,"keyboardPositionWithIAV","{CGRect={CGPoint=dd}{CGSize=dd}}@:");
  Rect a=hasPlain?rect(state,"keyboardPosition"):(Rect){{0,0},{0,0}};
  Rect b=hasIAV?rect(state,"keyboardPositionWithIAV"):(Rect){{0,0},{0,0}};
  snprintf(line,sizeof line,"KHSTATE seq=%u plainReadable=%d iavReadable=%d plain=(%.3f,%.3f,%.3f,%.3f) iav=(%.3f,%.3f,%.3f,%.3f)",seq,hasPlain,hasIAV,a.origin.x,a.origin.y,a.size.width,a.size.height,b.origin.x,b.origin.y,b.size.width,b.size.height);trace(line);
 }
 id hosts=isMain&&fvKHHosts?SEND(fvKHHosts,"allObjects"):0;U count=hosts?(U)SEND(hosts,"count"):0;
 unsigned live=0;
 for(U index=0;index<count&&index<32;index++){
  id host=SEND(hosts,"objectAtIndex:",index),item=SEND(host,"superview"),window=SEND(host,"window");
  if(!window||fvGTarget(item)!=host||((BOOL(*)(id,SEL))objc_msgSend)(item,S("isHidden")))continue;
  id scene=ivar(host,"appScene"),settings=fvKHObject(scene,"settings"),cached=ivar(host,"appSetting"),remote=ivar(host,"hostingView");
  if(!fvKHABI(settings,"frame","{CGRect={CGPoint=dd}{CGSize=dd}}@:"))continue;
  unsigned hash=fvKHSceneHash(scene);Rect frame=rect(settings,"frame");
  Rect c=fvKHABI(cached,"frame","{CGRect={CGPoint=dd}{CGSize=dd}}@:")?rect(cached,"frame"):(Rect){{0,0},{0,0}};
  Rect rb=remote?rect(remote,"bounds"):(Rect){{0,0},{0,0}};
  id app=ivar(host,"currentActivityApp"),bundle=fvKHObject(app,"bundleIdentifier");
  const char *bid=fvGKind(bundle,objc_getClass("NSString"))?((const char*(*)(id,SEL))objc_msgSend)(bundle,S("UTF8String")):"unknown";
  id eventBacking=fvKHObject(eventScene,"_scene");if(!eventBacking)eventBacking=fvKHObject(eventScene,"_FBSScene");
  int eventObjectMatches=eventScene==scene,eventBackingMatches=eventBacking==scene;
  int screenMatches=0;
  if(kind==2||kind==6){id activeScreen=fvKHObject(receiver,"_activeScreen");screenMatches=activeScreen&&SEND(window,"screen")==activeScreen;}
  snprintf(line,sizeof line,"KHHOST seq=%u route=%u bundle=%.180s hostScene=%08x contextHash=%08x directSceneObjectMatch=%d backingFBSSceneMatch=%d keyboardScreenMatchesWindow=%d settings=(%.3f,%.3f,%.3f,%.3f) cached=(%.3f,%.3f) remote=(%.3f,%.3f) mounted=%d",seq,kind,bid?bid:"unknown",hash,sceneHash,eventObjectMatches,eventBackingMatches,screenMatches,frame.origin.x,frame.origin.y,frame.size.width,frame.size.height,c.size.width,c.size.height,rb.size.width,rb.size.height,remote&&SEND(remote,"superview")==host);trace(line);live++;
 }
 if(!isMain){trace("KHHOST host snapshot skipped off-main; route data above remains read-only");}
 else if(!live&&fvKHMissingBudget){fvKHMissingBudget--;trace("KHHOST no currently attached tracked floating host; route is observation only");}
}
static BOOL fvKHShow(id self,SEL cmd,id inputViews,id placement,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<0;
 if(!(scope.previous&(1u<<0)))fvKHRecord(0,self,scene,(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,0,inputViews,placement);
 return ((BOOL(*)(id,SEL,id,id,id))fvKHShowOriginal)(self,cmd,inputViews,placement,scene);
}
static void fvKHArbiter(id self,SEL cmd,Rect plain,Rect iav,BOOL relevant,BOOL showing,BOOL notify,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<1;
 if(!(scope.previous&(1u<<1)))fvKHRecord(1,self,scene,plain,iav,relevant,showing,notify,0,0,0);
 ((void(*)(id,SEL,Rect,Rect,BOOL,BOOL,BOOL,id))fvKHArbiterOriginal)(self,cmd,plain,iav,relevant,showing,notify,scene);
}
static void fvKHPrepare(id self,SEL cmd,Rect plain,Rect iav,BOOL relevant,BOOL showing,BOOL notify,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<2;
 if(!(scope.previous&(1u<<2)))fvKHRecord(2,self,scene,plain,iav,relevant,showing,notify,0,0,0);
 ((void(*)(id,SEL,Rect,Rect,BOOL,BOOL,BOOL,id))fvKHPrepareOriginal)(self,cmd,plain,iav,relevant,showing,notify,scene);
}
/* Per-concrete-class original IMPs. Base is installed first; subclass
 * wrappers may call its wrapper, whose per-route TLS guard forwards once. */
static IMP fvKHSignalImp[5],fvKHQueueImp[5],fvKHQueueWithCompletionImp[5],fvKHDidHandleImp[5];
static void fvKHSignalBody(unsigned slot,id self,SEL cmd,id state,id completion){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<3;
 if(!(scope.previous&(1u<<3)))fvKHRecord(3,self,fvKHObject(self,"requiredScene"),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,state,0,0);
 ((void(*)(id,SEL,id,id))fvKHSignalImp[slot])(self,cmd,state,completion);
}
#define FVKH_SIGNAL_WRAPPER(N) static void fvKHSignal##N(id s,SEL c,id a,id b){fvKHSignalBody(N,s,c,a,b);}
FVKH_SIGNAL_WRAPPER(0) FVKH_SIGNAL_WRAPPER(1) FVKH_SIGNAL_WRAPPER(2) FVKH_SIGNAL_WRAPPER(3) FVKH_SIGNAL_WRAPPER(4)
static IMP fvKHSignalWrappers[5]={(IMP)fvKHSignal0,(IMP)fvKHSignal1,(IMP)fvKHSignal2,(IMP)fvKHSignal3,(IMP)fvKHSignal4};
static void fvKHQueueBody(unsigned slot,id self,SEL cmd,id state,id completion){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<4;
 if(!(scope.previous&(1u<<4)))fvKHRecord(4,self,fvKHObject(self,"requiredScene"),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,state,0,0);
 ((void(*)(id,SEL,id,id))fvKHQueueImp[slot])(self,cmd,state,completion);
}
#define FVKH_QUEUE_WRAPPER(N) static void fvKHQueue##N(id s,SEL c,id a,id b){fvKHQueueBody(N,s,c,a,b);}
FVKH_QUEUE_WRAPPER(0) FVKH_QUEUE_WRAPPER(1) FVKH_QUEUE_WRAPPER(2) FVKH_QUEUE_WRAPPER(3) FVKH_QUEUE_WRAPPER(4)
static IMP fvKHQueueWrappers[5]={(IMP)fvKHQueue0,(IMP)fvKHQueue1,(IMP)fvKHQueue2,(IMP)fvKHQueue3,(IMP)fvKHQueue4};
static void fvKHQueueCompletionBody(unsigned slot,id self,SEL cmd,id completion){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<8;
 if(!(scope.previous&(1u<<8)))fvKHRecord(8,self,fvKHObject(self,"requiredScene"),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,fvKHObject(self,"currentState"),0,0);
 ((void(*)(id,SEL,id))fvKHQueueWithCompletionImp[slot])(self,cmd,completion);
}
#define FVKH_QC_WRAPPER(N) static void fvKHQueueCompletion##N(id s,SEL c,id a){fvKHQueueCompletionBody(N,s,c,a);}
FVKH_QC_WRAPPER(0) FVKH_QC_WRAPPER(1) FVKH_QC_WRAPPER(2) FVKH_QC_WRAPPER(3) FVKH_QC_WRAPPER(4)
static IMP fvKHQueueCompletionWrappers[5]={(IMP)fvKHQueueCompletion0,(IMP)fvKHQueueCompletion1,(IMP)fvKHQueueCompletion2,(IMP)fvKHQueueCompletion3,(IMP)fvKHQueueCompletion4};
static BOOL fvKHDidHandleBody(unsigned slot,id self,SEL cmd,id information,BOOL snapshot,BOOL local){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<5;
 if(!(scope.previous&(1u<<5)))fvKHRecord(5,self,fvKHObject(self,"requiredScene"),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},snapshot,local,-1,information,0,0);
 return ((BOOL(*)(id,SEL,id,BOOL,BOOL))fvKHDidHandleImp[slot])(self,cmd,information,snapshot,local);
}
#define FVKH_DH_WRAPPER(N) static BOOL fvKHDidHandle##N(id s,SEL c,id i,BOOL a,BOOL b){return fvKHDidHandleBody(N,s,c,i,a,b);}
FVKH_DH_WRAPPER(0) FVKH_DH_WRAPPER(1) FVKH_DH_WRAPPER(2) FVKH_DH_WRAPPER(3) FVKH_DH_WRAPPER(4)
static IMP fvKHDidHandleWrappers[5]={(IMP)fvKHDidHandle0,(IMP)fvKHDidHandle1,(IMP)fvKHDidHandle2,(IMP)fvKHDidHandle3,(IMP)fvKHDidHandle4};
static void fvKHRemoteChanged(id self,SEL cmd,id information,BOOL snapshot,BOOL local){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<6;
 if(!(scope.previous&(1u<<6)))fvKHRecord(6,self,fvKHObject(self,"requiredScene"),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},snapshot,local,-1,information,0,0);
 ((void(*)(id,SEL,id,BOOL,BOOL))fvKHRemoteChangedOriginal)(self,cmd,information,snapshot,local);
}
static void fvKHIAVHook(id self,SEL cmd,BOOL destination,double height,BOOL relevant,BOOL notify,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<9;
 if(!(scope.previous&(1u<<9))){Rect marker={{0,0},{430,height}};fvKHRecord(9,self,scene,marker,(Rect){{0,0},{0,0}},relevant,destination,notify,0,0,0);}
 ((void(*)(id,SEL,BOOL,double,BOOL,BOOL,id))fvKHIAVHeightOriginal)(self,cmd,destination,height,relevant,notify,scene);
}
static int fvKHHook(const char *className,const char *selector,const char *expected,IMP replacement,IMP *old){
 Class cls=objc_getClass(className);Method method=cls?class_getInstanceMethod(cls,S(selector)):0;
 const char *actual=method?method_getTypeEncoding(method):0;
 if(!method||!fvKHEncoding(actual,expected))return 0;
 *old=method_getImplementation(method);
 if(!class_addMethod(cls,S(selector),replacement,actual))class_replaceMethod(cls,S(selector),replacement,actual);
 return 1;
}
static void fvKHInstall(void){
 const char *r="{CGRect={CGPoint=dd}{CGSize=dd}}";unsigned installed=0;
 for(unsigned b=0;b<2;b++){
  char types[32];snprintf(types,sizeof types,"%c@:@@@",b?'c':'B');
  if(fvKHHook("UIKeyboardSceneDelegate","_showKeyboardWindowForInputViewSet:placement:windowScene:",types,(IMP)fvKHShow,&fvKHShowOriginal)){installed|=1;break;}
 }
 const char *selectors[]={"updateArbiterClientWithKeyboardFrame:keyboarFrameWithIAV:isIAVRelevant:showing:notifyRemote:forScene:","prepareToMoveKeyboard:withIAV:isIAVRelevant:showing:notifyRemote:forScene:"};
 const char *classes[]={"UIKeyboardSceneDelegate","_UIRemoteKeyboards"};
 for(unsigned k=0;k<2;k++)for(unsigned flags=0;flags<8;flags++){
  char types[192];snprintf(types,sizeof types,"v@:%s%s%c%c%c@",r,r,flags&1?'c':'B',flags&2?'c':'B',flags&4?'c':'B');
  if(fvKHHook(classes[k],selectors[k],types,k?(IMP)fvKHPrepare:(IMP)fvKHArbiter,k?&fvKHPrepareOriginal:&fvKHArbiterOriginal)){installed|=2u<<k;break;}
 }
 const char *clientClasses[]={"_UIKeyboardArbiterClientSpringBoard","_UIKeyboardArbiterClientInputUIHost","_UIKeyboardArbiterClientInputDestination","_UIKeyboardArbiterClient","_UIRemoteKeyboards"};
 const char *stateSelectors[]={"signalToProxyKeyboardChanged:onCompletion:","queue_keyboardChanged:onComplete:","queue_keyboardChangedWithCompletion:"};
 const char *stateTypes[]={"v@:@@?","v@:@@?","v@:@?"};
 for(unsigned i=0;i<5;i++){
  if(fvKHHook(clientClasses[i],stateSelectors[0],stateTypes[0],fvKHSignalWrappers[i],&fvKHSignalImp[i]))installed|=8;
  if(fvKHHook(clientClasses[i],stateSelectors[1],stateTypes[1],fvKHQueueWrappers[i],&fvKHQueueImp[i]))installed|=16;
  if(fvKHHook(clientClasses[i],stateSelectors[2],stateTypes[2],fvKHQueueCompletionWrappers[i],&fvKHQueueWithCompletionImp[i]))installed|=128;
 }
 const char *bools[]={"B","c"};
 for(unsigned i=0;i<5;i++)for(unsigned a=0;a<2;a++)for(unsigned b=0;b<2;b++)for(unsigned c=0;c<2;c++){
  char methodTypes[80];snprintf(methodTypes,sizeof methodTypes,"%s@:@@%s%s",bools[a],bools[b],bools[c]);
  if(fvKHHook(clientClasses[i],"_didHandleKeyboardClientChange:shouldConsiderSnapshottingKeyboard:isLocalEvent:",methodTypes,fvKHDidHandleWrappers[i],&fvKHDidHandleImp[i]))installed|=32;
 }
 for(unsigned a=0;a<2;a++)for(unsigned b=0;b<2;b++){
  char methodTypes[80];snprintf(methodTypes,sizeof methodTypes,"v@:@@%s%s",bools[a],bools[b]);
  if(fvKHHook("_UIRemoteKeyboards","keyboardChanged:shouldConsiderSnapshottingKeyboard:isLocalEvent:",methodTypes,(IMP)fvKHRemoteChanged,&fvKHRemoteChangedOriginal)){installed|=64;goto remoteChangedDone;}
 }
 remoteChangedDone:
 for(unsigned a=0;a<2;a++)for(unsigned b=0;b<2;b++){
  char methodTypes[80];snprintf(methodTypes,sizeof methodTypes,"v@:%cdBB@",bools[a]);
  if(fvKHHook("_UIKeyboardArbiterClientInputDestination","inputDestinationDidChange:withIAVHeight:isIAVRelevant:notifyRemote:forScene:",methodTypes,(IMP)fvKHIAVHook,&fvKHIAVHeightOriginal)){installed|=256;break;}
 }
 char line[340];snprintf(line,sizeof line,"KHROUTE v0.1.22 installedMask=%u show=%u updateArbiter=%u prepareScreen=%u signal=%u queueState=%u arbiterChange=%u remoteChange=%u queueCompletion=%u iavHeight=%u; native-ratio comparison only; no geometry rewrite",installed,!!(installed&1),!!(installed&2),!!(installed&4),!!(installed&8),!!(installed&16),!!(installed&32),!!(installed&64),!!(installed&128),!!(installed&256));trace(line);
}
#endif


