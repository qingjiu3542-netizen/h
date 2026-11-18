/* Read-only diagnostic. No scene submissions, UI setters or Swift addresses.
 * FV_FP_CORE_TEST exposes the same encoding/budget logic to native C tests.
 * Enable only with FV_FIRST_PRESENTATION_PROBE; geometry hooks stay disabled.
 */
static int fvFPEqual(const char *a,const char *b){
 if(!a||!b)return 0;
 while(*a&&*a==*b){a++;b++;}return *a==*b;
}
static int fvFPEncoding(const char *actual,const char *expected){
 char out[192];unsigned n=0;int depth=0;
 if(!actual)return 0;
 for(;*actual;actual++){
  char c=*actual;
  if(!depth&&c>='0'&&c<='9')continue;
  if(n+1>=sizeof(out))return 0;
  out[n++]=c;
  if(c=='{')depth++;
  if(c=='}'&&--depth<0)return 0;
 }
 out[n]=0;return depth==0&&fvFPEqual(out,expected);
}
static int fvFPTake(unsigned *count,unsigned limit){
 if(*count>=limit)return 0;
 (*count)++;
 return 1;
}
#ifndef FV_FP_CORE_TEST
extern const char *ivar_getTypeEncoding(void *);
extern unsigned long long mach_absolute_time(void);
extern int snprintf(char *,U,const char *,...);
extern const char *class_getName(Class);
/* A process captures at most 128 method invocations (256 snapshots).
 * No object retention, associations, recurrent sampling or scene mutations.
 * Missing/ABI-incompatible readers report unavailable rather than invoking.
 */
typedef struct {Class cls;const char *name;const char *encoding;IMP original;IMP replacement;int done;} FVFPSlot;
static unsigned fvFPEvents;
static int fvFPBusy,fvFPAttempts;
static Class fvFPHost;
static FVFPSlot fvFPSlots[6];
static int fvFPMethod(id o,const char *name,const char *encoding){
 Method m=o?class_getInstanceMethod(object_getClass(o),S(name)):0;
 return m&&fvFPEncoding(method_getTypeEncoding(m),encoding);
}
static id fvFPObject(id o,const char *name){
 return fvFPMethod(o,name,"@@:")?SEND(o,name):0;
}
static id fvFPIvar(id o,const char *name){
 void *v=o?class_getInstanceVariable(object_getClass(o),name):0;
 const char *t=v?ivar_getTypeEncoding(v):0;
 return t&&t[0]=='@'?object_getIvar(o,v):0;
}
static long fvFPInteger(id o,const char *name){
 if(fvFPMethod(o,name,"q@:"))return ((long(*)(id,SEL))objc_msgSend)(o,S(name));
 if(fvFPMethod(o,name,"B@:"))return ((unsigned char(*)(id,SEL))objc_msgSend)(o,S(name));
 if(fvFPMethod(o,name,"c@:"))return ((signed char(*)(id,SEL))objc_msgSend)(o,S(name));
 return -999;
}
static void fvFPFrame(char *out,U cap,id o,const char *name){
 if(!fvFPMethod(o,name,"{CGRect={CGPoint=dd}{CGSize=dd}}@:")){
  snprintf(out,cap,"unavailable");return;
 }
 Rect r=rect(o,name);
 snprintf(out,cap,"%.3f,%.3f,%.3f,%.3f",r.origin.x,r.origin.y,r.size.width,r.size.height);
}
static void fvFPSnapshot(id self,id child,const char *event,unsigned seq){
 char line[1800],hf[120],sf[120],af[120],rf[120],rb[120],cf[120];
 id scene=fvFPIvar(self,"appScene"),setting=fvFPIvar(self,"appSetting");
 id remote=fvFPIvar(self,"hostingView"),live=fvFPObject(scene,"settings");
 id parent=fvFPObject(self,"superview"),window=fvFPObject(self,"window");
 fvFPFrame(hf,sizeof(hf),self,"frame");fvFPFrame(sf,sizeof(sf),live,"frame");
 fvFPFrame(af,sizeof(af),setting,"frame");fvFPFrame(rf,sizeof(rf),remote,"frame");
 fvFPFrame(rb,sizeof(rb),remote,"bounds");fvFPFrame(cf,sizeof(cf),child,"frame");
 snprintf(line,sizeof(line),
  "FP seq=%u tick=%llu event=%s self=%p class=%s host=%d ratio=%d parent=%p window=%p windowScene=%p scene=%p settings=%p appSetting=%p remote=%p remoteParent=%p remoteWindow=%p child=%p childParent=%p childWindow=%p childIsRemote=%d fg=%ld cachedFg=%ld orientation=%ld hostFrame=[%s] sceneFrame=[%s] cachedFrame=[%s] remoteFrame=[%s] remoteBounds=[%s] childFrame=[%s]",
  seq,mach_absolute_time(),event,self,class_getName(object_getClass(self)),object_getClass(self)==fvFPHost,enabled(),parent,window,fvFPObject(window,"windowScene"),scene,live,setting,remote,fvFPObject(remote,"superview"),fvFPObject(remote,"window"),child,fvFPObject(child,"superview"),fvFPObject(child,"window"),child&&child==remote,fvFPInteger(live,"isForeground"),fvFPInteger(setting,"isForeground"),fvFPInteger(live,"interfaceOrientation"),hf,sf,af,rf,rb,cf);
 rawTrace(line);
}
static unsigned fvFPBefore(id self,id child,const char *event){
 if(!fvGMain()||fvFPBusy||!fvFPTake(&fvFPEvents,128))return 0;
 fvFPBusy=1;unsigned seq=fvFPEvents;fvFPSnapshot(self,child,event,seq);fvFPBusy=0;return seq;
}
static void fvFPAfter(id self,id child,const char *event,unsigned seq){
 if(!seq)return;fvFPBusy=1;fvFPSnapshot(self,child,event,seq);fvFPBusy=0;
 if(seq==128)rawTrace("FP capture exhausted: 128 invocations / 256 snapshots; restart for another capture");
}
#define FV_FP_ZERO(fn,index,label) \
 static void fn(id self,SEL cmd){ \
 unsigned seq=fvFPBefore(self,0,label ".before"); \
 ((void(*)(id,SEL))fvFPSlots[index].original)(self,cmd); \
 fvFPAfter(self,0,label ".after",seq); }
#define FV_FP_ONE(fn,index,label) \
 static void fn(id self,SEL cmd,id child){ \
 unsigned seq=fvFPBefore(self,child,label ".before"); \
 ((void(*)(id,SEL,id))fvFPSlots[index].original)(self,cmd,child); \
 fvFPAfter(self,child,label ".after",seq); }
FV_FP_ONE(fvFPHostAdd,0,"host.addSubview")
FV_FP_ZERO(fvFPHostLayout,1,"host.layoutSubviews")
FV_FP_ZERO(fvFPHostWindow,2,"host.didMoveToWindow")
FV_FP_ONE(fvFPDispatchAdd,3,"dispatch.addSubview")
FV_FP_ZERO(fvFPDispatchLayout,4,"dispatch.layoutSubviews")
FV_FP_ZERO(fvFPDispatchWindow,5,"dispatch.didMoveToWindow")
#undef FV_FP_ZERO
#undef FV_FP_ONE
static void fvFPInstall(void *unused){
 (void)unused;if(!fvGMain())return;
 Class h=objc_getClass("_TtC12floatingView24FVAppDispatchHostingView");
 Class d=objc_getClass("_TtC12floatingView17FVAppDispatchView");
 if(!h||!d){
  if(fvFPAttempts++<50)dispatch_after_f(dispatch_time(0,10000000),&_dispatch_main_q,0,fvFPInstall);
  else rawTrace("FP install stopped: required classes unavailable after 51 attempts");
  return;
 }
 fvFPHost=h;
 const char *names[]={"addSubview:","layoutSubviews","didMoveToWindow"};
 IMP replacements[]={(IMP)fvFPHostAdd,(IMP)fvFPHostLayout,(IMP)fvFPHostWindow,(IMP)fvFPDispatchAdd,(IMP)fvFPDispatchLayout,(IMP)fvFPDispatchWindow};
 for(unsigned i=0;i<6;i++){
  FVFPSlot *s=&fvFPSlots[i];if(s->done)continue;s->done=1;
  s->cls=i<3?h:d;s->name=names[i%3];s->encoding=i%3?"v@:":"v@:@";s->replacement=replacements[i];
  Method m=class_getInstanceMethod(s->cls,S(s->name));const char *encoding=m?method_getTypeEncoding(m):0;
  int valid=m&&fvFPEncoding(encoding,s->encoding);
  char line[512];snprintf(line,sizeof(line),"FP ABI class=%s selector=%s encoding=%s accepted=%d",class_getName(s->cls),s->name,encoding?encoding:"missing",valid);rawTrace(line);
  if(!valid)continue;
  s->original=method_getImplementation(m);
  /* Inherited UIView methods become overrides ONLY on the named classes. */
  if(!class_addMethod(s->cls,S(s->name),s->replacement,encoding))
   class_replaceMethod(s->cls,S(s->name),s->replacement,encoding);
 }
 rawTrace("FP probe ready: read-only; raw ticks; missing fields unavailable; no client-frame completion inference");
}
#endif
