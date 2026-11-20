#ifdef FV_KEYBOARD_HOST_ROUTE_PROBE
/* System-side transport comparison ONLY. Never changes a CGRect, placement,
 * state object, focus, scene settings, callback, fence or original return.
 * Target-app code and receipt/ticket bridge are not compiled in this mode. */
#include "FVKeyboardHostRouteCore.h"
typedef _Bool FVKHBool;
extern unsigned method_getNumberOfArguments(Method);
extern Method *class_copyMethodList(Class,unsigned *);
extern void free(void *);
extern const char *class_getName(Class);
extern double CFAbsoluteTimeGetCurrent(void);
extern int snprintf(char *,U,const char *,...);
extern int getpid(void);
static void fvKHLog(const char *event){
 char line[1500];int n=snprintf(line,sizeof line,"%s\n",event);if(n<=0)return;
 U length=(U)n;if(length>=sizeof line)length=sizeof line-1;
 const char *paths[]={"/tmp/FVAspect8.log","/var/mobile/Library/Preferences/FVAspect8.log"};
 for(unsigned i=0;i<2;i++){int fd=open(paths[i],0x209,0644);if(fd>=0){write(fd,line,length);close(fd);}}
}
static unsigned char fvKHCorrelationSalt;
static unsigned long long fvKHToken(id object){
 return fvKHObjectToken((unsigned long long)(U)object,(unsigned long long)(U)&fvKHCorrelationSalt);
}
static void fvKHCorrelation(unsigned seq,unsigned kind,int isMain,unsigned mode,id receiver,id context,id state){
 const char *role=kind==8?(isMain?"receiver-current-prestate":"not-sampled-off-main"):
  (kind==3||kind==4||kind==5||kind==6)?"original-state-argument":"no-state-argument";
 char line[480];snprintf(line,sizeof line,"KHCORR seq=%u pid=%d route=%u main=%d modeSnapshot=%u receiverToken=%016llx contextToken=%016llx stateToken=%016llx stateRole=%s; equality-hint-only, pointer-reuse-possible, not-target-authorization",seq,getpid(),kind,isMain,mode,fvKHToken(receiver),fvKHToken(context),fvKHToken(state),role);fvKHLog(line);
}
static id fvKHHosts;
static volatile unsigned fvKHCurrentMode;
static unsigned fvKHActiveCounts[10][2],fvKHOtherCounts[10][2],fvKHMissingBudget=5,fvKHSequence;
static _Thread_local unsigned fvKHDepth;
static IMP fvKHShowOriginal,fvKHArbiterOriginal,fvKHPrepareOriginal,fvKHRemoteChangedOriginal,fvKHIAVHeightOriginal;
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
 const char *types=method_getTypeEncoding(m);
 if(fvKHEncoding(types,"B@:"))return ((FVKHBool(*)(id,SEL))objc_msgSend)(object,S(selector))?1:0;
 if(fvKHEncoding(types,"c@:"))return ((signed char(*)(id,SEL))objc_msgSend)(object,S(selector))?1:0;
 return -1;
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
 if(!isMain){
  static unsigned workerCounts[10][2];if(!fvKHTake(&workerCounts[kind][mode],12))return;
  unsigned seq=__atomic_add_fetch(&fvKHSequence,1u,__ATOMIC_RELAXED);char line[420];
  fvKHCorrelation(seq,kind,0,mode,receiver,eventScene,state);
  snprintf(line,sizeof line,"KHWORKER seq=%u route=%u modeSnapshot=%u scalar1=%d scalar2=%d notify=%d plain=(%.3f,%.3f,%.3f,%.3f) iav=(%.3f,%.3f,%.3f,%.3f); argument-only, no object getters or host enumeration",seq,kind,mode,relevant,showing,notify,plain.origin.x,plain.origin.y,plain.size.width,plain.size.height,iav.origin.x,iav.origin.y,iav.size.width,iav.size.height);fvKHLog(line);return;
 }
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
 unsigned seq=__atomic_add_fetch(&fvKHSequence,1u,__ATOMIC_RELAXED),sceneHash=fvKHSceneHash(eventScene);
 fvKHCorrelation(seq,kind,1,mode,receiver,eventScene,state);
 char line[1100];
 const char *routeNames[]={"show-window-scene","update-arbiter-scene","prepare-keyboard-scene","signal-arbiter-state","queue-arbiter-state","arbiter-client-change","remote-did-handle-change","unused","queue-with-completion-prestate","input-destination-change"};
 const char *name=receiver?class_getName(object_getClass(receiver)):"nil";
 const char *eventName=eventScene?class_getName(object_getClass(eventScene)):"nil";
 snprintf(line,sizeof line,"KHROUTE abi-fix-1 seq=%u stamp=%.6f mode=%s route=%s receiver=%.90s active=%d context=%08x contextClass=%.70s inputSetEmpty=%d placeShowsKeyboard=%d placeShowsViews=%d undocked=%d interactive=%d extended=%.3f snapshotOrRelevant=%d localOrShowingOrDestination=%d notify=%d plain=(%.3f,%.3f,%.3f,%.3f) iav=(%.3f,%.3f,%.3f,%.3f) remoteFull=(%.3f,%.3f,%.3f,%.3f)",seq,CFAbsoluteTimeGetCurrent(),mode?"ratio9x16":"native",routeNames[kind],name?name:"unknown",active,sceneHash,eventName?eventName:"unknown",empty,placeKeyboard,placeInputs,undocked,interactive,extended,relevant,showing,notify,plain.origin.x,plain.origin.y,plain.size.width,plain.size.height,iav.origin.x,iav.origin.y,iav.size.width,iav.size.height,fullKeyboard.origin.x,fullKeyboard.origin.y,fullKeyboard.size.width,fullKeyboard.size.height);fvKHLog(line);
 if(kind==2||kind==6){
  id screen=fvKHObject(receiver,"_activeScreen"),required=fvKHObject(receiver,"requiredScene");
  id current=fvKHObject(receiver,"currentState");
  Rect currentRect=fvKHABI(current,"keyboardPositionWithIAV","{CGRect={CGPoint=dd}{CGSize=dd}}@:")?rect(current,"keyboardPositionWithIAV"):(Rect){{0,0},{0,0}};
  Rect screenBounds=fvKHABI(screen,"bounds","{CGRect={CGPoint=dd}{CGSize=dd}}@:")?rect(screen,"bounds"):(Rect){{0,0},{0,0}};
  snprintf(line,sizeof line,"KHREMOTE seq=%u screen=%08x screenBounds=(%.2f,%.2f,%.2f,%.2f) requiredScene=%08x currentState=(%.2f,%.2f,%.2f,%.2f) remoteFrame=(%.2f,%.2f,%.2f,%.2f)",seq,fvKHSceneHash(screen),screenBounds.origin.x,screenBounds.origin.y,screenBounds.size.width,screenBounds.size.height,fvKHSceneHash(required),currentRect.origin.x,currentRect.origin.y,currentRect.size.width,currentRect.size.height,fullKeyboard.origin.x,fullKeyboard.origin.y,fullKeyboard.size.width,fullKeyboard.size.height);fvKHLog(line);
 }
 if(state){
  int hasPlain=fvKHABI(state,"keyboardPosition","{CGRect={CGPoint=dd}{CGSize=dd}}@:");
  int hasIAV=fvKHABI(state,"keyboardPositionWithIAV","{CGRect={CGPoint=dd}{CGSize=dd}}@:");
  Rect a=hasPlain?rect(state,"keyboardPosition"):(Rect){{0,0},{0,0}};
  Rect b=hasIAV?rect(state,"keyboardPositionWithIAV"):(Rect){{0,0},{0,0}};
  snprintf(line,sizeof line,"KHSTATE seq=%u plainReadable=%d iavReadable=%d plain=(%.3f,%.3f,%.3f,%.3f) iav=(%.3f,%.3f,%.3f,%.3f)",seq,hasPlain,hasIAV,a.origin.x,a.origin.y,a.size.width,a.size.height,b.origin.x,b.origin.y,b.size.width,b.size.height);fvKHLog(line);
 }
 id hosts=isMain&&fvKHHosts?SEND(fvKHHosts,"allObjects"):0;U count=hosts?(U)SEND(hosts,"count"):0;
 unsigned live=0;
 for(U index=0;index<count&&index<32;index++){
  id host=SEND(hosts,"objectAtIndex:",index),item=SEND(host,"superview"),window=SEND(host,"window");
  if(!window||fvGTarget(item)!=host||((FVKHBool(*)(id,SEL))objc_msgSend)(item,S("isHidden")))continue;
  id scene=ivar(host,"appScene"),settings=fvKHObject(scene,"settings"),cached=ivar(host,"appSetting"),remote=ivar(host,"hostingView");
  if(!fvKHABI(settings,"frame","{CGRect={CGPoint=dd}{CGSize=dd}}@:"))continue;
  unsigned hash=fvKHSceneHash(scene);Rect frame=rect(settings,"frame");
  Rect c=fvKHABI(cached,"frame","{CGRect={CGPoint=dd}{CGSize=dd}}@:")?rect(cached,"frame"):(Rect){{0,0},{0,0}};
  Rect rb=fvKHABI(remote,"bounds","{CGRect={CGPoint=dd}{CGSize=dd}}@:")?rect(remote,"bounds"):(Rect){{0,0},{0,0}};
  id app=ivar(host,"currentActivityApp"),bundle=fvKHObject(app,"bundleIdentifier");
  const char *bid=fvGKind(bundle,objc_getClass("NSString"))?((const char*(*)(id,SEL))objc_msgSend)(bundle,S("UTF8String")):"unknown";
  id eventBacking=fvKHObject(eventScene,"_scene");if(!eventBacking)eventBacking=fvKHObject(eventScene,"_FBSScene");
  int eventObjectMatches=eventScene==scene,eventBackingMatches=eventBacking==scene;
  int screenMatches=0;
  if(kind==2||kind==6){id activeScreen=fvKHObject(receiver,"_activeScreen");screenMatches=activeScreen&&SEND(window,"screen")==activeScreen;}
  snprintf(line,sizeof line,"KHHOST seq=%u route=%u bundle=%.180s hostScene=%08x contextHash=%08x directSceneObjectMatch=%d backingFBSSceneMatch=%d keyboardScreenMatchesWindow=%d settings=(%.3f,%.3f,%.3f,%.3f) cached=(%.3f,%.3f) remote=(%.3f,%.3f) mounted=%d",seq,kind,bid?bid:"unknown",hash,sceneHash,eventObjectMatches,eventBackingMatches,screenMatches,frame.origin.x,frame.origin.y,frame.size.width,frame.size.height,c.size.width,c.size.height,rb.size.width,rb.size.height,remote&&SEND(remote,"superview")==host);fvKHLog(line);live++;
 }
 if(!isMain){fvKHLog("KHHOST host snapshot skipped off-main; route data above remains read-only");}
 else if(!live&&fvKHMissingBudget){fvKHMissingBudget--;fvKHLog("KHHOST no currently attached tracked floating host; route is observation only");}
}
static FVKHBool fvKHShow(id self,SEL cmd,id inputViews,id placement,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<0;
 if(!(scope.previous&(1u<<0)))fvKHRecord(0,self,scene,(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,0,inputViews,placement);
 return ((FVKHBool(*)(id,SEL,id,id,id))fvKHShowOriginal)(self,cmd,inputViews,placement,scene);
}
static void fvKHArbiter(id self,SEL cmd,Rect plain,Rect iav,FVKHBool relevant,FVKHBool showing,FVKHBool notify,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<1;
 if(!(scope.previous&(1u<<1)))fvKHRecord(1,self,scene,plain,iav,relevant,showing,notify,0,0,0);
 ((void(*)(id,SEL,Rect,Rect,FVKHBool,FVKHBool,FVKHBool,id))fvKHArbiterOriginal)(self,cmd,plain,iav,relevant,showing,notify,scene);
}
static void fvKHPrepare(id self,SEL cmd,Rect plain,Rect iav,FVKHBool relevant,FVKHBool showing,FVKHBool notify,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<2;
 if(!(scope.previous&(1u<<2)))fvKHRecord(2,self,scene,plain,iav,relevant,showing,notify,0,0,0);
 ((void(*)(id,SEL,Rect,Rect,FVKHBool,FVKHBool,FVKHBool,id))fvKHPrepareOriginal)(self,cmd,plain,iav,relevant,showing,notify,scene);
}
/* Per-concrete-class original IMPs. Base is installed first; subclass
 * wrappers may call its wrapper, whose per-route TLS guard forwards once. */
static IMP fvKHSignalImp[5],fvKHQueueImp[5],fvKHQueueWithCompletionImp[5],fvKHDidHandleImp[5];
static void fvKHSignalBody(unsigned slot,id self,SEL cmd,id state,id completion){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<3;
 if(!(scope.previous&(1u<<3)))fvKHRecord(3,self,(fvGMain()?fvKHObject(self,"requiredScene"):0),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,state,0,0);
 ((void(*)(id,SEL,id,id))fvKHSignalImp[slot])(self,cmd,state,completion);
}
#define FVKH_SIGNAL_WRAPPER(N) static void fvKHSignal##N(id s,SEL c,id a,id b){fvKHSignalBody(N,s,c,a,b);}
FVKH_SIGNAL_WRAPPER(0) FVKH_SIGNAL_WRAPPER(1) FVKH_SIGNAL_WRAPPER(2) FVKH_SIGNAL_WRAPPER(3) FVKH_SIGNAL_WRAPPER(4)
static IMP fvKHSignalWrappers[5]={(IMP)fvKHSignal0,(IMP)fvKHSignal1,(IMP)fvKHSignal2,(IMP)fvKHSignal3,(IMP)fvKHSignal4};
static void fvKHQueueBody(unsigned slot,id self,SEL cmd,id state,id completion){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<4;
 if(!(scope.previous&(1u<<4)))fvKHRecord(4,self,(fvGMain()?fvKHObject(self,"requiredScene"):0),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,state,0,0);
 ((void(*)(id,SEL,id,id))fvKHQueueImp[slot])(self,cmd,state,completion);
}
#define FVKH_QUEUE_WRAPPER(N) static void fvKHQueue##N(id s,SEL c,id a,id b){fvKHQueueBody(N,s,c,a,b);}
FVKH_QUEUE_WRAPPER(0) FVKH_QUEUE_WRAPPER(1) FVKH_QUEUE_WRAPPER(2) FVKH_QUEUE_WRAPPER(3) FVKH_QUEUE_WRAPPER(4)
static IMP fvKHQueueWrappers[5]={(IMP)fvKHQueue0,(IMP)fvKHQueue1,(IMP)fvKHQueue2,(IMP)fvKHQueue3,(IMP)fvKHQueue4};
static void fvKHQueueCompletionBody(unsigned slot,id self,SEL cmd,id completion){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<8;
 if(!(scope.previous&(1u<<8)))fvKHRecord(8,self,(fvGMain()?fvKHObject(self,"requiredScene"):0),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,(fvGMain()?fvKHObject(self,"currentState"):0),0,0);
 ((void(*)(id,SEL,id))fvKHQueueWithCompletionImp[slot])(self,cmd,completion);
}
#define FVKH_QC_WRAPPER(N) static void fvKHQueueCompletion##N(id s,SEL c,id a){fvKHQueueCompletionBody(N,s,c,a);}
FVKH_QC_WRAPPER(0) FVKH_QC_WRAPPER(1) FVKH_QC_WRAPPER(2) FVKH_QC_WRAPPER(3) FVKH_QC_WRAPPER(4)
static IMP fvKHQueueCompletionWrappers[5]={(IMP)fvKHQueueCompletion0,(IMP)fvKHQueueCompletion1,(IMP)fvKHQueueCompletion2,(IMP)fvKHQueueCompletion3,(IMP)fvKHQueueCompletion4};
static FVKHBool fvKHDidHandleBody(unsigned slot,id self,SEL cmd,id information,FVKHBool snapshot,FVKHBool local){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<5;
 if(!(scope.previous&(1u<<5)))fvKHRecord(5,self,(fvGMain()?fvKHObject(self,"requiredScene"):0),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},snapshot,local,-1,information,0,0);
 return ((FVKHBool(*)(id,SEL,id,FVKHBool,FVKHBool))fvKHDidHandleImp[slot])(self,cmd,information,snapshot,local);
}
#define FVKH_DH_WRAPPER(N) static FVKHBool fvKHDidHandle##N(id s,SEL c,id i,FVKHBool a,FVKHBool b){return fvKHDidHandleBody(N,s,c,i,a,b);}
FVKH_DH_WRAPPER(0) FVKH_DH_WRAPPER(1) FVKH_DH_WRAPPER(2) FVKH_DH_WRAPPER(3) FVKH_DH_WRAPPER(4)
static IMP fvKHDidHandleWrappers[5]={(IMP)fvKHDidHandle0,(IMP)fvKHDidHandle1,(IMP)fvKHDidHandle2,(IMP)fvKHDidHandle3,(IMP)fvKHDidHandle4};
static FVKHBool fvKHRemoteChanged(id self,SEL cmd,id information,FVKHBool snapshot,FVKHBool local){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<6;
 if(!(scope.previous&(1u<<6)))fvKHRecord(6,self,(fvGMain()?fvKHObject(self,"requiredScene"):0),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},snapshot,local,-1,information,0,0);
 return ((FVKHBool(*)(id,SEL,id,FVKHBool,FVKHBool))fvKHRemoteChangedOriginal)(self,cmd,information,snapshot,local);
}
static void fvKHIAVHook(id self,SEL cmd,FVKHBool destination,double height,FVKHBool relevant,FVKHBool notify,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<9;
 if(!(scope.previous&(1u<<9))){
  static unsigned samples;if(fvKHTake(&samples,16)){char line[180];snprintf(line,sizeof line,"KHIAV argument-only destination=%d iavHeight=%.3f relevant=%d notify=%d",(int)destination,height,(int)relevant,(int)notify);fvKHLog(line);}
  fvKHRecord(9,self,scene,(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},relevant,destination,notify,0,0,0);
 }
 ((void(*)(id,SEL,FVKHBool,double,FVKHBool,FVKHBool,id))fvKHIAVHeightOriginal)(self,cmd,destination,height,relevant,notify,scene);
}
static int fvKHHook(const char *className,const char *selector,const char *expected,IMP replacement,IMP *old){
 if(*old)return 1;
 Class cls=objc_getClass(className);Method method=cls?class_getInstanceMethod(cls,S(selector)):0;
 const char *actual=method?method_getTypeEncoding(method):0;
 unsigned expectedArgs=2;for(const char *p=selector;*p;p++)if(*p==':')expectedArgs++;
 const char *reason=!cls?"missing-class":!method?"missing-method":0;
 if(method){
  unsigned count=0;Method *methods=class_copyMethodList(cls,&count);int owned=0;
  for(unsigned i=0;i<count;i++)if(methods[i]==method){owned=1;break;}
  free(methods);
  if(!owned)reason="inherited-covered-by-defining-class";
  else if(method_getNumberOfArguments(method)!=expectedArgs)reason="argument-count-mismatch";
  else if(!fvKHEncoding(actual,expected))reason="encoding-mismatch";
 }
 if(!reason){
  IMP original=method_getImplementation(method);
  if(!original||original==replacement)reason="invalid-original";
  else{*old=original;class_replaceMethod(cls,S(selector),replacement,actual);}
 }
 char line[650];snprintf(line,sizeof line,"KHINSTALL class=%.90s selector=%.180s status=%s actual=%.180s expected=%.120s",className,selector,reason?reason:"installed",actual?actual:"nil",expected);fvKHLog(line);
 return !reason;
}
static void fvKHInstall(void){
 const char *r="{CGRect={CGPoint=dd}{CGSize=dd}}";unsigned installed=0;
 /* Every B parameter below uses FVKHBool (_Bool). Other encodings are
  * rejected, not accepted with an ABI-incompatible signed-char wrapper. */
 if(fvKHHook("UIKeyboardSceneDelegate","_showKeyboardWindowForInputViewSet:placement:windowScene:","B@:@@@",(IMP)fvKHShow,&fvKHShowOriginal))installed|=1;
 const char *selectors[]={"updateArbiterClientWithKeyboardFrame:keyboarFrameWithIAV:isIAVRelevant:showing:notifyRemote:forScene:","prepareToMoveKeyboard:withIAV:isIAVRelevant:showing:notifyRemote:forScene:"};
 const char *classes[]={"UIKeyboardSceneDelegate","_UIRemoteKeyboards"};
 for(unsigned k=0;k<2;k++){
  char types[192];snprintf(types,sizeof types,"v@:%s%sBBB@",r,r);
  if(fvKHHook(classes[k],selectors[k],types,k?(IMP)fvKHPrepare:(IMP)fvKHArbiter,k?&fvKHPrepareOriginal:&fvKHArbiterOriginal))installed|=2u<<k;
 }
 const char *clientClasses[]={"_UIKeyboardArbiterClient","_UIKeyboardArbiterClientSpringBoard","_UIKeyboardArbiterClientInputUIHost","_UIKeyboardArbiterClientInputDestination","_UIRemoteKeyboards"};
 for(unsigned i=0;i<5;i++){
  if(fvKHHook(clientClasses[i],"signalToProxyKeyboardChanged:onCompletion:","v@:@@?",fvKHSignalWrappers[i],&fvKHSignalImp[i]))installed|=8;
  if(fvKHHook(clientClasses[i],"queue_keyboardChanged:onComplete:","v@:@@?",fvKHQueueWrappers[i],&fvKHQueueImp[i]))installed|=16;
  if(fvKHHook(clientClasses[i],"queue_keyboardChangedWithCompletion:","v@:@?",fvKHQueueCompletionWrappers[i],&fvKHQueueWithCompletionImp[i]))installed|=128;
  if(fvKHHook(clientClasses[i],"_didHandleKeyboardClientChange:shouldConsiderSnapshottingKeyboard:isLocalEvent:","B@:@BB",fvKHDidHandleWrappers[i],&fvKHDidHandleImp[i]))installed|=32;
 }
 /* iOS 16 header: bool return, ONE object and TWO bool arguments. */
 if(fvKHHook("_UIRemoteKeyboards","didHandleKeyboardChange:shouldConsiderSnapshottingKeyboard:isLocalEvent:","B@:@BB",(IMP)fvKHRemoteChanged,&fvKHRemoteChangedOriginal))installed|=64;
 if(fvKHHook("_UIKeyboardArbiterClientInputDestination","inputDestinationDidChange:withIAVHeight:isIAVRelevant:notifyRemote:forScene:","v@:BdBB@",(IMP)fvKHIAVHook,&fvKHIAVHeightOriginal))installed|=256;
 char line[360];snprintf(line,sizeof line,"KHROUTE abi-fix-1 installedMask=%u show=%u updateArbiter=%u prepareScene=%u signal=%u queueState=%u arbiterChange=%u remoteChange=%u queueCompletion=%u iavHeight=%u; installation only, not proof of delivery or avoidance",installed,!!(installed&1),!!(installed&2),!!(installed&4),!!(installed&8),!!(installed&16),!!(installed&32),!!(installed&64),!!(installed&128),!!(installed&256));fvKHLog(line);
}
#endif


