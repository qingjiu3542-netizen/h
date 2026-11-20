from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parent
source=(R/'FVKeyboardHostRouteProbe.inc').read_text()
def extract(name):
 start=source.index(name);start=source.rfind('static ',0,start);brace=source.index('{',start);depth=1;i=brace+1
 while depth:
  depth+=(source[i]=='{')-(source[i]=='}');i+=1
 return source[start:i]
prefix=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef void *id;typedef void *SEL;typedef signed char BOOL;typedef void(*IMP)(void);
typedef struct{double x,y;} Point;typedef struct{double width,height;} Size;typedef struct{Point origin;Size size;} Rect;
typedef struct{unsigned previous;} FVKHScope;
static _Thread_local unsigned fvKHDepth;
static IMP fvKHShowOriginal,fvKHArbiterOriginal,fvKHPrepareOriginal,fvKHSignalOriginal,fvKHQueueOriginal;
static unsigned calls[5],records[5];static int nested,raiseNow;
#ifdef THROWHARNESS
extern void throwNative(void);
#else
static void throwNative(void){assert(0);}
#endif
static id receiver=(id)1,scene=(id)2,views=(id)3,placement=(id)4,state=(id)5,completion=(id)6;static SEL cmd=(id)7;
static Rect a={{0,554},{430,378}},b={{0,522},{430,410}};
static void fvKHArbiter(id,SEL,Rect,Rect,BOOL,BOOL,BOOL,id);
static void fvKHPrepare(id,SEL,Rect,Rect,BOOL,BOOL,BOOL,id);
static void fvKHSignal(id,SEL,id,id);
static id fvKHObject(id self,const char *selector){assert(self==receiver&&!strcmp(selector,"requiredScene"));return scene;}
static void fvKHRecord(unsigned kind,id self,id s,Rect plain,Rect iav,int relevant,int showing,int notify,id object){
 assert(kind<5&&self==receiver&&s==scene);records[kind]++;
 if(kind==1||kind==2){assert(!memcmp(&plain,&a,sizeof a)&&!memcmp(&iav,&b,sizeof b));assert(relevant==1&&showing==1&&notify==0);}
 else assert(relevant==-1&&showing==-1&&notify==-1);
 if(kind>=3)assert(object==state);
}
static BOOL origShow(id self,SEL selector,id v,id p,id s){assert(self==receiver&&selector==cmd&&v==views&&p==placement&&s==scene);calls[0]++;if(nested)fvKHArbiter(self,cmd,a,b,1,1,0,s);return (BOOL)1;}
static void origArbiter(id self,SEL selector,Rect plain,Rect iav,BOOL relevant,BOOL showing,BOOL notify,id s){assert(self==receiver&&selector==cmd&&s==scene);assert(!memcmp(&plain,&a,sizeof a)&&!memcmp(&iav,&b,sizeof b));assert(relevant==1&&showing==1&&notify==0);calls[1]++;if(nested)fvKHPrepare(self,cmd,plain,iav,relevant,showing,notify,s);}
static void origPrepare(id self,SEL selector,Rect plain,Rect iav,BOOL relevant,BOOL showing,BOOL notify,id s){assert(self==receiver&&selector==cmd&&s==scene);assert(!memcmp(&plain,&a,sizeof a)&&!memcmp(&iav,&b,sizeof b));assert(relevant==1&&showing==1&&notify==0);calls[2]++;if(raiseNow)throwNative();if(nested)fvKHSignal(self,cmd,state,completion);}
static void origSignal(id self,SEL selector,id object,id block){assert(self==receiver&&selector==cmd&&object==state&&block==completion);calls[3]++;}
static void origQueue(id self,SEL selector,id object,id block){assert(self==receiver&&selector==cmd&&object==state&&block==completion);calls[4]++;}
static void  reset(void){memset(calls,0,sizeof calls);memset(records,0,sizeof records);fvKHDepth=0;nested=0;raiseNow=0;}
'''
code='\n'.join(extract(name) for name in ['void fvKHClose(','BOOL fvKHShow(','void fvKHArbiter(','void fvKHPrepare(','void fvKHSignal(','void fvKHQueue('])
suffix=r'''
int main(void){
 fvKHShowOriginal=(IMP)origShow;fvKHArbiterOriginal=(IMP)origArbiter;fvKHPrepareOriginal=(IMP)origPrepare;fvKHSignalOriginal=(IMP)origSignal;fvKHQueueOriginal=(IMP)origQueue;
 reset();assert(fvKHShow(receiver,cmd,views,placement,scene)==1);assert(calls[0]==1&&records[0]==1&&!fvKHDepth);
 reset();fvKHArbiter(receiver,cmd,a,b,1,1,0,scene);assert(calls[1]==1&&records[1]==1&&!fvKHDepth);
 reset();fvKHPrepare(receiver,cmd,a,b,1,1,0,scene);assert(calls[2]==1&&records[2]==1&&!fvKHDepth);
 reset();fvKHSignal(receiver,cmd,state,completion);assert(calls[3]==1&&records[3]==1&&!fvKHDepth);
 reset();fvKHQueue(receiver,cmd,state,completion);assert(calls[4]==1&&records[4]==1&&!fvKHDepth);
 reset();nested=1;assert(fvKHShow(receiver,cmd,views,placement,scene)==1);for(unsigned i=0;i<4;i++)assert(calls[i]==1&&records[i]==1);assert(!fvKHDepth);
 reset();fvKHDepth=1u<<2;fvKHPrepare(receiver,cmd,a,b,1,1,0,scene);assert(calls[2]==1&&records[2]==0&&fvKHDepth==(1u<<2));
 puts("PASS actual host-route wrappers: original-once, unchanged CGRect/BOOL/object/completion/return, nested-route visibility and scoped recursion");
 return 0;
}
void runNativeThrow(void){fvKHPrepareOriginal=(IMP)origPrepare;reset();raiseNow=1;fvKHPrepare(receiver,cmd,a,b,1,1,0,scene);assert(0);}
int restoredAfterThrow(void){return calls[2]==1&&records[2]==1&&!fvKHDepth;}
'''''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'route.c';p.write_text(prefix+code+suffix)
 subprocess.run(['cc','-std=c11','-fexceptions','-Wall','-Wextra','-Werror',str(p),'-o',str(Path(td)/'test')],check=True)
 subprocess.run([str(Path(td)/'test')],check=True)
 obj=Path(td)/'route.o'
 subprocess.run(['cc','-std=c11','-fexceptions','-Wall','-Wextra','-Werror','-DTHROWHARNESS','-Dmain=normalMain','-c',str(p),'-o',str(obj)],check=True)
 driver=Path(td)/'unwind.cpp';driver.write_text('''
#include <cassert>
#include <cstdio>
extern "C" void runNativeThrow(void);
extern "C" int restoredAfterThrow(void);
extern "C" void throwNative(void){throw 17;}
int main(){try{runNativeThrow();assert(false);}catch(int value){assert(value==17);assert(restoredAfterThrow());}std::puts("PASS host route: original C++ exception unwinding restores nested route guard");}
''')
 subprocess.run(['c++','-std=c++11',str(driver),str(obj),'-o',str(Path(td)/'unwind')],check=True)
 subprocess.run([str(Path(td)/'unwind')],check=True)
for forbidden in ['setCurrent','setFrame:', 'setBounds:', 'setPlacement:', 'setRequiredScene:', 'postNotification', 'object_setIvar', 'dispatch_after', 'dispatch_async', 'setObject:forKey:', 'updateSettings:']:
 assert forbidden not in source,forbidden
assert 'FV_KEYBOARD_HOST_ROUTE_PROBE' in source
assert 'weakObjectsHashTable' in source
assert 'class_getInstanceMethod' in source and 'fvKHEncoding(actual,expected)' in source
print('PASS host-route policy: no state/geometry/focus/event writes, no delayed work, ABI-gated endpoints, weak live-host references')
