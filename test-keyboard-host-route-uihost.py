from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parent
source=(R/'FVKeyboardHostRouteProbe.inc').read_text()
def function(name):
 start=source.index(name); opening=source.index('{',start); depth=1; end=opening+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}'); end+=1
 return source[start:end]
common=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
typedef void *id; typedef const char *SEL; typedef void (*IMP)(void); typedef _Bool FVKHBool;
typedef struct{double x,y;}Point;typedef struct{double width,height;}Size;typedef struct{Point origin;Size size;}Rect;
static _Thread_local unsigned fvKHDepth;static volatile unsigned fvKHCurrentMode;
static IMP fvKHUIHostVisibilityOriginal;static unsigned fvKHUIHostCounts[2][2],fvKHUIHostSequence;
static unsigned forwards,records;static int mainThread=1,ratio=0,nestOnce;static id expectedSelf=(id)1;static SEL expectedCmd="keyboardVisibilityDidChangeWithFrame:visible:tracking:";static Rect expectedFrame;static FVKHBool expectedVisible,expectedTracking;static char lastLog[512];
typedef struct{unsigned previous;}FVKHScope;static void fvKHClose(FVKHScope*s){fvKHDepth=s->previous;}
static void fvKHUIHostVisibility(id,SEL,Rect,FVKHBool,FVKHBool);
static int fvGMain(void){return mainThread;}static int enabled(void){return ratio;}
static int fvKHTake(unsigned *s,unsigned cap){if(*s>=cap)return 0;(*s)++;return 1;}
static double CFAbsoluteTimeGetCurrent(void){return 123.5;}static int getpid(void){return 999;}
static void *object_getClass(id x){return x;}static const char *class_getName(void *x){(void)x;return "_UIKeyboardArbiterClientInputUIHost";}
static void fvKHLog(const char*s){records++;snprintf(lastLog,sizeof lastLog,"%s",s);}
static void original(id self,SEL cmd,Rect frame,FVKHBool visible,FVKHBool tracking){assert(self==expectedSelf&&cmd==expectedCmd);assert(!memcmp(&frame,&expectedFrame,sizeof frame));assert(visible==expectedVisible&&tracking==expectedTracking);forwards++;if(nestOnce){nestOnce=0;fvKHUIHostVisibility(self,cmd,frame,visible,tracking);}}
'''
body=function('static void fvKHUIHostVisibility(')
test=r'''
int main(void){
 expectedFrame=(Rect){{0,386.444},{430,378}};fvKHUIHostVisibilityOriginal=(IMP)original;
 for(int m=0;m<2;m++)for(int v=0;v<2;v++)for(int t=0;t<2;t++){
  ratio=m;expectedVisible=v;expectedTracking=t;expectedSelf=(id)1;unsigned before=forwards,logs=records;
  fvKHUIHostVisibility(expectedSelf,expectedCmd,expectedFrame,v,t);
  assert(forwards==before+1&&records==logs+1&&fvKHDepth==0);
  assert(strstr(lastLog,m?"mode=ratio9x16":"mode=native"));assert(strstr(lastLog,"frame=(0.000,386.444,430.000,378.000)"));
 }
 expectedFrame=(Rect){{0,0},{0,0}};expectedVisible=0;expectedTracking=0;ratio=0;unsigned logs=records,before=forwards;
 fvKHUIHostVisibility(expectedSelf,expectedCmd,expectedFrame,0,0);assert(records==logs+1&&forwards==before+1);
 expectedFrame=(Rect){{0,386.444},{430,378}};expectedVisible=1;expectedTracking=1;ratio=1;logs=records;before=forwards;nestOnce=1;
 fvKHUIHostVisibility(expectedSelf,expectedCmd,expectedFrame,1,1);assert(records==logs+1&&forwards==before+2&&fvKHDepth==0);
 mainThread=0;fvKHCurrentMode=1;expectedVisible=0;expectedTracking=1;logs=records;before=forwards;
 fvKHUIHostVisibility(expectedSelf,expectedCmd,expectedFrame,0,1);assert(records==logs+1&&forwards==before+1&&strstr(lastLog,"main=0")&&strstr(lastLog,"mode=ratio9x16"));return 0;
}
'''
text=common+'\n'+body+'\n'+test
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'test.c';p.write_text(text);exe=Path(td)/'test'
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-cast-function-type',str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('PASS actual UIHost visibility wrapper: CGRect/BOOL forwarding, modes, nesting, worker snapshot')
