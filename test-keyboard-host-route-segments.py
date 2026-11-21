"""Execute real segmentation/identity/budget/record and existing callback wrappers."""
from pathlib import Path
import subprocess, tempfile
R = Path(__file__).resolve().parent
source = (R/'FVKeyboardHostRouteProbe.inc').read_text()
def function(name):
    start = source.index(name); opening = source.index('{', start); depth = 1; end = opening+1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}'); end += 1
    return source[start:end]+'\n'
code = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
typedef void *id;typedef const char *SEL;typedef void (*IMP)(void);typedef unsigned long U;typedef _Bool FVKHBool;
typedef struct{double x,y;}Point;typedef struct{double width,height;}Size;typedef struct{Point origin;Size size;}Rect;
typedef struct{const char *encoding;}MockMethod;typedef MockMethod *Method;
typedef struct{int type,phase,field[4],abi[4],remoteABI;const char *text;}Object;
static Object state={.type=1,.phase=1,.field={1,1,1,1}}, string={.type=2,.text="secret.bundle.scene"}, nonstring={.type=3};
static id fvKHHosts;static volatile unsigned fvKHCurrentMode;
static unsigned fvKHActiveCounts[7][10][2],fvKHOtherCounts[7][10][2],fvKHMissingBudget=5,fvKHSequence;
static unsigned fvKHSession,fvKHSegmentsClosed,fvKHSessionLimitNoted,fvKHDetailTotal;
static int fvKHObservedPhase=-1;static double fvKHStartedAt;static unsigned char fvKHCorrelationSalt;
static _Thread_local unsigned fvKHDepth;static IMP fvKHRemoteChangedOriginal,fvKHShowOriginal;
static int mainThread=1,ratio,getters,forwards,logs,sessionLogs,quotaLogs,hardLogs,remoteReads,encodingLogs,identityLogs;
static char last[1600],lastState[1600],lastIdentity[1600];static Rect remoteRect={{-12.25,93.875},{430.5,378.125}},lastRemote;
static const char *fields[]={"hostBundleIdentifier","sourceBundleIdentifier","sourceDisplayIdentifier","sourceSceneIdentityString"};
static int fvGMain(void){return mainThread;}static int enabled(void){assert(mainThread);return ratio;}
static int getpid(void){return 53506;}static double CFAbsoluteTimeGetCurrent(void){return 800.25+logs*.001;}
static void fvKHLog(const char *s){logs++;snprintf(last,sizeof last,"%s",s);assert(!strstr(s,"secret.bundle.scene"));
 if(strstr(s,"KHSESSION "))sessionLogs++;
 if(strstr(s,"KHQUOTA exhausted"))quotaLogs++;
 if(strstr(s,"scope=show-segments"))hardLogs++;
 if(strstr(s,"KHSTATE "))snprintf(lastState,sizeof lastState,"%s",s);
 if(strstr(s,"KHGETTER "))encodingLogs++;
 if(strstr(s,"KHIDENT ")){identityLogs++;snprintf(lastIdentity,sizeof lastIdentity,"%s",s);}}
#define S(x) (x)
static id object_getClass(id o){assert(mainThread);getters++;return o;}
static const char *class_getName(id cls){(void)cls;assert(mainThread);getters++;return "MockState";}
static id objc_getClass(const char *name){(void)name;assert(mainThread);getters++;return (id)123;}
static int fvGKind(id o,id cls){(void)cls;assert(mainThread);getters++;return o&&((Object*)o)->type==2;}
static Method class_getInstanceMethod(id cls,SEL selector){
 assert(mainThread);getters++;static MockMethod methods[8];if(cls!=&state)return 0;
 for(int i=0;i<4;i++)if(!strcmp(selector,fields[i])){if(state.abi[i]==1)return 0;methods[i].encoding=state.abi[i]==2?"q16@0:8":"@16@0:8";return &methods[i];}
 if(!strcmp(selector,"keyboardPositionWithRemoteIAV")){if(state.remoteABI==1)return 0;methods[4].encoding=state.remoteABI==2?"{CGRect={CGPoint=ff}{CGSize=ff}}16@0:8":"{CGRect={CGPoint=dd}{CGSize=dd}}16@0:8";return &methods[4];}
 return 0;
}
static const char *method_getTypeEncoding(Method m){return m->encoding;}
static id mockSend(id o,SEL s){assert(mainThread);getters++;
 if(o==&state)for(int i=0;i<4;i++)if(!strcmp(s,fields[i]))return state.field[i]==0?0:state.field[i]==2?&nonstring:&string;
 return 0;
}
#define SEND(o,s,...) mockSend(o,s)
static const char *mockMessage(id o,SEL s){assert(mainThread&&o==&string&&!strcmp(s,"UTF8String"));getters++;return string.text;}
static IMP objc_msgSend=(IMP)mockMessage;
static int fvKHBoolProperty(id o,const char *s){assert(mainThread);getters++;return o==&state&&!strcmp(s,"keyboardOnScreen")?state.phase:-1;}
static double fvKHDoubleProperty(id o,const char *s){(void)o;(void)s;assert(mainThread);getters++;return -1;}
static Rect rect(id o,const char *s){assert(mainThread&&o==&state&&!strcmp(s,"keyboardPositionWithRemoteIAV"));getters++;remoteReads++;lastRemote=remoteRect;return remoteRect;}
static unsigned fvKHSceneHash(id o){assert(mainThread);(void)o;getters++;return 0;}
static id ivar(id o,const char *s){(void)o;(void)s;assert(mainThread);getters++;return 0;}
static id fvGTarget(id o){assert(mainThread);return o;}
'''
code += '#include "'+str(R/'FVKeyboardHostRouteCore.h')+'"\n'
for name in ('static unsigned long long fvKHToken(', 'static void fvKHCorrelation(', 'static int fvKHABI(', 'static id fvKHObject(', 'static int fvKHTake(', 'static int fvKHQuota(', 'static void fvKHObservePhase(', 'static void fvKHPhase('):
    code += function(name)
code += 'typedef struct{unsigned hash;int status;} FVKHIdentity;\n'
code += function('static FVKHIdentity fvKHIdentity(')+function('static void fvKHStateIdentity(')
code += 'typedef struct{unsigned previous;}FVKHScope;\n'+function('static void fvKHClose(')
code += function('static void fvKHRecord(')+function('static FVKHBool fvKHRemoteChanged(')+function('static FVKHBool fvKHShow(')
code += r'''
static FVKHBool originalRemote(id self,SEL cmd,id information,FVKHBool a,FVKHBool b){assert(self==(id)1&&cmd==(SEL)2&&information==&state&&a==1&&b==0);forwards++;return 1;}
static FVKHBool originalShow(id self,SEL cmd,id views,id place,id scene){(void)self;(void)cmd;(void)views;(void)place;(void)scene;forwards++;return 1;}
static void changed(void){assert(fvKHRemoteChanged((id)1,(SEL)2,&state,1,0)==1);}
int main(void){
 fvKHRemoteChangedOriginal=(IMP)originalRemote;fvKHShowOriginal=(IMP)originalShow;
 /* Real wrapper/record: all four remote coordinates survive, identity redacted. */
 changed();assert(fvKHSession==1&&sessionLogs==1&&forwards==1&&identityLogs==4);
 assert(remoteReads==1&&!memcmp(&lastRemote,&remoteRect,sizeof remoteRect));
 assert(strstr(lastState,"remoteReadable=1 remote=(-12.250,93.875,430.500,378.125)"));
 assert(strstr(lastIdentity,"field=sourceSceneIdentityString status=value")&&strstr(lastIdentity,"hashValid=1"));
 /* Every already-hit state route with the same value shares a single segment. */
 for(unsigned kind=3;kind<=6;kind++)fvKHRecord(kind,(id)1,0,(Rect){{0,0},{0,0}},(Rect){{0,0},{0,0}},0,0,0,&state,0,0);
 for(int i=0;i<20;i++){ratio=i%2;changed();}assert(fvKHSession==1&&sessionLogs==1);
 /* A prestate-only callback must not manufacture a hide transition. */
 state.phase=0;fvKHPhase(8,&state,0,0);assert(fvKHObservedPhase==1);
 changed();assert(fvKHSession==1&&fvKHObservedPhase==0&&sessionLogs==2);
 state.phase=1;changed();assert(fvKHSession==2&&sessionLogs==3);
 /* Placement and state duplicate observations do not double-open a session. */
 fvKHPhase(0,0,1,1);assert(fvKHSession==2);fvKHPhase(0,0,0,0);assert(fvKHObservedPhase==0);
 assert(fvKHShow((id)1,(SEL)2,0,0,0)==1);assert(fvKHObservedPhase==0);
 state.phase=1;changed();assert(fvKHSession==3);
 /* ABI/missing/nil/non-string cases never invoke a rejected getter. */
 FVKHIdentity identity;int before=getters;mainThread=0;identity=fvKHIdentity(&state,fields[0]);assert(identity.status==0&&getters==before);mainThread=1;
 identity=fvKHIdentity(0,fields[0]);assert(identity.status==0);
 state.abi[0]=1;identity=fvKHIdentity(&state,fields[0]);assert(identity.status==1&&!identity.hash);
 state.abi[0]=2;identity=fvKHIdentity(&state,fields[0]);assert(identity.status==2&&!identity.hash);
 state.abi[0]=0;state.field[0]=0;identity=fvKHIdentity(&state,fields[0]);assert(identity.status==3&&!identity.hash);
 state.field[0]=2;identity=fvKHIdentity(&state,fields[0]);assert(identity.status==4&&!identity.hash);
 state.field[0]=1;string.text=0;identity=fvKHIdentity(&state,fields[0]);assert(identity.status==5&&!identity.hash);string.text="secret.bundle.scene";
 /* FNV-1a hash-zero input is valid, unlike missing/nil. */
 string.text="\xcc\x24\x31\xc4";
 identity=fvKHIdentity(&state,fields[0]);assert(identity.status==6&&identity.hash==0);
 fvKHStateIdentity(901,&state);assert(strstr(lastIdentity,"status=value hash=00000000 hashValid=1"));string.text="secret.bundle.scene";
 int noted=encodingLogs;for(int i=0;i<100;i++)fvKHStateIdentity(900,&state);assert(encodingLogs==noted);
 state.remoteABI=2;int reads=remoteReads;changed();assert(remoteReads==reads&&strstr(lastState,"remoteReadable=0"));
 state.remoteABI=1;changed();assert(remoteReads==reads);state.remoteABI=0;
 /* Main route budget has a fresh, bounded bucket only after a real reopen. */
 unsigned budget=0;int q=quotaLogs;for(int i=0;i<100;i++)fvKHQuota(&budget,2,"test",6,1,fvKHSession);
 assert(budget==3&&quotaLogs==q+1);
 unsigned fixed=0;q=quotaLogs;for(int i=0;i<100;i++)fvKHTake(&fixed,16);assert(fixed==17&&quotaLogs==q+1);
 /* Worker path executes the real record with invalid opaque pointer values.
  * Any getter would assert; wrappers still forward their original arguments. */
 mainThread=0;before=getters;int f=forwards;changed();assert(getters==before&&forwards==f+1);
 q=quotaLogs;for(int i=0;i<100;i++)fvKHRecord(1,(id)0xdead,(id)0xbeef,remoteRect,remoteRect,1,1,1,(id)0xbad,0,0);
 assert(getters==before&&strstr(last,"worker-route")&&quotaLogs==q+1);
 mainThread=1;
 /* Hard process detail cap has a single permanent sentinel and no reset. */
 unsigned detailBefore=fvKHDetailTotal;assert(detailBefore<512);q=quotaLogs;
 for(int i=0;i<1000;i++)fvKHQuota(&fvKHDetailTotal,512,"process-details",6,0,fvKHSession);
 assert(fvKHDetailTotal==513&&quotaLogs==q+1);
 for(int i=0;i<1000;i++){state.phase=0;changed();state.phase=1;changed();}
 assert(fvKHSession==6&&fvKHSegmentsClosed&&hardLogs==1&&fvKHDetailTotal==513);
 assert(fvKHObservedPhase==1);int stopped=logs;for(int i=0;i<1000;i++){state.phase=i%2;changed();}
 assert(logs==stopped&&hardLogs==1&&fvKHSession==6&&fvKHDepth==0);
 puts("PASS actual segment/identity/record/wrappers: route dedup, hide/reopen, mode not phase, hard caps, once exhaustion, worker no getters, ABI/type/nil rejection, remote CGRect");return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
    p=Path(td)/'segments.c';p.write_text(code);exe=Path(td)/'test'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-cast-function-type',str(p),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
# Never add new layout hooks while extending diagnostics.
for forbidden in ('fvKHLayoutViewsImp', 'fvKHPrepareHostedWindowImp', 'fvKHLayoutKeyboardViews(', 'fvKHPrepareHostedWindow('):
    assert forbidden not in source, forbidden
print('PASS source hook boundary: unfinished layout/hosted-window drafts removed; no new layout hooks')
