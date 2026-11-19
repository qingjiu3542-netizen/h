/* Opt-in application-process adapter. Observe real UIKit keyboard events,
 * normalize only an exact authorized portrait Scene, forward once. */
typedef void *id;typedef void *SEL;typedef void *Class;typedef void *Method;typedef void (*IMP)(void);
typedef unsigned long U;typedef signed char BOOL;
typedef struct{double x,y;} Point;typedef struct{double width,height;} Size;
typedef struct{Point origin;Size size;} Rect;
extern id objc_msgSend(id,SEL,...);
extern Class objc_getClass(const char*);extern Class object_getClass(id);extern SEL sel_registerName(const char*);
extern Method class_getInstanceMethod(Class,SEL);extern IMP method_getImplementation(Method);
extern const char *method_getTypeEncoding(Method);extern BOOL class_addMethod(Class,SEL,IMP,const char*);
extern IMP class_replaceMethod(Class,SEL,IMP,const char*);
extern void dispatch_async_f(void*,void*,void(*)(void*));extern void *_dispatch_main_q;
extern unsigned long long dispatch_time(unsigned long long,long long);
extern void dispatch_after_f(unsigned long long,void*,void*,void(*)(void*));
extern double CFAbsoluteTimeGetCurrent(void);extern void NSLog(id,...);
extern id NSHomeDirectory(void);
extern int open(const char*,int,...);extern long write(int,const void*,U);extern int close(int);
extern U strlen(const char*);extern int snprintf(char*,U,const char*,...);
#include "FVKeyboardAppFrameCore.h"
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
static _Thread_local unsigned forwarding;
static unsigned reportBudget=16;
static const char *statePath="/var/mobile/Library/Preferences/com.minis.floatingview.keyboard-layout.plist";
static void appLog(const char*s){
 id path=M1(NSHomeDirectory(),"stringByAppendingPathComponent:",text("Library/Caches/FVKeyboardApp.log"));
 const char *p=((const char*(*)(id,SEL))objc_msgSend)(path,S("UTF8String"));if(!p)return;
 int fd=open(p,0x209,0600);if(fd>=0){write(fd,s,strlen(s));write(fd,"\n",1);close(fd);}
}
static void report(const char*s){if(reportBudget){reportBudget--;appLog(s);NSLog(text("[FVKeyboardApp] %s"),s);}}
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
static id readState(void){
 id p=M1(objc_getClass("NSDictionary"),"dictionaryWithContentsOfFile:",text(statePath));
 if(!isKind(p,"NSDictionary"))return 0;
 if(number(get(p,"schema"))!=1||number(get(p,"enabled"))!=1)return 0;
 return p;
}
static int localTarget(id state,id source,FVKARect *native,FVKARect *target){
 if(!isKind(source,"UIScreen"))return 0;
 id app=M0(objc_getClass("UIApplication"),"sharedApplication");
 if(!responds(app,"connectedScenes"))return 0;
 id scenes=M0(M0(app,"connectedScenes"),"allObjects");U n=(U)M0(scenes,"count");
 id chosen=0,chosenWindow=0;unsigned active=0;
 for(U i=0;i<n;i++){
  id scene=((id(*)(id,SEL,U))objc_msgSend)(scenes,S("objectAtIndex:"),i);
  if(!isKind(scene,"UIWindowScene")||!responds(scene,"activationState"))continue;
  if(((long(*)(id,SEL))objc_msgSend)(scene,S("activationState"))!=0)continue;
  active++;chosen=scene;
 }
 if(active!=1||!chosen||!responds(chosen,"_scene")||!responds(chosen,"screen")||M0(chosen,"screen")!=source)return 0;
 if(!responds(chosen,"interfaceOrientation")||((long(*)(id,SEL))objc_msgSend)(chosen,S("interfaceOrientation"))!=1)return 0;
 id backing=M0(chosen,"_scene");if(!responds(backing,"identifier"))return 0;
 if(!equal(M0(backing,"identifier"),get(state,"scene")))return 0;
 id windows=M0(chosen,"windows");n=(U)M0(windows,"count");
 for(U i=0;i<n;i++){
  id w=((id(*)(id,SEL,U))objc_msgSend)(windows,S("objectAtIndex:"),i);
  if(((BOOL(*)(id,SEL))objc_msgSend)(w,S("isKeyWindow"))&&!((BOOL(*)(id,SEL))objc_msgSend)(w,S("isHidden"))){
   if(chosenWindow)return 0;chosenWindow=w;
  }
 }
 if(!chosenWindow)return 0;
 FVKARect t={number(get(state,"targetX")),number(get(state,"targetY")),number(get(state,"targetWidth")),number(get(state,"targetHeight"))};
 FVKARect s={number(get(state,"sourceX")),number(get(state,"sourceY")),number(get(state,"sourceWidth")),number(get(state,"sourceHeight"))};
 if(!fvKAFAuthorize(1,1,1,1,number(get(state,"inner"))==1,CFAbsoluteTimeGetCurrent(),number(get(state,"stamp")),core(rect(chosenWindow,"bounds")),t))return 0;
 Rect screen=rect(source,"bounds");
 if(!fvKAFNear(screen.size.width,s.width)||!fvKAFNear(screen.size.height,s.height))return 0;
 *native=s;*target=t;return 1;
}
static int readBox(id value,Rect *out){
 if(!isKind(value,"NSValue")||!responds(value,"objCType")||!responds(value,"CGRectValue"))return 0;
 const char *e=((const char*(*)(id,SEL))objc_msgSend)(value,S("objCType"));
 if(!e||__builtin_strcmp(e,"{CGRect={CGPoint=dd}{CGSize=dd}}"))return 0;
 *out=rect(value,"CGRectValue");return fvKAFRect(core(*out));
}
static id mappedInfo(id name,id object,id userInfo){
 if(!eventKind(name)||!isKind(userInfo,"NSDictionary")||!isKind(object,"UIScreen"))return 0;
 id local=get(userInfo,"UIKeyboardIsLocalUserInfoKey");if(!local||number(local)!=1)return 0;
 if(!((BOOL(*)(id,SEL))objc_msgSend)(objc_getClass("NSThread"),S("isMainThread")))return 0;
 id state=readState();FVKARect source,target;
 if(!state){report("real keyboard event unchanged: no readable enabled Scene ticket");return 0;}
 if(!localTarget(state,object,&source,&target)){report("real keyboard event unchanged: Scene/window/orientation/size/freshness guard failed");return 0;}
 Rect end;FVKARect mapped;
 if(!readBox(get(userInfo,"UIKeyboardFrameEndUserInfoKey"),&end)||!fvKAFMap(source,target,core(end),&mapped))return 0;
 id changed=M0(userInfo,"mutableCopy");if(!changed)return 0;
 Rect next={{mapped.x,mapped.y},{mapped.width,mapped.height}};
 id box=((id(*)(id,SEL,Rect))objc_msgSend)(objc_getClass("NSValue"),S("valueWithCGRect:"),next);
 M2(changed,"setObject:forKey:",box,text("UIKeyboardFrameEndUserInfoKey"));
 Rect begin;FVKARect mappedBegin;
 if(readBox(get(userInfo,"UIKeyboardFrameBeginUserInfoKey"),&begin)&&fvKAFMapBegin(source,target,core(begin),&mappedBegin)){
  Rect r={{mappedBegin.x,mappedBegin.y},{mappedBegin.width,mappedBegin.height}};
  box=((id(*)(id,SEL,Rect))objc_msgSend)(objc_getClass("NSValue"),S("valueWithCGRect:"),r);
  M2(changed,"setObject:forKey:",box,text("UIKeyboardFrameBeginUserInfoKey"));
 }
 if(reportBudget){
  char line[256];snprintf(line,sizeof line,"real keyboard event mapped nativeTop=%.2f targetTop=%.2f height=%.2f; Scene matched; native timing preserved",end.origin.y,next.origin.y,next.size.height);
  report(line);
 }
 return changed; /* caller owns; all unrelated native keys are preserved */
}
static void namedHook(id center,SEL cmd,id name,id object,id info){
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
 if(forwarding||center!=M0(objc_getClass("NSNotificationCenter"),"defaultCenter")){
  ((void(*)(id,SEL,id))oldNote)(center,cmd,note);return;
 }
 forwarding++;
 id name=M0(note,"name"),object=M0(note,"object"),mapped=mappedInfo(name,object,M0(note,"userInfo"));
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
static void install(void *unused){
 (void)unused;
 id bundle=M0(M0(objc_getClass("NSBundle"),"mainBundle"),"bundleIdentifier");
 if(equal(bundle,text("com.apple.springboard"))||equal(bundle,text("com.apple.Preferences")))return;
 id defaultCenter=M0(objc_getClass("NSNotificationCenter"),"defaultCenter");
 Class center=defaultCenter?object_getClass(defaultCenter):0;if(!center)return;
 Method a=class_getInstanceMethod(center,S("postNotificationName:object:userInfo:"));
 Method b=class_getInstanceMethod(center,S("postNotification:"));
 if(!a||!b||!encoding(method_getTypeEncoding(a),"v@:@@@")||!encoding(method_getTypeEncoding(b),"v@:@"))return;
 hook(center,"postNotificationName:object:userInfo:","v@:@@@",(IMP)namedHook,&oldNamed);
 hook(center,"postNotification:","v@:@",(IMP)noteHook,&oldNote);
 report("portrait real-event adapter installed; exact Scene ticket required; landscape unchanged");
}
__attribute__((constructor))static void start(void){dispatch_async_f(&_dispatch_main_q,0,install);}
