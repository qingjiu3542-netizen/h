"""Run the production core AND verbatim production native submit function.
No iOS behavior claim: mocks replace only Objective-C/platform boundaries.
"""
from pathlib import Path
import os, subprocess, tempfile
R=Path(__file__).resolve().parent
s=Path(os.environ.get('FV_SAFEAREA_TEST_SOURCE',R/'FVSceneSafeArea.inc')).read_text()
def extract(start,end):
 a=s.index(start);return s[a:s.index(end,a)]
submit=extract('static int fvSASubmit(', '\nstatic FVSAOps fvSAOperations(')
abi=extract('static int fvSAABI(', '\nstatic id fvSAObject(')
prefix=r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "FVSceneSafeAreaCore.h"
#include "FVKeyboardHostRouteCore.h"
typedef unsigned long U;typedef const char*SEL;
typedef struct Obj {int refs,kind,present,abi;FVSAInsets value;int unrelated;struct Obj*settings,*other;} Obj;
typedef Obj*id;typedef void*Method;
static Obj SC,LIVE,NEXT,COPY,MAP,CMAP,NMAP,BOX,VALUECLASS,MUTABLECLASS;
static id fvSAScene=&SC;static FVSAState fvSAS;
static int on,valid,mainThread,submits,setters,flip,reentry,reentryResult,dead;
static int fvSAFieldProved=1;
static FVSAOps ops;
#define S(x) (x)
static const char *encoding;static unsigned arguments;
static void *object_getClass(id o){return o;}
static Method class_getInstanceMethod(void*c,SEL sel){(void)sel;return c;}
static unsigned method_getNumberOfArguments(Method m){(void)m;return arguments;}
static const char *method_getTypeEncoding(Method m){(void)m;return encoding;}
'''
mock=r'''
static id objc_getClass(const char*n){return !strcmp(n,"NSValue")?&VALUECLASS:&MUTABLECLASS;}
static int fvGKind(id o,id cls){return o&&cls==&MUTABLECLASS&&o->kind==2;}
static id fvSAObject(id o,const char*s){
 if(!o)return 0;if(o->refs<=0)dead++;
 if(!strcmp(s,"settings"))return o->settings;
 if(!strcmp(s,"otherSettings"))return o->other;
 assert(0);return 0;
}
static int fvSACurrent(void *p,int restoring){(void)p;return mainThread&&valid&&(restoring||on);}
static id fvSAField(id o){return o&&o->other&&o->other->present?o->other:0;}
static int fvSAUnbox(id v,FVSAInsets*out){if(!v||!v->present||!v->abi)return 0;*out=v->value;return fvSAValid(*out);}
static int fvSARead(void*p,FVSASnapshot*out){
 (void)p;id o=fvSAScene->settings;if(!mainThread||!o)return 0;
 *out=(FVSASnapshot){(U)fvSAScene,o->other->present,o->other->abi,o->other->value};return 1;
}
static id fvSABox(FVSAInsets value){BOX.value=value;return &BOX;}
static id send(id o,const char*s,...){
 if(!o)return 0;if(o->refs<=0)dead++;
 va_list ap;va_start(ap,s);id result=0;
 if(!strcmp(s,"retain")){o->refs++;result=o;}
 else if(!strcmp(s,"release"))o->refs--;
 else if(!strcmp(s,"mutableCopy")){
  COPY=*o;COPY.refs=1;COPY.kind=2;CMAP=*o->other;CMAP.refs=1;COPY.other=&CMAP;result=&COPY;
  if(flip==1)SC.settings=&NEXT;
  if(flip==6){NMAP.value=MAP.value;SC.settings=&NEXT;}
  if(flip==4)on=0;
 }
 else if(!strcmp(s,"setObject:forSetting:")){
  id v=va_arg(ap,id);assert(va_arg(ap,U)==15);o->value=v->value;o->present=1;setters++;
  if(flip==2)SC.settings=&NEXT;
  if(flip==5)on=0;
 }
 else if(!strcmp(s,"updateSettings:withTransitionContext:completion:")){
  id c=va_arg(ap,id);MAP=*c->other;MAP.refs=1;LIVE.unrelated=c->unrelated;LIVE.other=&MAP;SC.settings=&LIVE;submits++;
  if(reentry){on=0;reentryResult=fvSATransact(&fvSAS,&ops,1,(FVSAInsets){0});}
  if(flip==3)SC.settings=&NEXT;
 }
 else assert(!"unexpected selector");va_end(ap);return result;
}
#define SEND(o,s,...) send((o),(s),##__VA_ARGS__)
'''
main=r'''
static FVSAInsets baseline={0,0,0,0},target={0,0,12,0};
static void reset(void){
 Obj*all[]={&SC,&LIVE,&NEXT,&COPY,&MAP,&CMAP,&NMAP,&BOX,&VALUECLASS,&MUTABLECLASS};
 for(unsigned i=0;i<sizeof all/sizeof all[0];i++){memset(all[i],0,sizeof(Obj));all[i]->refs=1;all[i]->present=all[i]->abi=1;}
 SC.settings=&LIVE;LIVE.other=&MAP;LIVE.unrelated=71;NEXT.other=&NMAP;NEXT.unrelated=99;NMAP.value=(FVSAInsets){0,0,44,0};
 COPY.refs=0;fvSAS=(FVSAState){.scene=(U)&SC};on=valid=mainThread=1;
 submits=setters=flip=reentry=reentryResult=dead=0;
 ops=(FVSAOps){0,fvSACurrent,fvSARead,fvSASubmit};
}
static void balanced(void){assert(!dead);assert(LIVE.refs==1&&NEXT.refs==1&&SC.refs==1&&COPY.refs==0);}
static int apply(void){return fvSATransact(&fvSAS,&ops,0,target);}
static int restore(void){return fvSATransact(&fvSAS,&ops,1,(FVSAInsets){0});}
int main(void){
 /* Exact production ABI helper: struct HFA, offsets, block and scalar widths. */
 arguments=3;encoding="v48@0:8{UIEdgeInsets=dddd}16";assert(fvSAABI(&SC,"setSafeAreaInsetsPortrait:","v@:{UIEdgeInsets=dddd}"));
 encoding="v48@0:8{UIEdgeInsets=ffff}16";assert(!fvSAABI(&SC,"setSafeAreaInsetsPortrait:","v@:{UIEdgeInsets=dddd}"));
 encoding="v48@0:8{CGRect={CGPoint=dd}{CGSize=dd}}16";assert(!fvSAABI(&SC,"setSafeAreaInsetsPortrait:","v@:{UIEdgeInsets=dddd}"));
 arguments=4;encoding="v48@0:8{UIEdgeInsets=dddd}16";assert(!fvSAABI(&SC,"setSafeAreaInsetsPortrait:","v@:{UIEdgeInsets=dddd}"));
 arguments=0;encoding=0;assert(!fvSAABI(&SC,"uiSettings","@@:"));
 puts("PASS production ABI helper rejects wrong struct, scalar, count and missing encodings");
 /* The native submit's independent selector checks accept the known fixture. */
 reset();
'''
# In the behavioral harness the platform ABI lookup is fixed to the valid
# expected signature. The production ABI function itself runs unmodified above.
# Rename it for independent testing; submit resolves fixtureABI instead.
abi=abi.replace('fvSAABI(', 'productionABI(',1)
main=main.replace('fvSAABI(', 'productionABI(')
fixture='static int fvSAABI(id o,const char*s,const char*e){(void)s;(void)e;return o&&encoding!=0;}\n'
main+=r'''
 encoding="fixture-valid";
 assert(apply()==FVSAApplied&&submits==1&&fvSAS.owned&&fvSAEqual(fvSAS.original,baseline));
 assert(LIVE.unrelated==71);LIVE.unrelated=808;
 assert(restore()==FVSARestored&&submits==2&&!fvSAS.owned&&fvSAEqual(MAP.value,baseline)&&LIVE.unrelated==808);balanced();
 /* Explicit zero restores as an EXPLICIT object, not absent. */
 assert(MAP.present);assert(apply()==FVSARejected);
 reset();MAP.present=0;assert(apply()==FVSARejected&&!submits&&!setters);balanced();
 reset();MAP.abi=0;assert(apply()==FVSARejected&&!submits);balanced();
 reset();assert(fvSATransact(&fvSAS,&ops,0,baseline)==FVSANoop&&!submits&&!fvSAS.owned);balanced();
 reset();assert(apply()==FVSAApplied);MAP.value.bottom=37;
 assert(restore()==FVSASuperseded&&submits==1&&MAP.value.bottom==37&&!fvSAS.owned&&fvSAS.spent);assert(apply()==FVSARejected);balanced();
 /* Native/settings replacement during copy, setter, or update; no replay. */
 for(int f=1;f<=2;f++){reset();flip=f;assert(apply()==FVSASuperseded);assert(!submits&&SC.settings==&NEXT&&NEXT.unrelated==99);balanced();}
 reset();flip=3;assert(apply()==FVSASuperseded&&submits==1&&SC.settings==&NEXT&&!fvSAS.owned);balanced();
 for(int f=4;f<=5;f++){reset();flip=f;assert(apply()==FVSASuperseded&&!submits);balanced();}
 reset();flip=6;assert(apply()==FVSASuperseded&&!submits&&SC.settings==&NEXT&&NEXT.unrelated==99);balanced();
 reset();fvSAFieldProved=0;assert(apply()==FVSASuperseded&&!submits);fvSAFieldProved=1;balanced();
 reset();MAP.value.bottom=1;assert(apply()==FVSASuperseded&&!submits);balanced();
 /* Hide/teardown/rotation/fullscreen/toggle use the SAME restore operation. */
 for(int reason=0;reason<5;reason++){reset();assert(apply()==FVSAApplied);on=0;assert(restore()==FVSARestored&&fvSAEqual(MAP.value,baseline));balanced();}
 reset();reentry=1;assert(apply()==FVSAApplied);assert(reentryResult==FVSAReentry&&fvSAS.cancel&&fvSAS.spent&&fvSAS.owned);reentry=0;
 assert(restore()==FVSARestored&&fvSAEqual(MAP.value,baseline));balanced();
 reset();assert(apply()==FVSAApplied);SC.settings=&NEXT;assert(restore()==FVSASuperseded&&NEXT.other->value.bottom==44&&submits==1);balanced();
 reset();mainThread=0;assert(apply()==FVSARejected&&!submits);balanced();
 reset();valid=0;assert(apply()==FVSARejected&&!submits);balanced();
 reset();encoding=0;assert(apply()==FVSASuperseded&&!submits);balanced();
 puts("PASS real fvSASubmit + production transaction: ownership, reentry, explicit-zero/absent, newest unrelated fields, hide, replacement, no-op, ABI reject, off-main, restoration");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'safearea.c';out=Path(td)/'safearea';p.write_text(prefix+abi+fixture+mock+submit+main)
 subprocess.run(['clang','-std=gnu11','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-I',str(R),str(p),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
print('PASS verbatim native publication extraction and shared core regressions')
