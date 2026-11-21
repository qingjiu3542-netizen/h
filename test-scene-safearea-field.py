"""Execute actual read/prove functions; never equate an absent object to zero."""
from pathlib import Path
import subprocess,tempfile,os
R=Path(__file__).resolve().parent
s=Path(os.environ.get('FV_SAFEAREA_TEST_SOURCE',R/'FVSceneSafeArea.inc')).read_text()
a=s.index('static int fvSARead(');b=s.index('static int fvSAFocus(',a)
actual=s[a:b]
prefix=r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "FVSceneSafeAreaCore.h"
typedef unsigned long U;typedef const char*SEL;
typedef struct O{int refs,present,abi;FVSAInsets value;struct O *other,*settings,*ui;}O;typedef O*id;
static O SC,BASE,UI,COPY,MAP,CMAP,NEXT;static id fvSAScene=&SC;
static int mainThread,alias,wrongField,wrongGetter,drift,setterCalls,badABI;
#define S(x) (x)
static int fvGMain(void){return mainThread;}
static id objc_getClass(const char*s){(void)s;return &COPY;}
static int fvGKind(id o,id cls){return o==cls;}
static id fvSAObject(id o,const char*s){
 if(!o)return 0;assert(o->refs>0);
 if(!strcmp(s,"settings"))return o->settings;
 if(!strcmp(s,"uiSettings")){if(drift)SC.settings=&NEXT;return o->ui;}
 if(!strcmp(s,"otherSettings"))return o->other;assert(0);return 0;
}
static int fvSAABI(id o,const char*s,const char*t){(void)s;(void)t;return o&&!badABI;}
static id fvSAField(id o){return o&&o->other&&o->other->present?o->other:0;}
static int fvSAUnbox(id o,FVSAInsets*out){if(!o||!o->present||!o->abi)return 0;*out=o->value;return fvSAValid(*out);}
static FVSAInsets fvSAGetInsets(id o,const char*s){(void)s;FVSAInsets v=o->other->value;if(wrongGetter)v.top+=1;return v;}
static id send(id o,const char*s,...){
 if(!o)return 0;assert(o->refs>0);
 if(!strcmp(s,"retain")){o->refs++;return o;}
 if(!strcmp(s,"release")){o->refs--;return 0;}
 if(!strcmp(s,"mutableCopy")){COPY=*o;COPY.refs=1;CMAP=*o->other;COPY.other=alias?o->other:&CMAP;return &COPY;}
 assert(0);return 0;
}
#define SEND(o,s,...) send(o,s,##__VA_ARGS__)
static void mockSetter(id o,SEL sel,FVSAInsets v){assert(!strcmp(sel,"setSafeAreaInsetsPortrait:"));setterCalls++;if(!wrongField)o->other->value=v;}
#define objc_msgSend mockSetter
'''
main=r'''
static void reset(void){
 O*all[]={&SC,&BASE,&UI,&COPY,&MAP,&CMAP,&NEXT};for(unsigned i=0;i<sizeof all/sizeof all[0];i++){memset(all[i],0,sizeof(O));all[i]->refs=all[i]->present=all[i]->abi=1;}
 SC.settings=&BASE;SC.ui=&UI;BASE.other=UI.other=&MAP;COPY.refs=0;
 mainThread=1;alias=wrongField=wrongGetter=drift=setterCalls=badABI=0;
}
static void balanced(void){assert(UI.refs==1&&BASE.refs==1&&COPY.refs==0);}
int main(void){
 FVSASnapshot r;
 reset();assert(fvSARead(0,&r)&&r.present&&r.abi&&r.value.bottom==0);assert(fvSAProveField());assert(setterCalls==1&&MAP.value.bottom==0);balanced();
 reset();MAP.present=0;assert(!fvSARead(0,&r)&&!fvSAProveField()&&!setterCalls&&!MAP.present);balanced();
 reset();MAP.abi=0;assert(!fvSARead(0,&r)&&!fvSAProveField()&&!setterCalls);balanced();
 reset();alias=1;assert(!fvSAProveField()&&!setterCalls&&MAP.value.bottom==0);balanced();
 reset();wrongField=1;assert(!fvSAProveField()&&setterCalls==1&&MAP.value.bottom==0);balanced();
 reset();wrongGetter=1;assert(!fvSARead(0,&r)&&!fvSAProveField());balanced();
 reset();drift=1;assert(!fvSARead(0,&r));balanced();
 reset();badABI=1;assert(!fvSARead(0,&r)&&!fvSAProveField()&&!setterCalls);balanced();
 reset();mainThread=0;assert(!fvSARead(0,&r));balanced();
 puts("PASS real read/prove: explicit zero != unset; wrong mapping/alias/ABI rejected; scratch probe leaves live field untouched; fresh facade/base agreement");return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'field.c';out=Path(td)/'field';p.write_text(prefix+actual+main)
 subprocess.run(['clang','-std=gnu11','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-misleading-indentation','-I',str(R),str(p),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
