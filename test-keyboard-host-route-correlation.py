"""Actual correlation helpers: never dereference receiver/state or infer ownership."""
from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parent
source=(R/'FVKeyboardHostRouteProbe.inc').read_text()
def extract(name):
 start=source.index(name);brace=source.index('{',start);depth=1;end=brace+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
code=r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef void *id;typedef unsigned long U;
'''
code+='#include "'+str(R/'FVKeyboardHostRouteCore.h')+'"\n'
code+=r'''
static unsigned char fvKHCorrelationSalt;
static char last[800];static int logCount;
static int getpid(void){return 1234;}
static void fvKHLog(const char *s){assert(strlen(s)<sizeof last);strcpy(last,s);logCount++;}
'''
code+=extract('static unsigned long long fvKHToken(')+'\n'+extract('static void fvKHCorrelation(')
code+=r'''
int main(void){
 /* Non-dereferenceable values intentionally supplied. */
 id receiver=(id)(U)0x101,state=(id)(U)0x203,context=(id)(U)0x305;
 assert(!fvKHToken(0));assert(fvKHToken(state)==fvKHToken(state));
 assert(fvKHToken(state)!=fvKHToken(receiver));
 assert(fvKHObjectToken(0x203,1)!=fvKHObjectToken(0x203,2));
 fvKHCorrelation(10,4,0,1,receiver,0,state);
 assert(strstr(last,"pid=1234")&&strstr(last,"main=0")&&strstr(last,"stateRole=original-state-argument"));
 char expected[80];snprintf(expected,sizeof expected,"stateToken=%016llx",fvKHToken(state));assert(strstr(last,expected));
 fvKHCorrelation(11,6,1,1,receiver,context,state);assert(strstr(last,expected));
 fvKHCorrelation(12,8,0,1,receiver,0,0);assert(strstr(last,"stateRole=not-sampled-off-main")&&strstr(last,"stateToken=0000000000000000"));
 fvKHCorrelation(13,8,1,1,receiver,context,state);assert(strstr(last,"stateRole=receiver-current-prestate"));
 assert(strstr(last,"not-target-authorization")&&strstr(last,"pointer-reuse-possible"));
 assert(logCount==4);return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'correlation.c';p.write_text(code);exe=Path(td)/'test'
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(p),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
record=extract('static void fvKHRecord(')
start=record.index('if(!isMain){');end=record.index('int empty=',start);worker=record[start:end]
assert 'fvKHCorrelation(seq,kind,0,mode,receiver,eventScene,state)' in worker
for forbidden in ('fvKHObject(', 'fvKHSceneHash(', 'fvKHABI(', 'rect(', 'SEND(', 'fvKHBoolProperty(', 'object_getClass('):
 assert forbidden not in worker,forbidden
assert 'return;' in worker
print('PASS actual correlation: nil/stable/distinct values, same state across routes, pid, snapshot roles and no object dereference')
print('PASS worker record: argument-only correlation before return; no object getter or host enumeration')
