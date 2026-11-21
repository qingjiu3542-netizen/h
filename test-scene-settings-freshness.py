# Executes actual frame commit/restore code; unrelated Scene state must survive.
# A mock regression, NOT evidence of the device's keyboard failure cause.
from pathlib import Path
import os, subprocess, tempfile, resource
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
R=Path(__file__).resolve().parent
s=Path(os.environ.get('FV_SCENE_TEST_SOURCE', R/'FVGeometry.inc')).read_text()
r=Path(os.environ.get('FV_RESTORE_TEST_SOURCE', R/'FVRestore.inc')).read_text()
def extract(text, begin, end):
 a=text.index(begin); return text[a:text.index(end,a)]
context=extract(s,'static BOOL fvGSceneContextCurrent(', 'static BOOL fvGSceneFrame(')
commit=extract(s,'static BOOL fvGSceneFrame(', '\ntypedef struct {Size sceneSize;')
restore=extract(r,'static int fvGRSubmit(', 'static int fvGRAccepted(')
prefix=r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
typedef int BOOL;typedef unsigned long U;
typedef struct{double x,y;}Point;typedef struct{double width,height;}Size;typedef struct{Point origin;Size size;}Rect;
typedef struct Obj{int refs,generation;Rect frame;struct Obj *scene,*cached,*state,*parent,*remote,*window,*settings;}Obj;typedef Obj*id;
static Obj H,SC,STALE,LIVE,NEXT,CACHE,STATE,PARENT,REMOTE,WINDOW,COPY;
static char fvGStateKey,fvGLeavingKey;
static int flip,submits,cacheWrites,deadMessages,transferredGeneration;
static void replaceSettings(id next){
 id old=SC.settings;if(old==next)return;
 if(next)next->refs++;SC.settings=next;if(old)old->refs--;
}
static id send(id o,const char*sel,...){
 if(!o)return 0;if(o->refs<=0)deadMessages++;
 va_list ap;va_start(ap,sel);id result=0;
 if(!strcmp(sel,"retain")){o->refs++;result=o;}
 else if(!strcmp(sel,"release"))o->refs--;
 else if(!strcmp(sel,"superview"))result=o->parent;
 else if(!strcmp(sel,"window"))result=o->window;
 else if(!strcmp(sel,"settings"))result=o->settings;
 else if(!strcmp(sel,"mutableCopy")){
  COPY.refs=1;COPY.frame=o->frame;COPY.generation=o->generation;result=&COPY;
  if(flip==1){replaceSettings(&NEXT);if(o->refs<=0)deadMessages++;}
 }
 else if(!strcmp(sel,"updateSettings:withTransitionContext:completion:")){
  id copy=va_arg(ap,id);LIVE.frame=copy->frame;LIVE.generation=copy->generation;
  SC.settings=&LIVE;submits++;transferredGeneration=copy->generation;
 }
 else assert(!"unexpected selector");
 va_end(ap);return result;
}
#define SEND(o,s,...) send((o),(s),##__VA_ARGS__)
static id ivar(id o,const char*n){if(!o)return 0;if(!strcmp(n,"appScene"))return o->scene;if(!strcmp(n,"appSetting"))return o->cached;if(!strcmp(n,"hostingView"))return o->remote;assert(0);return 0;}
static id objc_getAssociatedObject(id o,const void*k){return k==&fvGStateKey?o->state:0;}
static BOOL fvGMain(void){return 1;}
static BOOL fvGHas(id o,const char*s){(void)s;return o!=0;}
static Rect rect(id o,const char*s){(void)s;if(o->refs<=0)deadMessages++;return o->frame;}
static void setrect(id o,const char*s,Rect v){(void)s;if(o->refs<=0)deadMessages++;o->frame=v;if(o==&CACHE)cacheWrites++;if(o==&COPY&&flip==2)replaceSettings(&NEXT);}
static BOOL fvGSameRect(Rect a,Rect b){return fabs(a.origin.x-b.origin.x)<.001&&fabs(a.origin.y-b.origin.y)<.001&&fabs(a.size.width-b.size.width)<.001&&fabs(a.size.height-b.size.height)<.001;}
static void trace(const char*s){(void)s;}
typedef struct{id h,state,scene,cached,settings;int mode;Rect original;}FVRestoreContext;
static int fvGRIdentity(void*v){(void)v;return H.scene==&SC&&H.cached==&CACHE&&H.state==&STATE;}
static int fvGROrientation(void*v){(void)v;return 1;}
static BOOL fvGSettings(id scene,id cached,id*out){(void)cached;*out=SEND(scene,"settings");return *out!=0;}
static Rect target={{0,167.556},{430,764.444}};
static void reset(void){
 Obj*all[]={&H,&SC,&STALE,&LIVE,&NEXT,&CACHE,&STATE,&PARENT,&REMOTE,&WINDOW,&COPY};
 for(unsigned i=0;i<sizeof(all)/sizeof(all[0]);i++){memset(all[i],0,sizeof(Obj));all[i]->refs=1;all[i]->frame=(Rect){{0,0},{430,932}};}
 H.scene=&SC;H.cached=&CACHE;H.state=&STATE;H.parent=&PARENT;H.remote=&REMOTE;H.window=&WINDOW;
 PARENT.remote=&H;REMOTE.parent=&H;REMOTE.window=&WINDOW;SC.settings=&LIVE;
 STALE.generation=11;LIVE.generation=22;NEXT.generation=33;
 COPY.refs=0;flip=submits=cacheWrites=deadMessages=transferredGeneration=0;
}
static void balanced(void){assert(!deadMessages);assert(COPY.refs==0);assert(H.refs==1&&SC.refs==1&&STALE.refs==1&&LIVE.refs==(SC.settings==&LIVE?1:0)&&NEXT.refs==(SC.settings==&NEXT?2:1)&&CACHE.refs==1&&STATE.refs==1&&PARENT.refs==1&&REMOTE.refs==1&&WINDOW.refs==1);}
'''
call='fvGSceneFrame(&H,&SC,&LIVE,&CACHE,target)'
stale_call='fvGSceneFrame(&H,&SC,&STALE,&CACHE,target)'
main='int main(void){\n'
main+='reset();assert(!'+stale_call+');assert(!submits&&!cacheWrites&&LIVE.generation==22);balanced();\n'
main+='reset();assert('+call+');assert(submits==1&&transferredGeneration==22&&LIVE.generation==22&&cacheWrites==1);balanced();\n'
main+='for(int f=1;f<=2;f++){reset();flip=f;assert(!'+call+');assert(!submits&&!cacheWrites&&SC.settings==&NEXT&&NEXT.generation==33);balanced();}\n'
main+='reset();LIVE.frame=target;CACHE.frame=target;assert('+call+');assert(!submits&&!cacheWrites);balanced();\n'
main+='reset();FVRestoreContext c={&H,&STATE,&SC,&CACHE,&STALE,0,target};assert(fvGRSubmit(&c));assert(submits==1&&transferredGeneration==22&&LIVE.generation==22);balanced();\n'
main+='for(int f=1;f<=2;f++){reset();flip=f;FVRestoreContext c={&H,&STATE,&SC,&CACHE,&STALE,0,target};assert(!fvGRSubmit(&c));assert(!submits&&SC.settings==&NEXT&&NEXT.generation==33);balanced();}\n'
main+='reset();LIVE.frame=target;FVRestoreContext noOp={&H,&STATE,&SC,&CACHE,&STALE,0,target};assert(fvGRSubmit(&noOp));assert(!submits);balanced();\n'
main+='puts("PASS actual Scene settings freshness: preserve unrelated live fields, reject changed snapshot, restore symmetry, no-op, ownership");return 0;}\n'
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'test.c';out=Path(td)/'test';p.write_text(prefix+context+commit+restore+main)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-Wno-misleading-indentation',str(p),'-lm','-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
