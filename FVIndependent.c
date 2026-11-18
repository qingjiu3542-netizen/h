// Experimental floatingView aspect extension. No original executable bytes changed.
typedef void *id; typedef void *SEL; typedef void *Class; typedef void *Method; typedef id (*IMP)(id,SEL,...);
typedef unsigned long U; typedef signed char BOOL;
typedef struct {double x,y;} Point; typedef struct {double width,height;} Size; typedef struct {Point origin;Size size;} Rect; typedef struct {double a,b,c,d,tx,ty;} Transform;
extern id objc_msgSend(id,SEL,...); extern Class objc_getClass(const char*); extern SEL sel_registerName(const char*); extern Method class_getInstanceMethod(Class,SEL); extern IMP method_getImplementation(Method); extern const char *method_getTypeEncoding(Method); extern BOOL class_addMethod(Class,SEL,IMP,const char*); extern IMP class_replaceMethod(Class,SEL,IMP,const char*); extern Class objc_allocateClassPair(Class,const char*,U); extern void objc_registerClassPair(Class); extern void *class_getInstanceVariable(Class,const char*); extern id object_getIvar(id,void*); extern Class object_getClass(id); extern BOOL class_isMetaClass(Class);
extern void dispatch_async_f(void*,void*,void(*)(void*)); extern void *_dispatch_main_q; extern void dispatch_after_f(unsigned long long,void*,void*,void(*)(void*)); extern unsigned long long dispatch_time(unsigned long long,long long); extern double sqrt(double); extern double fabs(double);
#define S(x) sel_registerName(x)
// Apple arm64 objc messages use fixed-argument ABI, not C variadic ABI.
#define M0(o,s) ((id(*)(id,SEL))objc_msgSend)((id)o,S(s))
#define M1(o,s,a) ((id(*)(id,SEL,U))objc_msgSend)((id)o,S(s),(U)a)
#define M2(o,s,a,b) ((id(*)(id,SEL,U,U))objc_msgSend)((id)o,S(s),(U)a,(U)b)
#define M3(o,s,a,b,c) ((id(*)(id,SEL,U,U,U))objc_msgSend)((id)o,S(s),(U)a,(U)b,(U)c)
#define M4(o,s,a,b,c,d) ((id(*)(id,SEL,U,U,U,U))objc_msgSend)((id)o,S(s),(U)a,(U)b,(U)c,(U)d)
#define M5(o,s,a,b,c,d,e) ((id(*)(id,SEL,U,U,U,U,U))objc_msgSend)((id)o,S(s),(U)a,(U)b,(U)c,(U)d,(U)e)
#define M7(o,s,a,b,c,d,e,f,g) ((id(*)(id,SEL,U,U,U,U,U,U,U))objc_msgSend)((id)o,S(s),(U)a,(U)b,(U)c,(U)d,(U)e,(U)f,(U)g)
#define M8(o,s,a,b,c,d,e,f,g,h) ((id(*)(id,SEL,U,U,U,U,U,U,U,U))objc_msgSend)((id)o,S(s),(U)a,(U)b,(U)c,(U)d,(U)e,(U)f,(U)g,(U)h)
#define PICK(_0,_1,_2,_3,_4,_5,_6,_7,_8,N,...) N
#define SEND(o,s,...) PICK(_,##__VA_ARGS__,M8,M7,M6,M5,M4,M3,M2,M1,M0)(o,s,##__VA_ARGS__)
static id str(const char*x){return SEND(objc_getClass("NSString"),"stringWithUTF8String:",x);}
extern int open(const char*,int,...);extern long write(int,const void*,U);extern int close(int);extern U strlen(const char*);extern void NSLog(id,...);
static void rawTrace(const char *event){
#ifdef FV_FIRST_PRESENTATION_PROBE
 const char *paths[]={"/tmp/FVFirstPresentation.log","/var/mobile/Library/Preferences/FVFirstPresentation.log"};
#else
 const char *paths[]={"/tmp/FVAspect8.log","/var/mobile/Library/Preferences/FVAspect8.log"};
#endif
 // Darwin O_WRONLY | O_APPEND | O_CREAT; works before Foundation initializes.
 for(int i=0;i<2;i++){int fd=open(paths[i],0x209,0644);if(fd>=0){write(fd,event,strlen(event));write(fd,"\n",1);close(fd);}}
}
static void trace(const char *event){rawTrace(event);
#ifdef FV_FIRST_PRESENTATION_PROBE
 NSLog(str("[FPDIAG1] %s"),event);
#else
 NSLog(str("[FVAspect8] %s"),event);
#endif
}
static id prefs(void){return SEND(SEND(objc_getClass("NSUserDefaults"),"alloc"),"initWithSuiteName:",str("com.minis.floatingview.aspect"));}
static int enabled(void){id p=prefs();int n=((long(*)(id,SEL,id))objc_msgSend)(p,S("integerForKey:"),str("ratio"));SEND(p,"release");return n==1;}
static Rect rect(id o,const char*s){return ((Rect(*)(id,SEL))objc_msgSend)(o,S(s));}
static Transform trans(id o){return ((Transform(*)(id,SEL))objc_msgSend)(o,S("transform"));}
static void setrect(id o,const char*s,Rect r){((void(*)(id,SEL,Rect))objc_msgSend)(o,S(s),r);}
static id ivar(id o,const char*n){void*v=class_getInstanceVariable(object_getClass(o),n);return v?object_getIvar(o,v):0;}
#include "FVGeometry.inc"
#ifdef FV_FIRST_PRESENTATION_PROBE
#include "FVFirstPresentation.inc"
#endif

/* Keyboard migration is intentionally disabled in this independent branch.
 * The original floatingView owns the keyboard host and its Swift layout helper;
 * moving the system layer without that helper causes clipping and ownership
 * races. Keyboard adaptation will use a separate UIWindowScene geometry path.
 */
#if 0
extern long ivar_getOffset(void *);
extern U class_getInstanceSize(Class);
static Class fvKWindowClass; static IMP fvKOldFrame; static char fvKPendingKey;
static int fvKInstalled, fvKAttempts;
static id fvKEligible(id target){
 id host=target;for(int i=0;i<3&&host;i++)host=SEND(host,"superview");
 if(!fvGKind(host,fvGHostClass)){trace("K migrate skip: host chain mismatch");return 0;}id item=SEND(host,"superview");
 if(!fvGKind(item,fvGItemClass)){trace("K migrate skip: item mismatch");return 0;}void*v=class_getInstanceVariable(object_getClass(item),"isSideCard");
 if(!v)return 0;long off=ivar_getOffset(v);if(off<0||(U)off>=class_getInstanceSize(object_getClass(item)))return 0;
 return *(unsigned char*)((char*)item+off)?0:item;
}
static id fvKContainer(id target){
 id sw=SEND(target,"window");if(!sw)return 0;id screen=SEND(sw,"screen");id app=SEND(objc_getClass("UIApplication"),"sharedApplication");id ws=SEND(app,"windows");id result=0;U n=(U)SEND(ws,"count");
 for(U i=0;i<n;i++){id w=SEND(ws,"objectAtIndex:",i);if(!fvGKind(w,fvKWindowClass)||SEND(w,"screen")!=screen||SEND(w,"isHidden"))continue;id c=ivar(w,"keyboardView");if(!c||SEND(c,"window")!=w||SEND(c,"isHidden"))continue;if(result&&result!=c)return 0;result=c;}return result;
}
static void fvKMigrate(void*ctx){id t=ctx;trace("K migrate async");if(enabled()&&fvKEligible(t)){trace("K migrate eligible");id c=fvKContainer(t);if(!c){trace("K migrate skip: no keyboard container");}else if(SEND(t,"superview")==c){trace("K migrate skip: already attached");}else if((U)SEND(SEND(c,"subviews"),"count")!=0){trace("K migrate skip: container occupied");}else{((void(*)(id,SEL,id))objc_msgSend)(c,S("addSubview:"),t);trace("K migrate success");}}else trace("K migrate skip: ratio disabled or chain mismatch");objc_setAssociatedObject(t,&fvKPendingKey,0,1);SEND(t,"release");}
static void fvKFrameHook(id self,SEL cmd,Rect frame){((void(*)(id,SEL,Rect))fvKOldFrame)(self,cmd,frame);if(!fvGMain()||!enabled()||objc_getAssociatedObject(self,&fvKPendingKey)||!fvKEligible(self))return;objc_setAssociatedObject(self,&fvKPendingKey,(id)fvKWindowClass,1);SEND(self,"retain");dispatch_async_f(&_dispatch_main_q,self,fvKMigrate);}
static void fvKInstall(void*unused){(void)unused;if(fvKInstalled)return;trace("K migrate install attempt");Class c=objc_getClass("_UIKeyboardLayerHostView");fvKWindowClass=objc_getClass("_TtC12floatingView16FVKeyboardWindow");if(!fvKWindowClass)fvKWindowClass=objc_getClass("floatingView.FVKeyboardWindow");Method m=c?class_getInstanceMethod(c,S("setFrame:")):0;if(!fvGInstalled||!fvKWindowClass||!m){trace("K migrate install wait: class or container missing");if(fvKAttempts++<50)dispatch_after_f(dispatch_time(0,10000000),&_dispatch_main_q,0,fvKInstall);return;}fvGHook(c,"setFrame:",(IMP)fvKFrameHook,&fvKOldFrame);fvKInstalled=1;trace("K migrate hook installed");}
#endif
#if 0 // Superseded: old small-window-sized scene algorithm.
static IMP oldLayout;static int busy;
static void layout(id self,SEL cmd){
 if(busy)return;
 if(oldLayout)((void(*)(id,SEL))oldLayout)(self,cmd);
 if(!enabled())return;
 id parent=SEND(self,"superview");id scene=ivar(self,"appScene");id remote=ivar(self,"hostingView");
 if(!parent||!scene||!remote)return;
 id settings=SEND(scene,"settings");if(!settings)return;
 busy=1;
 // Parent coordinates remain portrait; existing outer rotation/scale stays intact.
 Rect b=rect(parent,"bounds");double w=b.size.width;
 if(w<80||w>3000){busy=0;return;}
 double h=w*16.0/9.0;
 if(fabs(b.size.height-h)>0.5){b.size.height=h;setrect(parent,"setBounds:",b);}
 Rect hb={{0,0},{w,h}};setrect(self,"setFrame:",hb);
 long orientation=((long(*)(id,SEL))objc_msgSend)(settings,S("interfaceOrientation"));
 int landscape=orientation==3||orientation==4;
 Rect sf={{0,0},{landscape?h:w,landscape?w:h}};
 Rect prev=rect(settings,"frame");
 if(fabs(prev.size.width-sf.size.width)>0.5||fabs(prev.size.height-sf.size.height)>0.5){
  id copy=SEND(settings,"mutableCopy");
  if(copy&&((BOOL(*)(id,SEL,SEL))objc_msgSend)(copy,S("respondsToSelector:"),S("setFrame:"))){
   setrect(copy,"setFrame:",sf);
   SEND(scene,"updateSettings:withTransitionContext:completion:",copy,(id)0,(id)0);
  }
  SEND(copy,"release");
 }
 // Preserve original rotation, replace scale using the new scene dimensions.
 Transform t=trans(remote);double norm=sqrt(t.a*t.a+t.b*t.b);
 if(norm>0.000001){
  t.a/=norm;t.b/=norm;t.c/=norm;t.d/=norm;t.tx=0;t.ty=0;
  double rw=fabs(t.a)*sf.size.width+fabs(t.c)*sf.size.height;
  double rh=fabs(t.b)*sf.size.width+fabs(t.d)*sf.size.height;
  double scale=w/rw;
  // Only apply a uniform fit; mismatched orientation is left for the next layout.
  if(rh>0&&fabs(rh*scale-h)<1){
   t.a*=scale;t.b*=scale;t.c*=scale;t.d*=scale;
   setrect(remote,"setBounds:",sf);
   ((void(*)(id,SEL,Transform))objc_msgSend)(remote,S("setTransform:"),t);
   ((void(*)(id,SEL,Point))objc_msgSend)(remote,S("setCenter:"),(Point){w/2,h/2});
  }
 }
 busy=0;
}
static int installed;static int attempts;
static void install(void*unused){
 Class c=objc_getClass("floatingView.FVAppDispatchHostingView");if(!c)c=objc_getClass("_TtC12floatingView24FVAppDispatchHostingView");
 if(c&&!installed){Method m=class_getInstanceMethod(c,S("layoutSubviews"));if(m){oldLayout=method_getImplementation(m);class_replaceMethod(c,S("layoutSubviews"),(IMP)layout,method_getTypeEncoding(m));installed=1;}}
 if(!installed&&attempts++<30)dispatch_after_f(dispatch_time(0,1000000000),&_dispatch_main_q,0,install);
}
#endif
#ifndef FV_FIRST_PRESENTATION_PROBE
// SETTINGS: installed only after dyld/ObjC finish registering the host bundle.
extern const char *class_getName(Class);
extern void *objc_getAssociatedObject(id,const void*);
extern void objc_setAssociatedObject(id,const void*,id,U);
extern int getpid(void);
extern int snprintf(char*,U,const char*,...);
typedef struct {const char *fname;void *base;const char *sname;void *addr;} FVImageInfo;
extern int dladdr(const void*,FVImageInfo*);
static IMP oldSpecs,oldViewWillAppear;
static int prefsInstalled,prefsAttempts;
static Class ratioController;
static char rootGuard,childCache;
static id moduleInfo(void){
 FVImageInfo info={0};dladdr((const void*)moduleInfo,&info);
 id executable=SEND(SEND(objc_getClass("NSBundle"),"mainBundle"),"executablePath");
 return ((id(*)(id,SEL,id,...))objc_msgSend)(objc_getClass("NSString"),S("stringWithFormat:"),str("FVAspect8\n模块：%@\n宿主：%@\n主页面挂钩：%@\n日志：/tmp/FVAspect8.log 或 /var/mobile/Library/Preferences/FVAspect8.log"),str(info.fname?info.fname:"unknown"),executable,str(prefsInstalled?"已安装":"等待 RootListController"));
}
static id specifiers(id self,SEL cmd){
 id cached=objc_getAssociatedObject(self,&childCache);if(cached)return cached;
 id a=SEND(objc_getClass("NSMutableArray"),"array");Class ps=objc_getClass("PSSpecifier");
 id group=SEND(ps,"groupSpecifierWithName:",str("窗口比例"));
 SEND(group,"setProperty:forKey:",str("切换后关闭并重新打开小窗。16:9 竖向对应 9:16；应用重新排版兼容性需要实测。"),str("footerText"));SEND(a,"addObject:",group);
 id x=SEND(ps,"preferenceSpecifierNamed:target:set:get:detail:cell:edit:",str("窗口比例"),self,S("fvAspectWrite:specifier:"),S("fvAspectRead:"),objc_getClass("PSListItemsController"),(long)2,(id)0);
 id ns[]={str("原生大小"),str("16:9")};id vs[]={SEND(objc_getClass("NSNumber"),"numberWithInt:",0),SEND(objc_getClass("NSNumber"),"numberWithInt:",1)};
 id names=SEND(objc_getClass("NSArray"),"arrayWithObjects:count:",ns,2);
 id values=SEND(objc_getClass("NSArray"),"arrayWithObjects:count:",vs,2);
 SEND(x,"setValues:titles:",values,names);
 SEND(x,"setProperty:forKey:",str("FVAspectRatio"),str("id"));
 SEND(x,"setProperty:forKey:",str("ratio"),str("key"));
 SEND(x,"setProperty:forKey:",str("com.minis.floatingview.aspect"),str("defaults"));
 SEND(x,"setProperty:forKey:",vs[0],str("default"));SEND(a,"addObject:",x);
 group=SEND(ps,"groupSpecifierWithName:",str("加载诊断"));
 SEND(group,"setProperty:forKey:",moduleInfo(),str("footerText"));SEND(a,"addObject:",group);
 objc_setAssociatedObject(self,&childCache,a,1); // RETAIN_NONATOMIC
 SEND(self,"setValue:forKey:",a,str("_specifiers"));trace("ratio child opened; specifiers cached");return a;
}
static id readPref(id self,SEL cmd,id sp){id p=prefs();id v=SEND(p,"objectForKey:",str("ratio"));if(!v)v=SEND(objc_getClass("NSNumber"),"numberWithInt:",0);SEND(v,"retain");SEND(p,"release");return SEND(v,"autorelease");}
static void writePref(id self,SEL cmd,id value,id sp){
 id p=prefs();SEND(p,"setObject:forKey:",value,str("ratio"));SEND(p,"synchronize");SEND(p,"release");trace("ratio preference saved");
}
static BOOL same(id a,const char *b){return ((BOOL(*)(id,SEL,id))objc_msgSend)(a,S("isEqualToString:"),str(b));}
static id mergedSpecs(id self,id original){
 if(!original){trace("RootListController returned nil; leaving cache untouched");return original;}
 U count=(U)SEND(original,"count"),at=count;
 for(U i=0;i<count;i++){
  id x=SEND(original,"objectAtIndex:",i);
  if(same(SEND(x,"propertyForKey:",str("id")),"FVAspectMainEntry"))return original;
  id detail=SEND(x,"propertyForKey:",str("detail"));
  if(same(SEND(x,"name"),"浮窗设置")||(detail&&detail==objc_getClass("_TtC17floatingViewPrefs14FloatingViewVC")))at=i+1;
 }
 id a=SEND(original,"mutableCopy");
 id x=SEND(objc_getClass("PSSpecifier"),"preferenceSpecifierNamed:target:set:get:detail:cell:edit:",str("窗口比例"),self,(SEL)0,(SEL)0,ratioController,(long)1,(id)0);
  SEND(x,"setProperty:forKey:",str("FVAspectMainEntry"),str("id"));
  // PSSpecifier factories may return nil when Preferences.framework is not loaded.
  if(!x){trace("PSSpecifier creation failed; original settings returned");SEND(a,"release");return original;}
  SEND(a,"insertObject:atIndex:",x,at);
 // Root's Swift implementation reads the inherited PSListController cache via
 // KVC. Update that same cache, not only the ObjC getter's return value.
 SEND(self,"setValue:forKey:",a,str("_specifiers"));
 trace("MAIN RootListController entry inserted; _specifiers updated");return SEND(a,"autorelease");
}
static id integratedSpecs(id self,SEL cmd){
 if(objc_getAssociatedObject(self,&rootGuard))return ((id(*)(id,SEL))oldSpecs)(self,cmd);
 objc_setAssociatedObject(self,&rootGuard,SEND(objc_getClass("NSNumber"),"numberWithInt:",1),1);
 id original=((id(*)(id,SEL))oldSpecs)(self,cmd);
 id result=mergedSpecs(self,original);objc_setAssociatedObject(self,&rootGuard,0,1);return result;
}
#ifndef FV_DIRECT_ENTRY
static void prefsViewWillAppear(id self,SEL cmd,BOOL animated){
 if(oldViewWillAppear)((void(*)(id,SEL,BOOL))oldViewWillAppear)(self,cmd,animated);
 // Also handles Swift direct dispatch that bypasses the ObjC specifiers thunk.
 id original=((id(*)(id,SEL))oldSpecs)(self,S("specifiers"));
 if(mergedSpecs(self,original)!=original)SEND(self,"reloadSpecifiers");
}
#endif
static void refreshExistingRoot(id controller,Class root,U depth){
 if(!controller||depth>20)return;
 if(((BOOL(*)(id,SEL,Class))objc_msgSend)(controller,S("isKindOfClass:"),root)){
  SEND(controller,"reloadSpecifiers");trace("existing MAIN controller refreshed");
 }
 id children=SEND(controller,"childViewControllers");U count=(U)SEND(children,"count");
 for(U i=0;i<count;i++)refreshExistingRoot(SEND(children,"objectAtIndex:",i),root,depth+1);
 refreshExistingRoot(SEND(controller,"presentedViewController"),root,depth+1);
}
static void refreshVisible(void *root){
 id app=SEND(objc_getClass("UIApplication"),"sharedApplication");id windows=SEND(app,"windows");
 U count=(U)SEND(windows,"count");for(U i=0;i<count;i++)refreshExistingRoot(SEND(SEND(windows,"objectAtIndex:",i),"rootViewController"),(Class)root,0);
}
static void installPrefs(void *unused){
 if(prefsInstalled)return;
 Class c=objc_getClass("_TtC17floatingViewPrefs18RootListController");
 if(!c)c=objc_getClass("floatingViewPrefs.RootListController");
 if(!c||!objc_getClass("PSListController")||!objc_getClass("PSSpecifier")){
  if(prefsAttempts++<30)dispatch_after_f(dispatch_time(0,1000000000),&_dispatch_main_q,(void*)1,installPrefs);
  else if(prefsAttempts==31)trace("RootListController absent after retries; image notifications remain active");
  return;
 }
 Method m=class_getInstanceMethod(c,S("specifiers"));if(!m){trace("RootListController has no specifiers");return;}
 if(class_getInstanceMethod(c,S("fvAspect8Owner"))){prefsInstalled=1;trace("RootListController already owned by another FVAspect8 image");return;}
 ratioController=objc_getClass("FVAspect8RatioController");
 if(!ratioController){
  ratioController=objc_allocateClassPair(objc_getClass("PSListController"),"FVAspect8RatioController",0);
  if(!ratioController){trace("could not allocate ratio controller");return;}
  class_addMethod(ratioController,S("specifiers"),(IMP)specifiers,"@@:");
  class_addMethod(ratioController,S("fvAspectRead:"),(IMP)readPref,"@@:@");
  class_addMethod(ratioController,S("fvAspectWrite:specifier:"),(IMP)writePref,"v@:@@");
  objc_registerClassPair(ratioController);
 }
 class_addMethod(c,S("fvAspect8Owner"),(IMP)readPref,"@@:@");
#ifndef FV_DIRECT_ENTRY
 oldSpecs=method_getImplementation(m);class_replaceMethod(c,S("specifiers"),(IMP)integratedSpecs,method_getTypeEncoding(m));
 m=class_getInstanceMethod(c,S("viewWillAppear:"));
 if(m){oldViewWillAppear=method_getImplementation(m);class_replaceMethod(c,S("viewWillAppear:"),(IMP)prefsViewWillAppear,method_getTypeEncoding(m));}
#endif
 prefsInstalled=1;trace("MAIN RootListController controller methods installed");
#ifndef FV_DIRECT_ENTRY
 dispatch_async_f(&_dispatch_main_q,c,refreshVisible);
#endif
 FVImageInfo info={0};dladdr((const void*)installPrefs,&info);if(info.fname)trace(info.fname);
}
// Called synchronously by the original Root specifiers thunk, with a +1 array.
__attribute__((visibility("default"))) id FVAspectMerge(id self,id original){
 installPrefs(0);
 if(!ratioController)return original;
 id result=mergedSpecs(self,original);
 if(result!=original){SEND(result,"retain");SEND(original,"release");}
 return result;
}
#endif
#ifdef FV_FIRST_PRESENTATION_PROBE
static BOOL same(id a,const char *b){return ((BOOL(*)(id,SEL,id))objc_msgSend)(a,S("isEqualToString:"),str(b));}
#endif
extern void _dyld_register_func_for_add_image(void(*)(const void*,long));
static void imageAdded(const void *header,long slide){
#ifndef FV_FIRST_PRESENTATION_PROBE
 dispatch_async_f(&_dispatch_main_q,(void*)1,installPrefs);
#endif
}
static void startOnMain(void *unused){
 trace("main queue reached; Foundation available");
#ifndef FV_FIRST_PRESENTATION_PROBE
 installPrefs(0);
#endif
 id name=SEND(SEND(objc_getClass("NSProcessInfo"),"processInfo"),"processName");
 if(same(name,"SpringBoard")){
#ifdef FV_FIRST_PRESENTATION_PROBE
  fvFPInstall(0);
#else
  dispatch_async_f(&_dispatch_main_q,0,install);
#endif
  trace("K migration disabled in stable baseline");
 }
}
__attribute__((constructor)) static void start(void){
 // Do not invoke Foundation/Swift from a dependency initializer: the owner
 // bundle and its dependencies may still be initializing under dyld's lock.
#ifdef FV_FIRST_PRESENTATION_PROBE
 rawTrace("FPDIAG1 initializer: first-presentation diagnostic; geometry/settings hooks disabled; keyboard migration disabled");
#else
 rawTrace("FVAspect8 constructor reached (before Foundation)");
 _dyld_register_func_for_add_image(imageAdded);
#endif
 dispatch_async_f(&_dispatch_main_q,0,startOnMain);
}
