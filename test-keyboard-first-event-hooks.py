from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parent
source=(R/'FVKeyboardAppFirstEvent.inc').read_text()
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
typedef struct{int valid;id scene,screen,ticketScene;double entered;} FVKAFirstContext;
typedef struct{FVKAFirstContext context;FVKAFirstContext *previous;unsigned captureDepth;} FVKAFirstScope;
static _Thread_local FVKAFirstContext *firstContext;
static _Thread_local unsigned firstCaptureDepth;
static IMP oldPrepareScreen;
static int calls,captures,releases,eligible,nest,inner,raiseNow;
#ifdef THROWHARNESS
extern void throwNative(void);
#else
static void throwNative(void){assert(0);}
#endif
static Rect expectedPlain={{0,612},{430,320}},expectedIAV={{0,554},{430,378}};
static id expectedRemote=(id)1,expectedScreen=(id)2,expectedScene=(id)3;
static SEL expectedCmd=(SEL)4;static BOOL expectedShowing;
static FVKAFirstContext sentinel={1,(id)99,0,0,0};
static double CFAbsoluteTimeGetCurrent(void){return 816844601.872;}
static id firstSceneForScreen(id screen){assert(screen==expectedScreen);return expectedScene;}
static void captureFirstContext(FVKAFirstContext *ctx,Rect plain,Rect iav,BOOL relevant,BOOL showing,id scene){
 assert(firstCaptureDepth==1);assert(!memcmp(&plain,&expectedPlain,sizeof plain));assert(!memcmp(&iav,&expectedIAV,sizeof iav));assert(relevant==1&&scene==expectedScene);captures++;
 ctx->valid=eligible&&showing;
 if(ctx->valid){ctx->scene=expectedScene;ctx->screen=expectedScreen;ctx->ticketScene=(id)5;}
}
static id releaseObject(id o,const char *cmd){assert(o&&!strcmp(cmd,"release"));releases++;return 0;}
#define M0(o,c) releaseObject(o,c)
static void prepareScreenHook(id,SEL,Rect,Rect,BOOL,id);
static void originalPrepare(id remote,SEL cmd,Rect plain,Rect iav,BOOL showing,id screen){
 calls++;assert(remote==expectedRemote&&cmd==expectedCmd&&screen==expectedScreen&&showing==expectedShowing);
 assert(!memcmp(&plain,&expectedPlain,sizeof plain)&&!memcmp(&iav,&expectedIAV,sizeof iav));
 assert(firstContext&&firstContext!=&sentinel);assert(firstContext->valid==(eligible&&showing&&!firstCaptureDepth));
 if(raiseNow)throwNative();
 FVKAFirstContext *current=firstContext;
 if(nest&&!inner){inner=1;prepareScreenHook(remote,cmd,plain,iav,showing,screen);assert(firstContext==current);}
}
static void reset(void){calls=captures=releases=nest=inner=raiseNow=0;eligible=1;firstCaptureDepth=0;expectedShowing=1;firstContext=&sentinel;}
'''
suffix=r'''
int main(void){
 oldPrepareScreen=(IMP)originalPrepare;
 reset();prepareScreenHook(expectedRemote,expectedCmd,expectedPlain,expectedIAV,expectedShowing,expectedScreen);assert(calls==1&&captures==1&&releases==3&&firstContext==&sentinel);
 reset();expectedShowing=0;prepareScreenHook(expectedRemote,expectedCmd,expectedPlain,expectedIAV,expectedShowing,expectedScreen);assert(calls==1&&captures==1&&!releases&&firstContext==&sentinel);
 reset();eligible=0;prepareScreenHook(expectedRemote,expectedCmd,expectedPlain,expectedIAV,expectedShowing,expectedScreen);assert(calls==1&&captures==1&&!releases&&firstContext==&sentinel);
 reset();nest=1;prepareScreenHook(expectedRemote,expectedCmd,expectedPlain,expectedIAV,expectedShowing,expectedScreen);assert(calls==2&&captures==2&&releases==6&&firstContext==&sentinel);
 reset();firstCaptureDepth=1;prepareScreenHook(expectedRemote,expectedCmd,expectedPlain,expectedIAV,expectedShowing,expectedScreen);assert(calls==1&&!captures&&!releases&&firstContext==&sentinel&&firstCaptureDepth==1);
 reset();firstContext=0;prepareScreenHook(expectedRemote,expectedCmd,expectedPlain,expectedIAV,expectedShowing,expectedScreen);assert(calls==1&&releases==3&&!firstContext);
 puts("PASS actual first-event native hook: 6 original-once, argument preservation, nesting and no-persistent-context cases");
 return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'first-hook.c';p.write_text(prefix+extract('void closeFirstScope(')+extract('void prepareScreenHook(')+suffix+'''
void runNativeThrow(void){oldPrepareScreen=(IMP)originalPrepare;reset();raiseNow=1;prepareScreenHook(expectedRemote,expectedCmd,expectedPlain,expectedIAV,expectedShowing,expectedScreen);assert(0);}
int restoredAfterThrow(void){return calls==1&&captures==1&&releases==3&&firstContext==&sentinel&&firstCaptureDepth==0;}
''')
 subprocess.run(['cc','-std=c11','-fexceptions','-Wall','-Wextra','-Werror',str(p),'-o',str(Path(td)/'test')],check=True)
 subprocess.run([str(Path(td)/'test')],check=True)
 obj=Path(td)/'first-hook.o'
 subprocess.run(['cc','-std=c11','-fexceptions','-Wall','-Wextra','-Werror','-DTHROWHARNESS','-Dmain=normalMain','-c',str(p),'-o',str(obj)],check=True)
 driver=Path(td)/'unwind.cpp';driver.write_text('''
#include <cassert>
#include <cstdio>
extern "C" void runNativeThrow(void);
extern "C" int restoredAfterThrow(void);
extern "C" void throwNative(void){throw 17;}
int main(){try{runNativeThrow();assert(false);}catch(int value){assert(value==17);assert(restoredAfterThrow());}std::puts("PASS actual first-event hook: native exception unwinding restores prior context and releases evidence");}
''')
 subprocess.run(['c++','-std=c++11',str(driver),str(obj),'-o',str(Path(td)/'unwind')],check=True)
 subprocess.run([str(Path(td)/'unwind')],check=True)
assert 'dispatch_after' not in source and 'dispatch_async' not in source
assert 'postNotification' not in source and 'usleep' not in source and 'sleep(' not in source
assert 'contextOK=shape==2' in source
assert 'prepareToMoveKeyboard:withIAV:showing:forScreen:' in source
assert "'keyboard-app-avoidance':['-fexceptions']" in (R/'ci/apple-build.py').read_text()
assert '__attribute__((cleanup(closeFirstScope)))' in source
print('PASS first-event policy: exact runtime ABI, synchronous stack lifetime, no delayed work or notification replay')
