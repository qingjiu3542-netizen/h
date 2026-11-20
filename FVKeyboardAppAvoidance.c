/* Opt-in application-process adapter. Observe real UIKit keyboard events,
 * normalize only an exact authorized portrait Scene, forward once. */
typedef void *id;typedef void *SEL;typedef void *Class;typedef void *Method;typedef void (*IMP)(void);
typedef unsigned long U;typedef signed char BOOL;
typedef struct{double x,y;} Point;typedef struct{double width,height;} Size;
typedef struct{Point origin;Size size;} Rect;
extern id objc_msgSend(id,SEL,...);
extern Class objc_getClass(const char*);extern Class object_getClass(id);extern SEL sel_registerName(const char*);
extern Class objc_allocateClassPair(Class,const char*,unsigned long);extern void objc_registerClassPair(Class);extern void objc_disposeClassPair(Class);
extern Method class_getInstanceMethod(Class,SEL);extern IMP method_getImplementation(Method);
extern const char *method_getTypeEncoding(Method);extern BOOL class_addMethod(Class,SEL,IMP,const char*);
extern IMP class_replaceMethod(Class,SEL,IMP,const char*);
extern void dispatch_async_f(void*,void*,void(*)(void*));extern void *_dispatch_main_q;
extern unsigned long long dispatch_time(unsigned long long,long long);
extern void dispatch_after_f(unsigned long long,void*,void*,void(*)(void*));
extern double CFAbsoluteTimeGetCurrent(void);extern void NSLog(id,...);
extern id NSHomeDirectory(void);extern int getpid(void);
extern int open(const char*,int,...);extern long write(int,const void*,U);extern int close(int);
extern long read(int,void*,U);extern int *__error(void);
extern U strlen(const char*);extern int snprintf(char*,U,const char*,...);
#include "FVKeyboardAppFrameCore.h"
#include "FVKeyboardAppMeasuredCore.h"
#define S(v) sel_registerName(v)
#define M0(o,s) ((id(*)(id,SEL))objc_msgSend)(o,S(s))
#define M1(o,s,a) ((id(*)(id,SEL,id))objc_msgSend)(o,S(s),(id)(a))
#define M2(o,s,a,b) ((id(*)(id,SEL,id,id))objc_msgSend)(o,S(s),(id)(a),(id)(b))
#define M3(o,s,a,b,c) ((id(*)(id,SEL,id,id,id))objc_msgSend)(o,S(s),(id)(a),(id)(b),(id)(c))
static id text(const char*s){return M1(objc_getClass("NSString"),"stringWithUTF8String:",s);}
static int responds(id o,const char*s){return o&&((BOOL(*)(id,SEL,SEL))objc_msgSend)(o,S("respondsToSelector:"),S(s));}
static int isKind(id o,const char*s){Class c=objc_getClass(s);return o&&c&&((BOOL(*)(id,SEL,Class))objc_msgSend)(o,S("isKindOfClass:"),c);}
static int equal(id a,id b){return a&&b&&((BOOL(*)(id,SEL,id))objc_msgSend)(a,S("isEqualToString:"),b);}
static id get(id o,const char*k){return M1(o,"objectForKey:",text(k));}
static double number(id o){return responds(o,"doubleValue")?((double(*)(id,SEL))objc_msgSend)(o,S("doubleValue")):0;}
static Rect rect(id o,const char*s){return ((Rect(*)(id,SEL))objc_msgSend)(o,S(s));}
static FVKARect core(Rect r){return (FVKARect){r.origin.x,r.origin.y,r.size.width,r.size.height};}
static IMP oldNamed,oldNote;
static int eventKind(id name);
static int readBox(id value,Rect *out);
static _Thread_local unsigned forwarding;
static unsigned reportBudget=16,reportOnceMask,observedEventCount,rawFrameBudget=6,mappedReportBudget=4;
static id eventProbeObserver;
static void appLog(const char*s){
 id path=M1(NSHomeDirectory(),"stringByAppendingPathComponent:",text("Library/Caches/FVKeyboardApp.log"));
 const char *p=((const char*(*)(id,SEL))objc_msgSend)(path,S("UTF8String"));if(!p)return;
 int fd=open(p,0x209,0600);if(fd>=0){
  id bundle=M0(M0(objc_getClass("NSBundle"),"mainBundle"),"bundleIdentifier");
  const char *bid=bundle?((const char*(*)(id,SEL))objc_msgSend)(bundle,S("UTF8String")):"unknown";
  char line[1024];int length=snprintf(line,sizeof line,"[v=0.1.19 pid=%d stamp=%.3f bundle=%.200s] %.600s\n",getpid(),CFAbsoluteTimeGetCurrent(),bid?bid:"unknown",s);
  if(length>0&&(U)length<sizeof line)write(fd,line,(U)length);
  close(fd);
 }
}
static void report(const char*s){if(reportBudget){reportBudget--;appLog(s);NSLog(text("[FVKeyboardApp] %s"),s);}}
static void reportOnce(unsigned key,const char*s){unsigned bit=1u<<key;if(!(reportOnceMask&bit)){reportOnceMask|=bit;report(s);}}
static void reportMapped(const char*s){if(mappedReportBudget){mappedReportBudget--;appLog(s);NSLog(text("[FVKeyboardApp] %s"),s);}}
static void reportHookEntry(id name,id center,int namedRoute){
 if(!eventKind(name))return;
 id main=M0(objc_getClass("NSNotificationCenter"),"defaultCenter");
 reportOnce(namedRoute?11:10,center==main?
  (namedRoute?"keyboard notification entered named-post hook on default center":"keyboard notification entered object-post hook on default center"):
  (namedRoute?"keyboard notification entered named-post hook on custom center":"keyboard notification entered object-post hook on custom center"));
}
static void keyboardObserved(id observer,SEL cmd,id note){
 (void)observer;(void)cmd;id name=M0(note,"name");if(!eventKind(name))return;
 unsigned index=__atomic_fetch_add(&observedEventCount,1,__ATOMIC_RELAXED);if(index>=8)return;
 id info=M0(note,"userInfo"),object=M0(note,"object");Rect end;
 if(readBox(get(info,"UIKeyboardFrameEndUserInfoKey"),&end)){
  char line[256];snprintf(line,sizeof line,"UIKit keyboard notification observed n=%u route=observer objectIsScreen=%d local=%d frame=(%.2f,%.2f,%.2f,%.2f)",index+1,isKind(object,"UIScreen"),number(get(info,"UIKeyboardIsLocalUserInfoKey"))==1,end.origin.x,end.origin.y,end.size.width,end.size.height);report(line);
 }else report("UIKit keyboard notification observed; end frame is not readable CGRect");
}
static void installObserver(id center){
 if(eventProbeObserver)return;
 Class c=objc_allocateClassPair(objc_getClass("NSObject"),"FVKeyboardAppEventProbeV2",0);
 if(!c){report("keyboard event observer install failed: class allocation");return;}
 if(!class_addMethod(c,S("keyboardObserved:"),(IMP)keyboardObserved,"v@:@")){objc_disposeClassPair(c);report("keyboard event observer install failed: selector registration");return;}
 objc_registerClassPair(c);eventProbeObserver=((id(*)(id,SEL))objc_msgSend)(c,S("new"));if(!eventProbeObserver)return;
 const char*names[]={"UIKeyboardWillShowNotification","UIKeyboardDidShowNotification","UIKeyboardWillChangeFrameNotification","UIKeyboardDidChangeFrameNotification"};
 for(unsigned i=0;i<4;i++)((void(*)(id,SEL,id,SEL,id,id))objc_msgSend)(center,S("addObserver:selector:name:object:"),eventProbeObserver,S("keyboardObserved:"),text(names[i]),(id)0);
 report("UIKit keyboard notification observer installed for frame diagnostics");
}
static int encoding(const char *actual,const char *expected){
 if(!actual)return 0;char out[128];unsigned n=0;int depth=0,quoted=0;
 for(;*actual;actual++){
  char c=*actual;if(c=='\"'){quoted=!quoted;continue;}if(quoted)continue;
  if(!depth&&c>='0'&&c<='9')continue;
  if(n+1>=sizeof out)return 0;out[n++]=c;
  if(c=='{')depth++;else if(c=='}')depth--;
 }
 out[n]=0;return !depth&&!quoted&&!__builtin_strcmp(out,expected);
}
static int eventKind(id name){
 if(equal(name,text("UIKeyboardWillShowNotification"))||equal(name,text("UIKeyboardDidShowNotification"))||
    equal(name,text("UIKeyboardWillChangeFrameNotification"))||equal(name,text("UIKeyboardDidChangeFrameNotification")))return 1;
 return 0;
}
static id appCachePath(const char **reason){
 id home=NSHomeDirectory();if(!home||!responds(home,"stringByAppendingPathComponent:")){if(reason)*reason="application home directory unavailable";return 0;}
 id library=M1(home,"stringByAppendingPathComponent:",text("Library"));
 id caches=M1(library,"stringByAppendingPathComponent:",text("Caches"));
 id path=M1(caches,"stringByAppendingPathComponent:",text("com.minis.floatingview.keyboard-layout.plist"));
 if(reason)*reason="resolved";return path;
}
static id mainBundleID(void){
 id bundle=M0(objc_getClass("NSBundle"),"mainBundle");
 id identifier=bundle&&responds(bundle,"bundleIdentifier")?M0(bundle,"bundleIdentifier"):0;
 return isKind(identifier,"NSString")?identifier:0;
}
/* An optional same-bundle canonical path, never a container scan. Home remains
 * authoritative when present, including disabled or malformed tickets. */
static int appObjectABI(id object,const char *selector,const char *types){
 Method m=object?class_getInstanceMethod(object_getClass(object),S(selector)):0;
 return m&&encoding(method_getTypeEncoding(m),types);
}
static id appProxyCachePath(id bundle){
 static id cached;static double attempted=-10;
 if(cached)return cached;double now=CFAbsoluteTimeGetCurrent();if(now-attempted<5)return 0;attempted=now;
 Class cls=objc_getClass("LSApplicationProxy");if(!cls||!appObjectABI(cls,"applicationProxyForIdentifier:","@@:@")||!bundle)return 0;
 id proxy=M1(cls,"applicationProxyForIdentifier:",bundle);
 if(!appObjectABI(proxy,"applicationIdentifier","@@:")||!appObjectABI(proxy,"dataContainerURL","@@:")||!equal(M0(proxy,"applicationIdentifier"),bundle))return 0;
 id url=M0(proxy,"dataContainerURL");if(!responds(url,"isFileURL")||!((BOOL(*)(id,SEL))objc_msgSend)(url,S("isFileURL"))||!responds(url,"path"))return 0;
 id root=M0(M0(url,"path"),"stringByStandardizingPath");
 const char *p=((const char*(*)(id,SEL))objc_msgSend)(root,S("UTF8String"));if(!p)return 0;
 const char *a="/var/mobile/Containers/Data/Application/",*b="/private/var/mobile/Containers/Data/Application/";U prefix=0;
 if(!__builtin_strncmp(p,a,strlen(a)))prefix=strlen(a);else if(!__builtin_strncmp(p,b,strlen(b)))prefix=strlen(b);else return 0;
 if(!p[prefix])return 0;for(U i=prefix;p[i];i++)if(!((p[i]>='0'&&p[i]<='9')||(p[i]>='a'&&p[i]<='f')||(p[i]>='A'&&p[i]<='F')||p[i]=='-'))return 0;
 id path=M1(root,"stringByAppendingPathComponent:",text("Library/Caches/com.minis.floatingview.keyboard-layout.plist"));
 cached=M0(path,"retain");return cached;
}
static id readState(id appBundle,const char **reason,int *missing){
 if(missing)*missing=0;
 id path=appCachePath(reason);if(!path)return 0;
 const char *file=((const char*(*)(id,SEL))objc_msgSend)(path,S("fileSystemRepresentation"));
 if(!file){if(reason)*reason="per-app ticket path has no filesystem representation";return 0;}
 int fd=open(file,0|0x00000100|0x01000000|0x00000004);
 int openError=fd<0?*__error():0;
 if(fd<0&&openError==2){
  id alternate=appProxyCachePath(appBundle);
  if(alternate&&!equal(alternate,path)){
   const char *other=((const char*(*)(id,SEL))objc_msgSend)(alternate,S("fileSystemRepresentation"));
   if(other){
    fd=open(other,0|0x00000100|0x01000000|0x00000004);openError=fd<0?*__error():0;
    reportOnce(17,fd>=0?"per-app ticket opened from verified same-bundle canonical container":"per-app canonical container differs; ticket still cannot be opened");
   }
  }
 }
 if(fd<0){int e=openError;if(missing)*missing=(e==2);char line[160];snprintf(line,sizeof line,"per-app ticket open failed errno=%d",e);reportOnce(12,line);if(reason)*reason="per-app Scene ticket open failed";return 0;}
 unsigned char bytes[8192];U length=0;int readError=0;
 while(length<sizeof bytes){long n=read(fd,bytes+length,sizeof bytes-length);if(n<0){readError=*__error();break;}if(n==0)break;length+=(U)n;}
 close(fd);
 if(readError||length==0||length==sizeof bytes){char line[160];snprintf(line,sizeof line,"per-app ticket read failed errno=%d bytes=%lu",readError,(unsigned long)length);reportOnce(13,line);if(reason)*reason="per-app Scene ticket read failed or exceeded size limit";return 0;}
 id data=((id(*)(id,SEL,const void*,U))objc_msgSend)(objc_getClass("NSData"),S("dataWithBytes:length:"),bytes,length);
 id error=0,format=0;
 id p=((id(*)(id,SEL,id,U,id,id*))objc_msgSend)(objc_getClass("NSPropertyListSerialization"),S("propertyListWithData:options:format:error:"),data,(U)0,&format,&error);
 if(!p){reportOnce(14,"per-app ticket plist parse failed after successful file read");if(reason)*reason="per-app Scene ticket plist parse failed";return 0;}
 if(!isKind(p,"NSDictionary")){reportOnce(15,"per-app ticket root object is not NSDictionary");if(reason)*reason="per-app Scene ticket root is not dictionary";return 0;}
 id schema=get(p,"schema"),enabledValue=get(p,"enabled"),scene=get(p,"scene"),bundle=get(p,"bundleID");
 if(!isKind(schema,"NSNumber")||!isKind(enabledValue,"NSNumber")||!isKind(scene,"NSString")||!isKind(bundle,"NSString")){
  if(reason)*reason="per-app ticket schema/identity fields missing or invalid";return 0;
 }
 if(number(schema)!=2){if(reason)*reason="per-app ticket schema version mismatch";return 0;}
 if(!appBundle||!equal(bundle,appBundle)){if(reason)*reason="per-app ticket bundle identifier mismatch";return 0;}
 if(number(enabledValue)!=1){if(reason)*reason="per-app Scene ticket is explicitly disabled";return 0;}
 const char *keys[]={"stamp","inner","targetX","targetY","targetWidth","targetHeight","sourceX","sourceY","sourceWidth","sourceHeight"};
 for(unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);i++){
  id v=get(p,keys[i]);if(!isKind(v,"NSNumber")||!__builtin_isfinite(number(v))){if(reason)*reason="per-app ticket contains missing or nonnumeric geometry field";return 0;}
 }
 reportOnce(18,"per-app ticket read and parsed: schema 2, matching bundle, enabled");
 if(reason)*reason="ticket read and parsed";return p;
}

static int liveTargetForScene(id eventObject,id requestedScene,FVKARect *native,FVKARect *target,const char **reason){
#define LIVEFAIL(message) do{if(reason)*reason=message;return 0;}while(0)
 id app=M0(objc_getClass("UIApplication"),"sharedApplication");if(!responds(app,"connectedScenes"))LIVEFAIL("live geometry unavailable: no connectedScenes");
 id set=M0(app,"connectedScenes"),scenes=set?M0(set,"allObjects"):0;U n=scenes?(U)M0(scenes,"count"):0;id chosen=0,win=0;unsigned active=0;
 for(U i=0;i<n;i++){id s=((id(*)(id,SEL,U))objc_msgSend)(scenes,S("objectAtIndex:"),i);if(!isKind(s,"UIWindowScene")||!responds(s,"activationState"))continue;if(((long(*)(id,SEL))objc_msgSend)(s,S("activationState"))!=0)continue;active++;chosen=s;}
 if(active!=1||!chosen||!responds(chosen,"screen")||!responds(chosen,"interfaceOrientation"))LIVEFAIL("live geometry foreground Scene is not unique");
 if(requestedScene&&requestedScene!=chosen)LIVEFAIL("native layout query Scene is not the unique foreground Scene");
 id screen=M0(chosen,"screen");if(!screen||!fvKAFEventScreenMatch(eventObject!=0,eventObject&&isKind(eventObject,"UIScreen"),eventObject==screen))LIVEFAIL("live geometry screen mismatch");
 if(((long(*)(id,SEL))objc_msgSend)(chosen,S("interfaceOrientation"))!=1)LIVEFAIL("live geometry is not portrait");
 id windows=M0(chosen,"windows");if(!windows)LIVEFAIL("live geometry has no windows");n=(U)M0(windows,"count");
 for(U i=0;i<n;i++){id w=((id(*)(id,SEL,U))objc_msgSend)(windows,S("objectAtIndex:"),i);if(((BOOL(*)(id,SEL))objc_msgSend)(w,S("isKeyWindow"))&&!((BOOL(*)(id,SEL))objc_msgSend)(w,S("isHidden"))){if(win)LIVEFAIL("live geometry has multiple key windows");win=w;}}
 if(!win)LIVEFAIL("live geometry has no visible key window");
 FVKARect t=core(rect(win,"bounds")),s=core(rect(screen,"bounds"));
 if(!fvKAFLiveViewportAllowed(active==1,eventObject==0||eventObject==screen,1,1,s,t))LIVEFAIL("live geometry is not an exact portrait 9:16 viewport");
 *native=s;*target=t;if(reason)*reason="authorized from live Scene/window bounds";return 1;
#undef LIVEFAIL
}
static int localTarget(id state,id eventObject,FVKARect *native,FVKARect *target,const char **reason){
#define FAIL(message) do{if(reason)*reason=message;return 0;}while(0)
 id app=M0(objc_getClass("UIApplication"),"sharedApplication");
 if(!responds(app,"connectedScenes"))FAIL("UIApplication has no connectedScenes selector");
 id sceneSet=M0(app,"connectedScenes");id scenes=sceneSet?M0(sceneSet,"allObjects"):0;
 U n=scenes?(U)M0(scenes,"count"):0;if(!n)FAIL("no connected scenes in target app");
 id chosen=0,chosenWindow=0;unsigned active=0;
 for(U i=0;i<n;i++){
  id scene=((id(*)(id,SEL,U))objc_msgSend)(scenes,S("objectAtIndex:"),i);
  if(!isKind(scene,"UIWindowScene")||!responds(scene,"activationState"))continue;
  if(((long(*)(id,SEL))objc_msgSend)(scene,S("activationState"))!=0)continue;
  active++;chosen=scene;
 }
 if(active!=1)FAIL("foreground UIWindowScene count is not one");
 if(!chosen||!responds(chosen,"_scene")||!responds(chosen,"screen"))FAIL("foreground UIWindowScene lacks scene identity or screen");
 id chosenScreen=M0(chosen,"screen");
 if(!chosenScreen)FAIL("foreground UIWindowScene has no screen");
 if(!fvKAFEventScreenMatch(eventObject!=0,eventObject&&isKind(eventObject,"UIScreen"),eventObject==chosenScreen))FAIL("notification object screen differs from target Scene screen");
 if(!responds(chosen,"interfaceOrientation")||((long(*)(id,SEL))objc_msgSend)(chosen,S("interfaceOrientation"))!=1)FAIL("target Scene is not upright portrait");
 id backing=M0(chosen,"_scene");if(!responds(backing,"identifier"))FAIL("backing Scene has no identifier");
 if(!equal(M0(backing,"identifier"),get(state,"scene")))FAIL("Scene identifier does not match ticket");
 id windows=M0(chosen,"windows");if(!windows)FAIL("target Scene has no windows array");n=(U)M0(windows,"count");
 for(U i=0;i<n;i++){
  id w=((id(*)(id,SEL,U))objc_msgSend)(windows,S("objectAtIndex:"),i);
  if(((BOOL(*)(id,SEL))objc_msgSend)(w,S("isKeyWindow"))&&!((BOOL(*)(id,SEL))objc_msgSend)(w,S("isHidden"))){
   if(chosenWindow)FAIL("multiple visible key windows in target Scene");chosenWindow=w;
  }
 }
 if(!chosenWindow)FAIL("target Scene has no visible key window");
 FVKARect t={number(get(state,"targetX")),number(get(state,"targetY")),number(get(state,"targetWidth")),number(get(state,"targetHeight"))};
 FVKARect s={number(get(state,"sourceX")),number(get(state,"sourceY")),number(get(state,"sourceWidth")),number(get(state,"sourceHeight"))};
 FVKARect local=core(rect(chosenWindow,"bounds"));
 if(!fvKAFRect(t)||!fvKAFNear(t.height,t.width*16.0/9.0))FAIL("ticket target rectangle invalid or not 9:16");
 if(!fvKAFNear(local.x,t.x)||!fvKAFNear(local.y,t.y)||!fvKAFNear(local.width,t.width)||!fvKAFNear(local.height,t.height))FAIL("key-window bounds differ from ticket target rectangle");
 double now=CFAbsoluteTimeGetCurrent(),stamp=number(get(state,"stamp"));
 if(!__builtin_isfinite(now)||!__builtin_isfinite(stamp)||stamp<0||now<stamp||now-stamp>120)FAIL("Scene ticket timestamp invalid, future-dated, or older than 120 seconds");
 if(!fvKAFAuthorize(1,1,1,1,number(get(state,"inner"))==1,now,stamp,local,t))FAIL("Scene authorization core rejected ticket");
 Rect screen=rect(chosenScreen,"bounds");
 if(!fvKAFRect(core(screen))||!fvKAFNear(screen.size.width,s.width)||!fvKAFNear(screen.size.height,s.height))FAIL("current screen bounds differ from ticket source rectangle");
 *native=s;*target=t;if(reason)*reason="authorized";return 1;
#undef FAIL
}

static int readBox(id value,Rect *out){
 if(!isKind(value,"NSValue")||!responds(value,"objCType")||!responds(value,"CGRectValue"))return 0;
 const char *e=((const char*(*)(id,SEL))objc_msgSend)(value,S("objCType"));
 if(!e||__builtin_strcmp(e,"{CGRect={CGPoint=dd}{CGSize=dd}}"))return 0;
 *out=rect(value,"CGRectValue");return fvKAFRect(core(*out));
}
static int measuredEvidence(id state,FVKARect source,FVKARect target,FVKARect *keys,double *sample,double *packet,double *sequence){
 const char *fields[]={"keysVersion","keysSequence","keysStamp","keysX","keysY","keysWidth","keysHeight"};
 for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0]);i++){
  id value=get(state,fields[i]);if(!isKind(value,"NSNumber")||!__builtin_isfinite(number(value)))return 0;
 }
 if(number(get(state,"keysVersion"))!=1)return 0;
 *keys=(FVKARect){number(get(state,"keysX")),number(get(state,"keysY")),number(get(state,"keysWidth")),number(get(state,"keysHeight"))};
 *sample=number(get(state,"keysStamp"));*packet=number(get(state,"stamp"));*sequence=number(get(state,"keysSequence"));
 return fvKAFMeasuredFresh(source,target,*keys,CFAbsoluteTimeGetCurrent(),*sample,*packet,*sequence);
}
#include "FVKeyboardAppFirstEvent.inc"
static id mappedInfo(id name,id object,id userInfo){
 if(!eventKind(name))return 0;
 if(!isKind(userInfo,"NSDictionary")){reportOnce(0,"real keyboard event unchanged: userInfo is not NSDictionary");return 0;}
 if(object&&!isKind(object,"UIScreen")){reportOnce(1,"real keyboard event unchanged: notification object is neither nil nor UIScreen");return 0;}
 id local=get(userInfo,"UIKeyboardIsLocalUserInfoKey");if(!local||number(local)!=1){reportOnce(2,"real keyboard event unchanged: local-keyboard marker missing or false");return 0;}
 if(!((BOOL(*)(id,SEL))objc_msgSend)(objc_getClass("NSThread"),S("isMainThread"))){reportOnce(3,"real keyboard event unchanged: notification is not on main thread");return 0;}
 id appBundle=mainBundleID();if(!appBundle){reportOnce(9,"application bundle identifier unavailable");return 0;}
 const char *ticketReason=0;int missing=0;id state=readState(appBundle,&ticketReason,&missing);FVKARect source,target;Rect end;FVKARect mapped;
 if(!readBox(get(userInfo,"UIKeyboardFrameEndUserInfoKey"),&end)){reportOnce(6,"real keyboard event unchanged: end frame is not supported CGRect");return 0;}
 if(rawFrameBudget){rawFrameBudget--;char line[240];snprintf(line,sizeof line,"native keyboard event end-frame=(%.2f,%.2f,%.2f,%.2f) objectScreen=%d local=%d",end.origin.x,end.origin.y,end.size.width,end.size.height,isKind(object,"UIScreen"),number(local)==1);report(line);}
 if(!state){
  if(!missing){reportOnce(4,ticketReason?ticketReason:"per-app ticket denied; live fallback not permitted");return 0;}
  const char *liveReason=0;FVKARect liveSource,liveRect;
  if(!liveTargetForScene(object,0,&liveSource,&liveRect,&liveReason)){reportOnce(16,liveReason?liveReason:"per-app ticket unavailable and live geometry rejected");return 0;}
  if(!fvKAFMap(liveSource,liveRect,core(end),&mapped))return 0;
  id changed=M0(userInfo,"mutableCopy");if(!changed)return 0;
  Rect next={{mapped.x,mapped.y},{mapped.width,mapped.height}};
  id box=((id(*)(id,SEL,Rect))objc_msgSend)(objc_getClass("NSValue"),S("valueWithCGRect:"),next);
  M2(changed,"setObject:forKey:",box,text("UIKeyboardFrameEndUserInfoKey"));
  Rect begin;FVKARect mappedBegin;
  if(readBox(get(userInfo,"UIKeyboardFrameBeginUserInfoKey"),&begin)&&fvKAFMapBegin(liveSource,liveRect,core(begin),&mappedBegin)){
   Rect r={{mappedBegin.x,mappedBegin.y},{mappedBegin.width,mappedBegin.height}};
   box=((id(*)(id,SEL,Rect))objc_msgSend)(objc_getClass("NSValue"),S("valueWithCGRect:"),r);
   M2(changed,"setObject:forKey:",box,text("UIKeyboardFrameBeginUserInfoKey"));
  }
  if(mappedReportBudget){char line[256];snprintf(line,sizeof line,"real keyboard event mapped live-geometry source=(%.2f,%.2f,%.2f,%.2f) target=(%.2f,%.2f,%.2f,%.2f)",end.origin.x,end.origin.y,end.size.width,end.size.height,next.origin.x,next.origin.y,next.size.width,next.size.height);reportMapped(line);}
  return changed;
 }
 const char *guardReason=0;
 if(!localTarget(state,object,&source,&target,&guardReason)){reportOnce(8,guardReason?guardReason:"Scene/window/orientation/size/freshness guard failed");return 0;}
 FVKARect measuredKeys={0};double measuredStamp=0,packetStamp=0,keySequence=0;
 int hasMeasured=measuredEvidence(state,source,target,&measuredKeys,&measuredStamp,&packetStamp,&keySequence);
 int measuredKind=0;double now=CFAbsoluteTimeGetCurrent();int firstRoute=0;
 FVKARect synchronousKeys={0};
 if(sameEventEvidence(state,object,userInfo,source,target,core(end),&synchronousKeys,&firstRoute))measuredKeys=synchronousKeys;
 if(firstRoute){
  measuredKind=fvKAFMeasuredShape(source,target,measuredKeys,core(end));
  if((measuredKind!=1&&measuredKind!=2)||!fvKAFMap(source,target,measuredKeys,&mapped))return 0;
 }else if(hasMeasured){
  measuredKind=fvKAFMapMeasured(source,target,measuredKeys,core(end),now,measuredStamp,packetStamp,keySequence,&mapped);
  if(!measuredKind){reportOnce(19,"measured keyboard event unchanged: frame does not match current full/truncated key geometry or is already correct");return 0;}
 }else{
  reportOnce(20,"complete-height repair waiting: no fresh successful-dock measurement; legacy frame mapping only");
  if(!fvKAFMap(source,target,core(end),&mapped)){reportOnce(7,"real keyboard event unchanged: end frame is not a full-width docked native-canvas keyboard");return 0;}
 }
 id changed=M0(userInfo,"mutableCopy");if(!changed)return 0;
 Rect next={{mapped.x,mapped.y},{mapped.width,mapped.height}};
 id box=((id(*)(id,SEL,Rect))objc_msgSend)(objc_getClass("NSValue"),S("valueWithCGRect:"),next);
 M2(changed,"setObject:forKey:",box,text("UIKeyboardFrameEndUserInfoKey"));
 Rect begin;FVKARect mappedBegin;
 int beginOK=0;
 if(readBox(get(userInfo,"UIKeyboardFrameBeginUserInfoKey"),&begin)){
  if(firstRoute)beginOK=fvKAFFirstEventBegin(source,target,measuredKeys,core(begin),&mappedBegin);
  else beginOK=hasMeasured?fvKAFMapMeasuredBegin(source,target,measuredKeys,core(begin),now,measuredStamp,packetStamp,keySequence,&mappedBegin):fvKAFMapBegin(source,target,core(begin),&mappedBegin);
 }
 if(beginOK){
  Rect r={{mappedBegin.x,mappedBegin.y},{mappedBegin.width,mappedBegin.height}};
  box=((id(*)(id,SEL,Rect))objc_msgSend)(objc_getClass("NSValue"),S("valueWithCGRect:"),r);
  M2(changed,"setObject:forKey:",box,text("UIKeyboardFrameBeginUserInfoKey"));
 }
 if(firstRoute&&firstSuccessBudget){
  firstSuccessBudget--;char line[350];int will=equal(name,text("UIKeyboardWillShowNotification"))||equal(name,text("UIKeyboardWillChangeFrameNotification"));
  snprintf(line,sizeof line,"first-event complete-height mapped route=%s will=%d source=(%.2f,%.2f,%.2f,%.2f) target=(%.2f,%.2f,%.2f,%.2f); native timing preserved",firstRoute==2?"native-preparation":"same-notification-bounds",will,end.origin.x,end.origin.y,end.size.width,end.size.height,next.origin.x,next.origin.y,next.size.width,next.size.height);appLog(line);
 }
 if(measuredKind==2&&!firstRoute){
  static unsigned completeBudget=8;if(completeBudget){completeBudget--;char line[320];
   snprintf(line,sizeof line,"keyboard complete-height mapped source=(%.2f,%.2f,%.2f,%.2f) target=(%.2f,%.2f,%.2f,%.2f) keysStamp=%.3f sequence=%.0f",end.origin.x,end.origin.y,end.size.width,end.size.height,next.origin.x,next.origin.y,next.size.width,next.size.height,measuredStamp,keySequence);appLog(line);
  }
 }
 if(mappedReportBudget){
  char line[256];snprintf(line,sizeof line,"real keyboard event mapped source=(%.2f,%.2f,%.2f,%.2f) target=(%.2f,%.2f,%.2f,%.2f); native timing preserved",end.origin.x,end.origin.y,end.size.width,end.size.height,next.origin.x,next.origin.y,next.size.width,next.size.height);
  reportMapped(line);
 }
 return changed; /* caller owns; all unrelated native keys are preserved */
}
static void namedHook(id center,SEL cmd,id name,id object,id info){
 reportHookEntry(name,center,1);
 if(forwarding||center!=M0(objc_getClass("NSNotificationCenter"),"defaultCenter")){
  ((void(*)(id,SEL,id,id,id))oldNamed)(center,cmd,name,object,info);return;
 }
 forwarding++;
 id mapped=mappedInfo(name,object,info);
 ((void(*)(id,SEL,id,id,id))oldNamed)(center,cmd,name,object,mapped?mapped:info);
 if(mapped)M0(mapped,"release");
 forwarding--;
}
static void noteHook(id center,SEL cmd,id note){
 id name=M0(note,"name");reportHookEntry(name,center,0);
 if(forwarding||center!=M0(objc_getClass("NSNotificationCenter"),"defaultCenter")){
  ((void(*)(id,SEL,id))oldNote)(center,cmd,note);return;
 }
 forwarding++;
 id object=M0(note,"object"),mapped=mappedInfo(name,object,M0(note,"userInfo"));
 id replacement=mapped?M3(objc_getClass("NSNotification"),"notificationWithName:object:userInfo:",name,object,mapped):0;
 ((void(*)(id,SEL,id))oldNote)(center,cmd,replacement?replacement:note);
 if(mapped)M0(mapped,"release");
 forwarding--;
}
static int hook(Class cls,const char *selector,const char *types,IMP replacement,IMP *old){
 Method m=class_getInstanceMethod(cls,S(selector));if(!m||!encoding(method_getTypeEncoding(m),types))return 0;
 *old=method_getImplementation(m);
 if(!class_addMethod(cls,S(selector),replacement,method_getTypeEncoding(m)))class_replaceMethod(cls,S(selector),replacement,method_getTypeEncoding(m));
 return 1;
}
#include "FVKeyboardAppNativeLayout.inc"
static void install(void *unused){
 (void)unused;
 id bundle=M0(M0(objc_getClass("NSBundle"),"mainBundle"),"bundleIdentifier");
 if(equal(bundle,text("com.apple.springboard"))||equal(bundle,text("com.apple.Preferences")))return;
 id defaultCenter=M0(objc_getClass("NSNotificationCenter"),"defaultCenter");
 if(!defaultCenter){report("adapter install failed: default notification center unavailable");return;}
 Class center=object_getClass(defaultCenter);if(!center){report("adapter install failed: notification-center class unavailable");return;}
 installFirstEvent();
 installNativeLayout();
 installObserver(defaultCenter);
 Method a=class_getInstanceMethod(center,S("postNotificationName:object:userInfo:"));
 Method b=class_getInstanceMethod(center,S("postNotification:"));
 if(!a||!b){report("adapter install failed: notification posting selector unavailable");return;}
 if(!encoding(method_getTypeEncoding(a),"v@:@@@")||!encoding(method_getTypeEncoding(b),"v@:@")){report("adapter install failed: notification selector ABI mismatch");return;}
 int namedOK=hook(center,"postNotificationName:object:userInfo:","v@:@@@",(IMP)namedHook,&oldNamed);
 int noteOK=hook(center,"postNotification:","v@:@",(IMP)noteHook,&oldNote);
 if(!namedOK||!noteOK){report("adapter install failed: notification hook registration failed");return;}
 report("portrait adapter installed; per-app Caches ticket schema 2; exact Scene/screen required; landscape unchanged");
}
__attribute__((constructor))static void start(void){dispatch_async_f(&_dispatch_main_q,0,install);}
