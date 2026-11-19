from pathlib import Path
import platform,subprocess,tempfile
R=Path(__file__).resolve().parent
s=(R/'FVKeyboardAppAvoidance.c').read_text()
a=s.index('static void namedHook(');b=s.index('static int hook(',a)
functions=s[a:b]
prefix=r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef void *id;typedef const char *SEL;typedef void(*IMP)(void);
static int cls,center,custom,name,object,originalInfo,changedInfo,note,replacement;
static unsigned forwarding;
static IMP oldNamed,oldNote;
static int maps,releases,namedCalls,noteCalls,canMap,nest;
static id delivered,deliveredNote;
static void noteHook(id,SEL,id);
static id objc_getClass(const char *s){(void)s;return &cls;}
static id m0(id o,const char *s){
 if(!strcmp(s,"defaultCenter"))return &center;
 if(!strcmp(s,"release")){assert(o==&changedInfo);releases++;return 0;}
 if(!strcmp(s,"name"))return &name;
 if(!strcmp(s,"object"))return &object;
 if(!strcmp(s,"userInfo"))return o==&replacement?&changedInfo:&originalInfo;
 assert(0);return 0;
}
static id m3(id c,const char*s,id n,id o,id i){
 (void)c;assert(!strcmp(s,"notificationWithName:object:userInfo:"));
 assert(n==&name&&o==&object&&i==&changedInfo);return &replacement;
}
#define M0(o,s) m0(o,s)
#define M3(c,s,n,o,i) m3(c,s,n,o,i)
static id mappedInfo(id n,id o,id i){
 assert(n==&name&&o==&object&&i==&originalInfo);maps++;
 return canMap?&changedInfo:0;
}
static void nativeNamed(id c,SEL cmd,id n,id o,id i){
 (void)cmd;assert(n==&name&&o==&object);namedCalls++;delivered=i;
 if(nest)noteHook(c,"postNotification:",i==&changedInfo?&replacement:&note);
}
static void nativeNote(id c,SEL cmd,id n){(void)c;(void)cmd;noteCalls++;deliveredNote=n;}
static void reset(void){forwarding=0;maps=releases=namedCalls=noteCalls=canMap=nest=0;delivered=deliveredNote=0;}
'''
suffix=r'''
int main(void){
 oldNamed=(IMP)nativeNamed;oldNote=(IMP)nativeNote;
 reset();canMap=1;namedHook(&center,"postNotificationName:object:userInfo:",&name,&object,&originalInfo);
 assert(namedCalls==1&&maps==1&&releases==1&&delivered==&changedInfo&&forwarding==0);
 reset();namedHook(&center,"postNotificationName:object:userInfo:",&name,&object,&originalInfo);
 assert(namedCalls==1&&maps==1&&!releases&&delivered==&originalInfo);
 reset();canMap=1;namedHook(&custom,"postNotificationName:object:userInfo:",&name,&object,&originalInfo);
 assert(namedCalls==1&&!maps&&!releases&&delivered==&originalInfo);
 reset();canMap=1;nest=1;namedHook(&center,"postNotificationName:object:userInfo:",&name,&object,&originalInfo);
 assert(namedCalls==1&&noteCalls==1&&maps==1&&releases==1&&deliveredNote==&replacement&&forwarding==0);
 reset();canMap=1;noteHook(&center,"postNotification:",&note);
 assert(noteCalls==1&&maps==1&&releases==1&&deliveredNote==&replacement);
 reset();noteHook(&center,"postNotification:",&note);
 assert(noteCalls==1&&maps==1&&!releases&&deliveredNote==&note);
 reset();canMap=1;noteHook(&custom,"postNotification:",&note);
 assert(noteCalls==1&&!maps&&!releases&&deliveredNote==&note);
 reset();forwarding=1;canMap=1;noteHook(&center,"postNotification:",&note);
 assert(noteCalls==1&&!maps&&forwarding==1);
 puts("PASS actual app notification hooks: 8 forwarding, nesting and unchanged-event cases");
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'forward.c';p.write_text(prefix+functions+suffix)
 exe=Path(td)/'forward';cc=['xcrun','clang'] if platform.system()=='Darwin' else ['cc']
 subprocess.run(cc+['-std=c11','-Wall','-Wextra','-Werror',str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
