from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parent
s=(R/'FVKeyboardHostRouteProbe.inc').read_text()
def extract(name):
 start=s.index(name);start=s.rfind('static ',0,start);brace=s.index('{',start);depth=1;i=brace+1
 while depth:
  depth+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[start:i]
prefix=r'''
#include <assert.h>
#include <stdio.h>
typedef void *id;typedef void *SEL;typedef _Bool FVKHBool;typedef void(*IMP)(void);
typedef struct{double x,y;} Point;typedef struct{double width,height;} Size;typedef struct{Point origin;Size size;} Rect;
typedef struct{unsigned previous;} FVKHScope;
static _Thread_local unsigned fvKHDepth;
static IMP fvKHIAVHeightOriginal;
static int calls,records;static id expectedSelf=(id)1,expectedScene=(id)2;static SEL expectedCmd=(SEL)3;
static FVKHBool expectedDestination,expectedRelevant,expectedNotify;
static double expectedHeight;
static void fvKHClose(FVKHScope *scope){fvKHDepth=scope->previous;}
static int fvKHTake(unsigned *value,unsigned cap){if(*value>=cap)return 0;(*value)++;return 1;}
static void fvKHLog(const char *line){(void)line;}
static void fvKHRecord(unsigned kind,id self,id scene,Rect plain,Rect iav,int relevant,int showing,int notify,id state,id views,id place){
 (void)iav;(void)state;(void)views;(void)place;
 assert(kind==9&&self==expectedSelf&&scene==expectedScene&&plain.size.width==0&&plain.size.height==0);
 assert(relevant==expectedRelevant&&showing==expectedDestination&&notify==expectedNotify);records++;
}
static void originalIAV(id self,SEL cmd,FVKHBool destination,double height,FVKHBool relevant,FVKHBool notify,id scene){
 assert(self==expectedSelf&&cmd==expectedCmd&&scene==expectedScene&&destination==expectedDestination&&height==expectedHeight&&relevant==expectedRelevant&&notify==expectedNotify);calls++;
}
static void reset(void){calls=records=0;fvKHDepth=0;expectedDestination=1;expectedRelevant=1;expectedNotify=0;expectedHeight=378.25;}
'''
suffix=r'''
int main(void){fvKHIAVHeightOriginal=(IMP)originalIAV;
 reset();fvKHIAVHook(expectedSelf,expectedCmd,expectedDestination,expectedHeight,expectedRelevant,expectedNotify,expectedScene);assert(calls==1&&records==1&&!fvKHDepth);
 reset();expectedDestination=0;expectedRelevant=0;expectedNotify=1;expectedHeight=210.33;
 fvKHIAVHook(expectedSelf,expectedCmd,expectedDestination,expectedHeight,expectedRelevant,expectedNotify,expectedScene);assert(calls==1&&records==1&&!fvKHDepth);
 reset();fvKHDepth=1u<<9;fvKHIAVHook(expectedSelf,expectedCmd,expectedDestination,expectedHeight,expectedRelevant,expectedNotify,expectedScene);assert(calls==1&&records==0&&fvKHDepth==(1u<<9));
 puts("PASS actual IAV delivery hook: 3 original-once, height/scene/flag preservation and nested route cases");return 0;}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'iav.c';p.write_text(prefix+extract('void fvKHIAVHook(')+suffix)
 subprocess.run(['cc','-std=c11','-fexceptions','-Wall','-Wextra','-Werror',str(p),'-o',str(Path(td)/'test')],check=True)
 subprocess.run([str(Path(td)/'test')],check=True)
assert 'inputDestinationDidChange:withIAVHeight:isIAVRelevant:notifyRemote:forScene:' in s
assert '"v@:BdBB@"' in s
print('PASS IAV policy: exact FVKHBool/CGFloat/object selector, read-only and preserved')
