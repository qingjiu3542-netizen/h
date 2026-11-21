# Regression executes the actual Scene setter with a deterministic reentrant mock.
# Host cache replacement must retire the transaction without using a dead object.
# Not a device keyboard-avoidance test.
from pathlib import Path
import os,subprocess,tempfile
R=Path(__file__).resolve().parent
s=Path(os.environ.get('FV_SCENE_TEST_SOURCE',str(R/'FVGeometry.inc'))).read_text();start=s.index('static BOOL fvGSceneFrame(');end=s.index('\ntypedef struct {Size sceneSize;',start)
func=s[start:end]
prefix=r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
typedef int BOOL;typedef unsigned long U;
typedef struct{double x,y;} Point;typedef struct{double width,height;} Size;typedef struct{Point origin;Size size;} Rect;
typedef struct Obj{int refs,kind;Rect frame;struct Obj *scene,*cached,*state,*parent,*remote,*window;}Obj;typedef Obj* id;
static Obj H,SC,SC2,SET,CACHE,CACHE2,STATE,STATE2,PARENT,REMOTE,WINDOW,COPY;
static char fvGStateKey,fvGLeavingKey;static int flip,deadMessages,cacheWrites,submits;
static id send(id o,const char *sel,...){
 if(!o)return 0;if(o->refs<=0)deadMessages++;
 va_list ap;va_start(ap,sel);id result=0;
 if(!strcmp(sel,"retain")){o->refs++;result=o;}
 else if(!strcmp(sel,"release")){o->refs--;}
 else if(!strcmp(sel,"superview"))result=o->parent;
 else if(!strcmp(sel,"window"))result=o->window;
 else if(!strcmp(sel,"settings")){result=&SET;if(flip==7&&H.cached==&CACHE){CACHE.refs--;H.cached=&CACHE2;}}
 else if(!strcmp(sel,"mutableCopy")){COPY.refs=1;COPY.frame=o->frame;result=&COPY;}
 else if(!strcmp(sel,"updateSettings:withTransitionContext:completion:")){
  id copy=va_arg(ap,id);SET.frame=copy->frame;submits++;
  if(flip==1){CACHE.refs--;H.cached=&CACHE2;}
  if(flip==2)H.scene=&SC2;
  if(flip==3)H.parent=0;
  if(flip==4)H.state=&STATE2;
  if(flip==5)SET.frame.size.height=932;
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
static void setrect(id o,const char*s,Rect r){(void)s;if(o->refs<=0)deadMessages++;o->frame=r;if(o==&CACHE||o==&CACHE2)cacheWrites++;if(flip==6&&o==H.cached)H.parent=0;}
static BOOL fvGSameRect(Rect a,Rect b){return fabs(a.origin.x-b.origin.x)<.001&&fabs(a.origin.y-b.origin.y)<.001&&fabs(a.size.width-b.size.width)<.001&&fabs(a.size.height-b.size.height)<.001;}
static void trace(const char*s){(void)s;}
'''
new='fvGSceneFrame(id h,' in func
current=s[s.index('static BOOL fvGSceneContextCurrent('):start] if 'static BOOL fvGSceneContextCurrent(' in s else ''
main=r'''
static Rect target={{0,167.556},{430,764.444}};
static void reset(void){
 Obj*all[]={&H,&SC,&SC2,&SET,&CACHE,&CACHE2,&STATE,&STATE2,&PARENT,&REMOTE,&WINDOW,&COPY};
 for(unsigned i=0;i<sizeof(all)/sizeof(all[0]);i++){memset(all[i],0,sizeof(Obj));all[i]->refs=1;all[i]->frame=(Rect){{0,0},{430,932}};}
 H.scene=&SC;H.cached=&CACHE;H.state=&STATE;H.parent=&PARENT;H.remote=&REMOTE;H.window=&WINDOW;PARENT.remote=&H;REMOTE.parent=&H;REMOTE.window=&WINDOW;
 flip=deadMessages=cacheWrites=submits=0;
}
'''
call='fvGSceneFrame(&H,&SC,&SET,&CACHE,target)' if new else 'fvGSceneFrame(&SC,&SET,&CACHE,target)'
main+='int main(void){\n'
if not new:
 main+='reset();flip=1;int ok='+call+';printf("OLD result=%d retiredCacheWrites=%d deadMessages=%d\\n",ok,cacheWrites,deadMessages);assert(!ok&&deadMessages==0&&cacheWrites==0);\n'
else:
 main+='reset();assert('+call+');assert(cacheWrites==1&&submits==1&&!deadMessages);\n'
 main+='for(int f=1;f<=5;f++){reset();flip=f;assert(!'+call+');assert(cacheWrites==0&&!deadMessages);assert(H.refs==1&&SC.refs==1&&SET.refs==1&&PARENT.refs==1&&REMOTE.refs==1&&WINDOW.refs==1&&STATE.refs==1);assert(CACHE.refs==(f==1?0:1));}\n'
 main+='reset();SET.frame=target;assert('+call+');assert(cacheWrites==1&&!submits&&!deadMessages);\n'
 main+='reset();SET.frame=target;CACHE.frame=target;assert('+call+');assert(!cacheWrites&&!submits&&!deadMessages);\n'
 main+='reset();flip=6;assert(!'+call+');assert(cacheWrites==1&&!deadMessages);\n'
 main+='reset();flip=7;assert(!'+call+');assert(!cacheWrites&&!submits&&!deadMessages&&CACHE.refs==0);\n'
 main+='puts("PASS actual Scene commit: cache/scene/state/parent replacement, rejection, setter reentry, unchanged frame, ownership balance");\n'
main+='return 0;}\n'
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'test.c';p.write_text(prefix+current+func+main)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-Wno-misleading-indentation',str(p),'-lm','-o',str(Path(td)/'test')],check=True)
 subprocess.run([str(Path(td)/'test')],check=True)
