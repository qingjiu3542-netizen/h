from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parent
source=(R/'FVKeyboardAppAvoidance.c').read_text()
start=source.index(' int fd=open(file,0|0x00000100|0x01000000|0x00000004);')
end=source.index(' unsigned char bytes[8192];',start)
fragment=source[start:end]
prefix=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef void *id;typedef void *SEL;
static int errorCode,calls,proxyCalls,homeFD,homeError,otherFD,otherError,proxyMode;
static int *__error(void){return &errorCode;}
static int fakeOpen(const char *file,int flags){
 (void)flags;calls++;
 if(!strcmp(file,"home")){errorCode=homeError;return homeFD;}
 assert(!strcmp(file,"other"));errorCode=otherError;return otherFD;
}
#define open fakeOpen
#define S(x) ((SEL)(x))
static const char *objc_msgSend(id object,SEL cmd){(void)cmd;return object;}
static int equal(id a,id b){return !strcmp(a,b);}
static id appProxyCachePath(id bundle){(void)bundle;proxyCalls++;errorCode=13;return proxyMode==0?0:(proxyMode==1?"other":"home");}
static void reportOnce(unsigned bit,const char *message){(void)bit;(void)message;errorCode=2;}
static id testOpen(int *missing,const char **reason){
 const char *file="home";id path="home",appBundle="test";*missing=0;
'''
suffix=r'''
 return (id)(long)(fd+1);
}
static void reset(void){errorCode=calls=proxyCalls=0;homeFD=-1;homeError=2;otherFD=-1;otherError=13;proxyMode=1;}
int main(void){int missing=0;const char *reason=0;
 reset();assert(!testOpen(&missing,&reason));assert(!missing&&calls==2&&proxyCalls==1);
 reset();otherError=2;assert(!testOpen(&missing,&reason));assert(missing&&calls==2);
 reset();homeError=13;assert(!testOpen(&missing,&reason));assert(!missing&&calls==1&&proxyCalls==0);
 reset();homeFD=3;assert(testOpen(&missing,&reason)==(id)4);assert(!missing&&calls==1&&proxyCalls==0);
 reset();otherFD=6;assert(testOpen(&missing,&reason)==(id)7);assert(!missing&&calls==2);
 reset();proxyMode=0;assert(!testOpen(&missing,&reason));assert(missing&&calls==1);
 reset();proxyMode=2;assert(!testOpen(&missing,&reason));assert(missing&&calls==1);
 puts("PASS actual ticket-open control flow: 7 missing/denied/canonical-path and errno-clobber cases");
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'open-test.c';p.write_text(prefix+fragment+suffix)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(p),'-o',str(Path(td)/'test')],check=True)
 subprocess.run([str(Path(td)/'test')],check=True)
