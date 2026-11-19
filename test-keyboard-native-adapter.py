from pathlib import Path
import platform,subprocess,tempfile
R=Path(__file__).resolve().parent
source=(R/'FVKeyboardDock.inc').read_text()
a=source.index('static int fvKDFlag(')
b=source.index('\nstatic int fvKDCurrent',a)
adapter=source[a:b]
prefix=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Darwin stdio fortify defines a function-like macro; the production
 * adapter declares the real symbol without stdio headers. Keep that ABI. */
#ifdef snprintf
#undef snprintf
#endif
#include "FVKeyboardNativeFlagCore.h"
typedef void *id;typedef void *Class;typedef unsigned long U;
static unsigned char object[320];
static int classToken,superToken,otherToken;static int exact=1;
static Class fvGItemClass=&classToken;
typedef struct{long offset;const char *encoding;} Ivar;
static Ivar fields[3]={{120,""},{152,""},{153,""}};
static const char *names[]={"isSideCard","innerKeyboardIsDisplay","recordOnlyInnerKeyboardIsShow"};
Class object_getClass(id item){return item?(exact?&classToken:&otherToken):0;}
Class class_getSuperclass(Class cls){return cls==&classToken?&superToken:0;}
U class_getInstanceSize(Class cls){return cls==&superToken?64:320;}
void *class_getInstanceVariable(Class cls,const char *name){
 if(cls!=&classToken)return 0;
 for(int i=0;i<3;i++)if(!strcmp(name,names[i]))return &fields[i];
 return 0;
}
const char *ivar_getTypeEncoding(void *v){return ((Ivar*)v)->encoding;}
long ivar_getOffset(void *v){return ((Ivar*)v)->offset;}
static void trace(const char *s){(void)s;}
'''
suffix=r'''
int main(void){
 object[152]=0;object[153]=1;
 assert(fvKDFlag(object,"isSideCard")==0);
 assert(fvKDFlag(object,"innerKeyboardIsDisplay")==0);
 assert(fvKDFlag(object,"recordOnlyInnerKeyboardIsShow")==1);
 assert(fvKBPortraitInnerEligible(fvKDFlag(object,names[0]),fvKDFlag(object,names[1]),fvKDFlag(object,names[2]),1));
 object[153]=0;assert(!fvKBPortraitInnerEligible(fvKDFlag(object,names[0]),fvKDFlag(object,names[1]),fvKDFlag(object,names[2]),1));
 exact=0;assert(fvKDFlag(object,names[0])==-1);exact=1;
 fields[1].encoding="q";assert(fvKDFlag(object,names[0])==-1);fields[1].encoding="";
 fields[2].offset=154;assert(fvKDFlag(object,names[1])==-1);fields[2].offset=153;
 object[153]=2;assert(fvKDFlag(object,names[1])==-1);object[153]=1;
 assert(fvKDFlag(object,"unknown")==-1);
 puts("PASS actual native flag adapter: empty Swift encodings accepted, portrait record selected, invalid layouts rejected");
}
'''
with tempfile.TemporaryDirectory() as directory:
 p=Path(directory)/'adapter.c';p.write_text(prefix+adapter+suffix)
 exe=Path(directory)/'adapter'
 cc=['xcrun','clang'] if platform.system()=='Darwin' else ['cc']
 subprocess.run(cc+['-std=c11','-Wall','-Wextra','-Werror','-I',str(R),str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
