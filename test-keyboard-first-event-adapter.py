from pathlib import Path
import runpy,subprocess,tempfile
R=Path(__file__).resolve().parent
base=runpy.run_path(str(R/'test-keyboard-measured-adapter.py'))
source=(R/'FVKeyboardAppFirstEvent.inc').read_text()
def extract(name):
 start=source.index(name);start=source.rfind('static ',0,start);brace=source.index('{',start);depth=1;i=brace+1
 while depth:
  depth+=(source[i]=='{')-(source[i]=='}');i+=1
 return source[start:i]
prefix=base['prefix']
stub='static int sameEventEvidence(id st,id obj,id info,FVKARect s,FVKARect t,FVKARect in,FVKARect *keys,int *route){(void)st;(void)obj;(void)info;(void)s;(void)t;(void)in;(void)keys;*route=0;return 0;}'
assert stub in prefix
prefix=prefix.replace(stub,'')
prefix=prefix.replace('static int stateObject,','static int boundsObject,boundsPresent,liveAllowed=1;static Rect boundsRect;\nstatic int stateObject,')
prefix=prefix.replace('if(object==&stateObject){','if(object==&stateObject){if(!strcmp(key,"scene"))return text("scene-A");')
prefix=prefix.replace('if(object==&infoObject){','if(object==&infoObject){if(!strcmp(key,"UIKeyboardBoundsUserInfoKey"))return boundsPresent?&boundsObject:0;')
prefix=prefix.replace('static int readBox(id object,Rect *out){','static int readBox(id object,Rect *out){if(object==&boundsObject){*out=boundsRect;return 1;}')
prefix=prefix.replace('(void)e;(void)scene;(void)s;(void)t;(void)r;return 0;','(void)e;(void)scene;(void)r;*s=(FVKARect){0,0,430,932};*t=(FVKARect){0,0,430,430*16.0/9};return liveAllowed;')
prefix+='''
typedef struct {int valid;id scene,screen,ticketScene;FVKARect source,target,keys;double entered;} FVKAFirstContext;
static FVKAFirstContext *firstContext;
static unsigned firstProbeBudget=6;
static FVKAFirstContext context;
static void fresh(void){reset();hasKeys=0;boundsPresent=0;liveAllowed=1;firstContext=0;firstSuccessBudget=6;firstProbeBudget=6;}
static void nativeContext(void){context=(FVKAFirstContext){1,(id)1,&screenObject,(id)"scene-A",{0,0,430,932},{0,0,430,430*16.0/9},{0,586,430,346},now};firstContext=&context;}
'''
suffix=r'''
int main(void){
 /* Cold first Will event, no publisher sample, full same-event bounds. */
 fresh();boundsPresent=1;boundsRect=(Rect){{0,0},{430,346}};
 assert(mappedInfo(text("UIKeyboardWillShowNotification"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,346)&&fvKAFNear(outputEnd.origin.y,418.444444));assert(fvKAFNear(outputBegin.origin.y,764.444444)&&fvKAFNear(outputBegin.size.height,346));assert(setCount==2);
 fresh();boundsPresent=1;boundsRect=(Rect){{0,0},{430,378}};endRect=(Rect){{0,721.67},{430,210.33}};beginRect=(Rect){{0,932},{430,210.33}};
 assert(mappedInfo(text("UIKeyboardWillChangeFrameNotification"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.origin.y,386.444444)&&fvKAFNear(outputEnd.size.height,378));
 /* Missing optional keys and absent synchronous context preserve 0.1.18. */
 fresh();assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,178.33));
 fresh();hasKeys=1;boundsPresent=1;boundsRect=(Rect){{0,0},{430,178.33}};assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,346));
 fresh();boundsPresent=1;boundsRect=(Rect){{0,0},{430,320}};assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,178.33));
 fresh();boundsPresent=1;boundsRect=(Rect){{0,0},{430,346}};authorized=0;assert(!mappedInfo(text("show"),&screenObject,&infoObject));assert(!copyCount);
 fresh();boundsPresent=1;boundsRect=(Rect){{0,0},{430,346}};assert(!mappedInfo(text("hide"),&screenObject,&infoObject));assert(!copyCount);
 fresh();boundsPresent=1;boundsRect=(Rect){{0,0},{430,346}};endRect=(Rect){{0,418.444444},{430,346}};assert(!mappedInfo(text("show"),&screenObject,&infoObject));assert(!copyCount);
 /* A valid synchronous preparation works without legacy bounds. */
 fresh();nativeContext();assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,346));
 fresh();nativeContext();context.ticketScene=text("scene-B");assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,178.33));
 fresh();nativeContext();context.screen=(id)2;assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,178.33));
 fresh();nativeContext();liveAllowed=0;assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,178.33));
 fresh();nativeContext();context.valid=0;assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,178.33));
 fresh();nativeContext();context.target.height=700;assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,178.33));
 fresh();nativeContext();context.keys=(FVKARect){0,554,430,378};assert(mappedInfo(text("show"),&screenObject,&infoObject));assert(fvKAFNear(outputEnd.size.height,178.33));
 /* Contradictory independently recognized candidates are rejected. */
 fresh();nativeContext();boundsPresent=1;boundsRect=(Rect){{0,0},{430,345.9}};context.keys=(FVKARect){0,585.3,430,346.7};endRect=(Rect){{0,753.2},{430,178.8}};
 FVKARect keys;int route;assert(!sameEventEvidence(&stateObject,&screenObject,&infoObject,context.source,context.target,core(endRect),&keys,&route));
 puts("PASS actual first-event adapter: 16 cold-Will, same-dispatch, no-stale-height, conflict and fallback scenarios");
}
'''
code=extract('int sameEventEvidence(')+'\n'+base['code']
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'first-event.c';p.write_text(prefix+code+suffix)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I',str(R),str(p),'-lm','-o',str(Path(td)/'test')],check=True)
 subprocess.run([str(Path(td)/'test')],check=True)
