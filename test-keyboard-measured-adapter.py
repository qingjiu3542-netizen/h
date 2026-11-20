from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parent
s=(R/'FVKeyboardAppAvoidance.c').read_text()
def extract(name):
 start=s.index(name);start=s.rfind('static ',0,start);brace=s.index('{',start);depth=1;i=brace+1
 while depth:
  depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[start:i]
code=extract('int measuredEvidence(')+'\n'+extract('id mappedInfo(')
code=code.replace('((BOOL(*)(id,SEL))objc_msgSend)','fakeThread').replace('((id(*)(id,SEL,Rect))objc_msgSend)','fakeBox')
prefix=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "FVKeyboardAppMeasuredCore.h"
#include "FVKeyboardAppFirstEventCore.h"
typedef void *id;typedef void *SEL;typedef signed char BOOL;
typedef struct {double x,y;} Point;typedef struct{double width,height;} Size;typedef struct{Point origin;Size size;} Rect;
static int stateObject,infoObject,changedObject,screenObject,endObject,beginObject;
static double values[9],one=1,now=816842874;static int hasKeys=1,authorized=1,copyCount,setCount,logCount;
static unsigned rawFrameBudget=0,mappedReportBudget=4,firstSuccessBudget=6;
static Rect endRect,beginRect,outputEnd,outputBegin,boxes[8];static unsigned boxIndex;
static const char *fields[]={"keysVersion","keysSequence","keysStamp","keysX","keysY","keysWidth","keysHeight","stamp"};
static int equal(id a,id b){return a&&b&&!strcmp(a,b);}
static id text(const char *p){return (id)p;}
static int sameEventEvidence(id st,id obj,id info,FVKARect s,FVKARect t,FVKARect in,FVKARect *keys,int *route){(void)st;(void)obj;(void)info;(void)s;(void)t;(void)in;(void)keys;*route=0;return 0;}
#define S(x) ((SEL)(x))
static id objc_getClass(const char *p){return (id)p;}
static BOOL fakeThread(id object,SEL cmd){(void)object;(void)cmd;return 1;}
static id fakeBox(id object,SEL cmd,Rect rect){(void)object;(void)cmd;assert(boxIndex<8);boxes[boxIndex]=rect;return &boxes[boxIndex++];}
static id get(id object,const char *key){
 if(object==&stateObject){for(unsigned i=0;i<8;i++)if(!strcmp(fields[i],key))return (!hasKeys&&i<7)?0:&values[i];return 0;}
 if(object==&infoObject){if(!strcmp(key,"UIKeyboardIsLocalUserInfoKey"))return &one;if(!strcmp(key,"UIKeyboardFrameEndUserInfoKey"))return &endObject;if(!strcmp(key,"UIKeyboardFrameBeginUserInfoKey"))return &beginObject;}
 return 0;
}
static int isKind(id object,const char *name){
 if(!strcmp(name,"NSDictionary"))return object==&infoObject||object==&stateObject;
 if(!strcmp(name,"UIScreen"))return object==&screenObject;
 if(!strcmp(name,"NSNumber"))return object!=0;
 return 0;
}
static double number(id object){return object?*(double*)object:0;}
static double CFAbsoluteTimeGetCurrent(void){return now;}
static FVKARect core(Rect r){return (FVKARect){r.origin.x,r.origin.y,r.size.width,r.size.height};}
static int readBox(id object,Rect *out){if(object==&endObject){*out=endRect;return 1;}if(object==&beginObject){*out=beginRect;return 1;}return 0;}
static int eventKind(id name){return strcmp(name,"hide")!=0;}
static id mainBundleID(void){return text("com.tencent.xin");}
static id readState(id bundle,const char **reason,int *missing){(void)bundle;(void)reason;*missing=0;return &stateObject;}
static int localTarget(id state,id event,FVKARect *source,FVKARect *target,const char **reason){(void)state;(void)event;(void)reason;*source=(FVKARect){0,0,430,932};*target=(FVKARect){0,0,430,430*16.0/9};return authorized;}
static int liveTargetForScene(id e,id scene,FVKARect*s,FVKARect*t,const char**r){(void)e;(void)scene;(void)s;(void)t;(void)r;return 0;}
static void reportOnce(unsigned key,const char *message){(void)key;(void)message;}
static void writeAppStatus(unsigned stage){assert(stage>=1&&stage<=4);}
static void report(const char *message){(void)message;}
static void reportMapped(const char *message){(void)message;logCount++;}
static void appLog(const char *message){(void)message;logCount++;}
static id mutableCopy(id object,const char *method){assert(object==&infoObject&&!strcmp(method,"mutableCopy"));copyCount++;return &changedObject;}
static void setObject(id object,const char *method,id value,id key){assert(object==&changedObject&&!strcmp(method,"setObject:forKey:"));setCount++;if(!strcmp(key,"UIKeyboardFrameEndUserInfoKey"))outputEnd=*(Rect*)value;else{assert(!strcmp(key,"UIKeyboardFrameBeginUserInfoKey"));outputBegin=*(Rect*)value;}}
#define M0(o,m) mutableCopy(o,m)
#define M2(o,m,a,b) setObject(o,m,a,b)
static void reset(void){
 hasKeys=authorized=1;copyCount=setCount=logCount=boxIndex=0;mappedReportBudget=4;
 values[0]=1;values[1]=8;values[2]=now-.8;values[3]=0;values[4]=586;values[5]=430;values[6]=346;values[7]=now-.7;
 endRect=(Rect){{0,753.67},{430,178.33}};beginRect=(Rect){{0,932},{430,178.33}};
}
'''
suffix=r'''
int main(void){
 reset();assert(mappedInfo(text("show"),&screenObject,&infoObject)==&changedObject);assert(copyCount==1&&setCount==2);assert(fvKAFNear(outputEnd.size.height,346)&&fvKAFNear(outputEnd.origin.y,418.444444));assert(fvKAFNear(outputBegin.origin.y,764.444444)&&fvKAFNear(outputBegin.size.height,346));
 reset();values[4]=554;values[6]=378;endRect=(Rect){{0,721.67},{430,210.33}};beginRect=(Rect){{0,932},{430,210.33}};assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,378)&&fvKAFNear(outputEnd.origin.y,386.444444));
 reset();endRect=(Rect){{0,418.444444},{430,346}};assert(!mappedInfo(text("show"),&screenObject,&infoObject));assert(copyCount==0);
 reset();assert(!mappedInfo(text("hide"),&screenObject,&infoObject));assert(copyCount==0);
 reset();authorized=0;assert(!mappedInfo(text("show"),&screenObject,&infoObject));assert(copyCount==0);
 reset();endRect=(Rect){{20,753.67},{400,178.33}};assert(!mappedInfo(text("show"),&screenObject,&infoObject));assert(copyCount==0);
 reset();endRect.origin.y=932;assert(!mappedInfo(text("show"),&screenObject,&infoObject));assert(copyCount==0);
 reset();endRect.size.height=210.33;endRect.origin.y=721.67;assert(!mappedInfo(text("show"),&screenObject,&infoObject));assert(copyCount==0);
 puts("PASS actual measured notification adapter: 8 device-sample, begin/end, idempotency, hide and authorization cases");
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'adapter.c';p.write_text(prefix+code+suffix)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I',str(R),str(p),'-lm','-o',str(Path(td)/'test')],check=True)
 subprocess.run([str(Path(td)/'test')],check=True)
