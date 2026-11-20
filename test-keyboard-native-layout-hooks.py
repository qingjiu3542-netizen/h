from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parent
source=(R/'FVKeyboardAppNativeLayout.inc').read_text()
def function(name):
 start=source.index('static double '+name+'(');brace=source.index('{',start);depth=1;i=brace+1
 while depth:
  depth+=(source[i]=='{')-(source[i]=='}');i+=1
 return source[start:i]
prefix=r'''
#include <assert.h>
#include <stdio.h>
typedef void *id;typedef void *SEL;typedef signed char BOOL;typedef void(*IMP)(void);
static IMP oldIntersectionShort,oldIntersectionLong;
static _Thread_local unsigned nativeLayoutDepth,nativeLocalMinimumSeen;
static int shortCalls,longCalls,resultCalls,nested,setMinimum,checkReentrant;
static id expectedRemote=(id)1,expectedScene=(id)2;static SEL shortCmd=(SEL)3,longCmd=(SEL)4;
static BOOL *expectedOut,expectedIgnore;
static double intersectionLongHook(id,SEL,id,BOOL*,BOOL);
static double nativeLayoutResult(id remote,id scene,double value,int localMinimum){
 assert(remote==expectedRemote&&scene==expectedScene);resultCalls++;
 if(checkReentrant){BOOL flag=0;expectedOut=&flag;expectedIgnore=1;
  assert(intersectionLongHook(remote,longCmd,scene,&flag,1)==210.0);}
 return localMinimum?value:value+1;
}
static double originalLong(id remote,SEL cmd,id scene,BOOL *out,BOOL ignore){
 assert(remote==expectedRemote&&scene==expectedScene&&cmd==longCmd);
 assert(out==expectedOut&&ignore==expectedIgnore);longCalls++;
 if(out)*out=setMinimum;
 return 210.0;
}
static double originalShort(id remote,SEL cmd,id scene){
 assert(remote==expectedRemote&&scene==expectedScene&&cmd==shortCmd);shortCalls++;
 if(nested){BOOL flag=0;expectedOut=&flag;expectedIgnore=1;return intersectionLongHook(remote,longCmd,scene,&flag,1);}
 return 210.0;
}
static void reset(void){shortCalls=longCalls=resultCalls=nested=setMinimum=checkReentrant=0;expectedOut=0;expectedIgnore=0;assert(nativeLayoutDepth==0);}
'''
suffix=r'''
int main(void){
 oldIntersectionShort=(IMP)originalShort;oldIntersectionLong=(IMP)originalLong;
 reset();assert(intersectionShortHook(expectedRemote,shortCmd,expectedScene)==211);assert(shortCalls==1&&resultCalls==1&&longCalls==0);
 reset();assert(intersectionLongHook(expectedRemote,longCmd,expectedScene,0,0)==211);assert(longCalls==1&&resultCalls==1);
 reset();BOOL flag=0;expectedOut=&flag;expectedIgnore=1;assert(intersectionLongHook(expectedRemote,longCmd,expectedScene,&flag,1)==211);assert(flag==0&&longCalls==1&&resultCalls==1);
 reset();setMinimum=1;expectedOut=&flag;assert(intersectionLongHook(expectedRemote,longCmd,expectedScene,&flag,0)==210);assert(flag==1&&longCalls==1&&resultCalls==1);
 reset();nested=1;assert(intersectionShortHook(expectedRemote,shortCmd,expectedScene)==211);assert(shortCalls==1&&longCalls==1&&resultCalls==1);
 reset();nested=setMinimum=1;assert(intersectionShortHook(expectedRemote,shortCmd,expectedScene)==210);assert(shortCalls==1&&longCalls==1&&resultCalls==1);
 reset();checkReentrant=1;assert(intersectionShortHook(expectedRemote,shortCmd,expectedScene)==211);assert(shortCalls==1&&longCalls==1&&resultCalls==1);
 assert(nativeLayoutDepth==0);
 puts("PASS actual native layout hooks: 7 single-forward, pointer/flag preservation and nested-query cases");
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'native-hooks.c';p.write_text(prefix+'\n'+function('intersectionShortHook')+'\n'+function('intersectionLongHook')+'\n'+suffix)
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(p),'-o',str(Path(td)/'test')],check=True)
 subprocess.run([str(Path(td)/'test')],check=True)
assert 'keyboardChanged:' not in source and 'updateKeyboardSize:' not in source
assert 'if(!state)NATIVEFAIL' in source
assert 'if(!localTarget(state,screen,source,target,reason))return 0;' in source
assert 'nativeMethodABI(remote,"_activeScreen","@@:")' in source
assert 'nativeMethodABI(native,"keyboardPositionWithIAV"' in source
app=(R/'FVKeyboardAppAvoidance.c').read_text()
assert 'if(requestedScene&&requestedScene!=chosen)' in app
assert 'if(missing)*missing=(e==2)' in app
assert 'if(!missing){reportOnce(4' in app
assert 'applicationProxyForIdentifier:' in app and 'applicationIdentifier' in app
print('PASS native evidence gates: explicit Scene, runtime ABI, ENOENT-only fallback and same-bundle lookup')
