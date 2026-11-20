"""Execute the actual installer and wrappers; no device/UI claims."""
from pathlib import Path
import subprocess, tempfile
R=Path(__file__).resolve().parent
source=(R/'FVKeyboardHostRouteProbe.inc').read_text()
def function(name):
 start=source.index(name); opening=source.index('{',start); depth=1; end=opening+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}'); end+=1
 return source[start:end]
def run_c(text,label):
 with tempfile.TemporaryDirectory() as td:
  p=Path(td)/'test.c'; p.write_text(text); exe=Path(td)/'test'
  subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-cast-function-type',str(p),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
 print('PASS '+label)
common=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
typedef void *id;typedef const char *SEL;typedef void *Class;typedef void *Method;
typedef void (*IMP)(void);typedef _Bool FVKHBool;
typedef struct {double x,y;} Point;typedef struct {double width,height;} Size;
typedef struct {Point origin;Size size;} Rect;
#define S(x) (x)
'''
# Model method ownership and argument counts separately from encoding strings.
installer=common+r'''
#include "FVKeyboardHostRouteCore.h"
typedef struct {int owner;const char *selector,*types;unsigned argc;IMP imp;} Entry;
static Entry entries[24];static unsigned n,changes;static int rejects;
static const char *classNames[]={"UIKeyboardSceneDelegate","_UIKeyboardArbiterClient","_UIKeyboardArbiterClientSpringBoard","_UIKeyboardArbiterClientInputUIHost","_UIKeyboardArbiterClientInputDestination","_UIRemoteKeyboards"};
static void original(void){} static void replacement(void){}
static Class objc_getClass(const char *s){for(unsigned i=0;i<6;i++)if(!strcmp(s,classNames[i]))return (Class)(size_t)(i+1);return 0;}
static Method class_getInstanceMethod(Class c,SEL s){int owner=(int)(size_t)c;for(unsigned i=0;i<n;i++)if(entries[i].owner==owner&&!strcmp(entries[i].selector,s))return &entries[i];if(owner>=3&&owner<=5)return class_getInstanceMethod((Class)2,s);return 0;}
static const char *method_getTypeEncoding(Method m){return ((Entry*)m)->types;}
static unsigned method_getNumberOfArguments(Method m){return ((Entry*)m)->argc;}
static IMP method_getImplementation(Method m){return ((Entry*)m)->imp;}
static Method *class_copyMethodList(Class c,unsigned *out){Method *a=malloc(n*sizeof *a);*out=0;for(unsigned i=0;i<n;i++)if(entries[i].owner==(int)(size_t)c)a[(*out)++]=&entries[i];return a;}
static IMP class_replaceMethod(Class c,SEL s,IMP value,const char *types){Entry *e=class_getInstanceMethod(c,s);assert(e&&e->owner==(int)(size_t)c&&!strcmp(e->types,types));IMP old=e->imp;e->imp=value;changes++;return old;}
static void fvKHLog(const char *s){if(strstr(s,"mismatch"))rejects++;}
static IMP fvKHShowOriginal,fvKHArbiterOriginal,fvKHPrepareOriginal,fvKHRemoteChangedOriginal,fvKHIAVHeightOriginal;
static IMP fvKHSignalImp[5],fvKHQueueImp[5],fvKHQueueWithCompletionImp[5],fvKHDidHandleImp[5];
#define fvKHShow replacement
#define fvKHArbiter replacement
#define fvKHPrepare replacement
#define fvKHRemoteChanged replacement
#define fvKHIAVHook replacement
static IMP fvKHSignalWrappers[5]={replacement,replacement,replacement,replacement,replacement};
static IMP fvKHQueueWrappers[5]={replacement,replacement,replacement,replacement,replacement};
static IMP fvKHQueueCompletionWrappers[5]={replacement,replacement,replacement,replacement,replacement};
static IMP fvKHDidHandleWrappers[5]={replacement,replacement,replacement,replacement,replacement};
static void add(int owner,const char *selector,const char *types,unsigned argc){entries[n++]=(Entry){owner,selector,types,argc,original};}
'''
installer+=function('static int fvKHHook(')+'\n'+function('static void fvKHInstall(')
installer+=r'''
int main(void){
 add(1,"_showKeyboardWindowForInputViewSet:placement:windowScene:","B40@0:8@16@24@32",5);
 const char *r="{CGRect={CGPoint=dd}{CGSize=dd}}";char rectTypes[180];snprintf(rectTypes,sizeof rectTypes,"v@:%s%sBBB@",r,r);
 add(1,"updateArbiterClientWithKeyboardFrame:keyboarFrameWithIAV:isIAVRelevant:showing:notifyRemote:forScene:",rectTypes,8);
 add(6,"prepareToMoveKeyboard:withIAV:isIAVRelevant:showing:notifyRemote:forScene:",rectTypes,8);
 add(2,"signalToProxyKeyboardChanged:onCompletion:","v32@0:8@16@?24",4);
 add(2,"queue_keyboardChanged:onComplete:","v@:@@?",4);
 add(2,"queue_keyboardChangedWithCompletion:","v@:@?",3);
 add(2,"_didHandleKeyboardClientChange:shouldConsiderSnapshottingKeyboard:isLocalEvent:","B32@0:8@16B24B28",5);
 add(5,"_didHandleKeyboardClientChange:shouldConsiderSnapshottingKeyboard:isLocalEvent:","B@:@BB",5);
 add(6,"didHandleKeyboardChange:shouldConsiderSnapshottingKeyboard:isLocalEvent:","B@:@BB",5);
 add(5,"inputDestinationDidChange:withIAVHeight:isIAVRelevant:notifyRemote:forScene:","v@:BdBB@",7);
 fvKHInstall();assert(changes==10);
 assert(fvKHRemoteChangedOriginal==original&&fvKHDidHandleImp[0]==original&&fvKHDidHandleImp[3]==original);
 assert(!fvKHDidHandleImp[1]&&!fvKHDidHandleImp[2]);
 fvKHInstall();assert(changes==10); /* idempotent: never save our own wrapper */
 IMP slot=0;
 assert(!fvKHHook(classNames[5],"didHandleKeyboardChange:shouldConsiderSnapshottingKeyboard:isLocalEvent:","B@:@@BB",replacement,&slot)&&!slot);
 entries[8].argc=6;
 assert(!fvKHHook(classNames[5],entries[8].selector,"B@:@BB",replacement,&slot)&&!slot);
 entries[8].argc=5;entries[8].types="c@:@cc";
 assert(!fvKHHook(classNames[5],entries[8].selector,"B@:@BB",replacement,&slot)&&!slot);
 assert(rejects==3&&changes==10);return 0;
}
'''
# Include path is absolute to keep temporary compilation independent of cwd.
installer=installer.replace('#include "FVKeyboardHostRouteCore.h"','#include "'+str(R/'FVKeyboardHostRouteCore.h')+'"')
run_c(installer,'actual installer: iOS16 selectors, offsets, argument counts, inherited methods, idempotency and ABI rejection')

wrappers=common+r'''
static unsigned fvKHDepth,calls,records;static int isMain=1;static FVKHBool answer,expectedA,expectedB;
static id expectedSelf=(id)1,expectedInfo=(id)2;static SEL expectedCmd="didHandle";
static int fvGMain(void){return isMain;}
static id fvKHObject(id self,const char *key){(void)self;(void)key;assert(isMain);return (id)3;}
typedef struct {unsigned previous;} FVKHScope;
static void fvKHClose(FVKHScope *s){fvKHDepth=s->previous;}
static IMP fvKHRemoteChangedOriginal,fvKHDidHandleImp[5];
static void fvKHRecord(unsigned kind,id self,id context,Rect plain,Rect iav,int a,int b,int notify,id state,id views,id place){
 (void)plain;(void)iav;(void)views;(void)place;assert(kind==5||kind==6);
 assert(self==expectedSelf&&state==expectedInfo&&a==expectedA&&b==expectedB&&notify==-1);
 assert(context==(isMain?(id)3:0));records++;
}
static FVKHBool realRemote(id self,SEL cmd,id info,FVKHBool a,FVKHBool b){
 assert(self==expectedSelf&&cmd==expectedCmd&&info==expectedInfo&&a==expectedA&&b==expectedB);calls++;return answer;
}
'''
wrappers+=function('static FVKHBool fvKHDidHandleBody(')+'\n'+function('static FVKHBool fvKHRemoteChanged(')
wrappers+=r'''
static FVKHBool nestedRemote(id self,SEL cmd,id info,FVKHBool a,FVKHBool b){
 calls++;fvKHRemoteChangedOriginal=(IMP)realRemote;
 return fvKHRemoteChanged(self,cmd,info,a,b);
}
int main(void){
 for(unsigned flags=0;flags<8;flags++)for(isMain=0;isMain<2;isMain++){
  expectedA=(flags&1)!=0;expectedB=(flags&2)!=0;answer=(flags&4)!=0;
  unsigned before=calls,logged=records;fvKHRemoteChangedOriginal=(IMP)realRemote;fvKHDidHandleImp[0]=(IMP)realRemote;
  assert(fvKHRemoteChanged(expectedSelf,expectedCmd,expectedInfo,expectedA,expectedB)==answer);
  assert(fvKHDidHandleBody(0,expectedSelf,expectedCmd,expectedInfo,expectedA,expectedB)==answer);
  assert(calls==before+2&&records==logged+2&&fvKHDepth==0);
 }
 isMain=1;unsigned before=calls,logged=records;fvKHRemoteChangedOriginal=(IMP)nestedRemote;
 assert(fvKHRemoteChanged(expectedSelf,expectedCmd,expectedInfo,expectedA,expectedB)==answer);
 assert(calls==before+2&&records==logged+1&&fvKHDepth==0);return 0;
}
'''
run_c(wrappers,'actual bool handlers: 32 forwards, true/false returns, worker no-getter and nested-route preservation')
