#ifdef FV_KEYBOARD_HOST_ROUTE_PROBE
/* System-side transport comparison ONLY. Never changes a CGRect, placement,
 * state object, focus, scene settings, callback, fence or original return.
 * Target-app code and receipt/ticket bridge are not compiled in this mode. */
#include "FVKeyboardHostRouteCore.h"
extern const char *class_getName(Class);
extern double CFAbsoluteTimeGetCurrent(void);
extern int snprintf(char *,U,const char *,...);
static id fvKHHosts;
static unsigned fvKHCounts[5][2],fvKHMissingBudget=5,fvKHSequence;
static _Thread_local unsigned fvKHDepth;
static IMP fvKHShowOriginal,fvKHArbiterOriginal,fvKHPrepareOriginal,fvKHSignalOriginal,fvKHQueueOriginal;
static int fvKHABI(id object,const char *selector,const char *expected){
 if(!object)return 0;
 Method method=class_getInstanceMethod(object_getClass(object),S(selector));
 return method&&fvKHEncoding(method_getTypeEncoding(method),expected);
}
static id fvKHObject(id object,const char *selector){
 return fvKHABI(object,selector,"@@:")?SEND(object,selector):0;
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
static void fvKHObserveHost(id host){
 if(!host||!fvGMain()||!fvGKind(host,fvGHostClass))return;
 if(!fvKHHosts)fvKHHosts=SEND(SEND(objc_getClass("NSHashTable"),"weakObjectsHashTable"),"retain");
 if(fvKHHosts)SEND(fvKHHosts,"addObject:",host);
}
typedef struct{unsigned previous;} FVKHScope;
static void fvKHClose(FVKHScope *scope){fvKHDepth=scope->previous;}
static void fvKHRecord(unsigned kind,id receiver,id eventScene,Rect plain,Rect iav,
 int relevant,int showing,int notify,id state){
 if(!fvGMain())return;
 unsigned mode=enabled()?1:0;
 if(kind>=5||fvKHCounts[kind][mode]>=10)return;
 fvKHCounts[kind][mode]++;
 unsigned seq=++fvKHSequence,sceneHash=fvKHSceneHash(eventScene);
 char line[700];
 const char *name=receiver?class_getName(object_getClass(receiver)):"nil";
 snprintf(line,sizeof line,"KHROUTE v0.1.21 seq=%u stamp=%.6f mode=%s route=%u receiver=%.100s eventScene=%08x relevant=%d showing=%d notify=%d plain=(%.3f,%.3f,%.3f,%.3f) iav=(%.3f,%.3f,%.3f,%.3f)",seq,CFAbsoluteTimeGetCurrent(),mode?"ratio9x16":"native",kind,name?name:"unknown",sceneHash,relevant,showing,notify,plain.origin.x,plain.origin.y,plain.size.width,plain.size.height,iav.origin.x,iav.origin.y,iav.size.width,iav.size.height);trace(line);
 if(state){
  int hasPlain=fvKHABI(state,"keyboardPosition","{CGRect={CGPoint=dd}{CGSize=dd}}@:");
  int hasIAV=fvKHABI(state,"keyboardPositionWithIAV","{CGRect={CGPoint=dd}{CGSize=dd}}@:");
  Rect a=hasPlain?rect(state,"keyboardPosition"):(Rect){{0,0},{0,0}};
  Rect b=hasIAV?rect(state,"keyboardPositionWithIAV"):(Rect){{0,0},{0,0}};
  snprintf(line,sizeof line,"KHSTATE seq=%u plainReadable=%d iavReadable=%d plain=(%.3f,%.3f,%.3f,%.3f) iav=(%.3f,%.3f,%.3f,%.3f)",seq,hasPlain,hasIAV,a.origin.x,a.origin.y,a.size.width,a.size.height,b.origin.x,b.origin.y,b.size.width,b.size.height);trace(line);
 }
 id hosts=fvKHHosts?SEND(fvKHHosts,"allObjects"):0;U count=hosts?(U)SEND(hosts,"count"):0;
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
  snprintf(line,sizeof line,"KHHOST seq=%u bundle=%.180s hostScene=%08x eventSceneHashMatches=%d settings=(%.3f,%.3f,%.3f,%.3f) cached=(%.3f,%.3f) remote=(%.3f,%.3f) mounted=%d",seq,bid?bid:"unknown",hash,hash&&sceneHash&&hash==sceneHash,frame.origin.x,frame.origin.y,frame.size.width,frame.size.height,c.size.width,c.size.height,rb.size.width,rb.size.height,remote&&SEND(remote,"superview")==host);trace(line);live++;
 }
 if(!live&&fvKHMissingBudget){fvKHMissingBudget--;trace("KHHOST no currently attached tracked floating host; route is observation only");}
}
static BOOL fvKHShow(id self,SEL cmd,id inputViews,id placement,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<0;
 if(!(scope.previous&(1u<<0)))fvKHRecord(0,self,scene,(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,0);
 return ((BOOL(*)(id,SEL,id,id,id))fvKHShowOriginal)(self,cmd,inputViews,placement,scene);
}
static void fvKHArbiter(id self,SEL cmd,Rect plain,Rect iav,BOOL relevant,BOOL showing,BOOL notify,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<1;
 if(!(scope.previous&(1u<<1)))fvKHRecord(1,self,scene,plain,iav,relevant,showing,notify,0);
 ((void(*)(id,SEL,Rect,Rect,BOOL,BOOL,BOOL,id))fvKHArbiterOriginal)(self,cmd,plain,iav,relevant,showing,notify,scene);
}
static void fvKHPrepare(id self,SEL cmd,Rect plain,Rect iav,BOOL relevant,BOOL showing,BOOL notify,id scene){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<2;
 if(!(scope.previous&(1u<<2)))fvKHRecord(2,self,scene,plain,iav,relevant,showing,notify,0);
 ((void(*)(id,SEL,Rect,Rect,BOOL,BOOL,BOOL,id))fvKHPrepareOriginal)(self,cmd,plain,iav,relevant,showing,notify,scene);
}
static void fvKHSignal(id self,SEL cmd,id state,id completion){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<3;
 if(!(scope.previous&(1u<<3)))fvKHRecord(3,self,fvKHObject(self,"requiredScene"),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,state);
 ((void(*)(id,SEL,id,id))fvKHSignalOriginal)(self,cmd,state,completion);
}
static void fvKHQueue(id self,SEL cmd,id state,id completion){
 FVKHScope scope __attribute__((cleanup(fvKHClose)))={fvKHDepth};fvKHDepth|=1u<<4;
 if(!(scope.previous&(1u<<4)))fvKHRecord(4,self,fvKHObject(self,"requiredScene"),(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},-1,-1,-1,state);
 ((void(*)(id,SEL,id,id))fvKHQueueOriginal)(self,cmd,state,completion);
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
 const char *objectSelectors[]={"signalToProxyKeyboardChanged:onCompletion:","queue_keyboardChanged:onComplete:"};
 for(unsigned k=0;k<2;k++){
  if(fvKHHook("_UIKeyboardArbiterClientInputDestination",objectSelectors[k],"v@:@@?",k?(IMP)fvKHQueue:(IMP)fvKHSignal,k?&fvKHQueueOriginal:&fvKHSignalOriginal))installed|=8u<<k;
 }
 char line[300];snprintf(line,sizeof line,"KHROUTE v0.1.21 installedMask=%u show=%u updateArbiter=%u prepareScene=%u signal=%u queue=%u; SpringBoard-only observation; no target-app component, no geometry rewrite",installed,!!(installed&1),!!(installed&2),!!(installed&4),!!(installed&8),!!(installed&16));trace(line);
}
#endif
