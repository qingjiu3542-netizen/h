from pathlib import Path
import subprocess,tempfile,json,hashlib,time
R=Path(__file__).resolve().parents[1]
results=[]
def run(args,label):
 x=subprocess.run(list(map(str,args)),cwd=R,capture_output=True,text=True)
 print(x.stdout,end='');print(x.stderr,end='')
 results.append({'test':label,'ok':x.returncode==0})
 if x.returncode:raise SystemExit(x.returncode)
with tempfile.TemporaryDirectory() as td:
 for p in sorted(R.glob('test-*.c')):
  exe=Path(td)/p.stem
  run(['clang','-std=c11','-Wall','-Wextra','-Werror',p,'-o',exe],p.name+':compile')
  run([exe],p.name)
 for p in sorted(R.glob('test-*.py')):run(['python3',p],p.name)
 for arch in ('arm64','arm64e'):
  for mode,extra in [('host-route',[]),('host-safearea',['-DFV_SCENE_SAFEAREA_EXPERIMENT=1'])]:
   run(['clang','-target',arch+'-apple-ios14.0','-fsyntax-only','-fexceptions','-Werror=implicit-function-declaration','-DFV_FIRST_MOUNT_MODE=1','-DFV_COMPLETE_MODE=1','-DFV_KEYBOARD_GEOMETRY_PROBE=1','-DFV_KEYBOARD_DOCK_EXPERIMENT=1','-DFV_KEYBOARD_HOST_ROUTE_PROBE=1',*extra,R/'FVIndependent.c'],mode+':'+arch+':syntax')
report={'tested_at_utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'results':results,'count':len(results),'all_pass':all(x['ok'] for x in results),'device_validation':False,'source_sha256':{n:hashlib.sha256((R/n).read_bytes()).hexdigest() for n in ['FVIndependent.c','FVGeometry.inc','FVRestore.inc','FVLifecycle.inc','FVKeyboardHostRouteProbe.inc','FVSceneSafeArea.inc','FVSceneSafeAreaCore.h','ci/apple-build.py']}}
(R/'ci/safearea29-local-tests.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS',len(results),'local checks; no device/Apple SDK inference')
