from pathlib import Path
import subprocess, struct, json, hashlib
R=Path(__file__).resolve().parents[1]; B=R/'out'; B.mkdir(exist_ok=True)
def run(*args):
 print('+',*map(str,args),flush=True)
 subprocess.run(list(map(str,args)),cwd=R,check=True)
def capture(*args):
 return subprocess.check_output(list(map(str,args)),cwd=R,text=True).strip()
sdk=capture('xcrun','--sdk','iphoneos','--show-sdk-path')
(B/'toolchain.txt').write_text(capture('xcodebuild','-version')+'\n'+capture('xcrun','clang','--version')+'\nSDK: '+sdk+'\n')
for test in ('test-first-mount','test-first-presentation','test-restore'):
 exe=B/test
 run('xcrun','clang','-std=c11','-Wall','-Wextra','-Werror',R/(test+'.c'),'-o',exe)
 run(exe)
summary=[]
for mode in ('baseline','probe'):
 for arch in ('arm64','arm64e'):
  out=B/(mode+'-'+arch+'.dylib')
  flags=['-DFV_FIRST_PRESENTATION_PROBE=1'] if mode=='probe' else ['-DFV_DIRECT_ENTRY=1']
  run('xcrun','--sdk','iphoneos','clang','-arch',arch,'-isysroot',sdk,
      '-miphoneos-version-min=14.0','-dynamiclib','-fPIC','-O2','-g',
      '-Werror=implicit-function-declaration',*flags,R/'FVIndependent.c',
      '-framework','Foundation','-lobjc','-Wl,-undefined,error',
      '-Wl,-install_name,@rpath/FVIndependent-'+mode+'.dylib','-o',out)
  data=out.read_bytes();magic,cpu,sub=struct.unpack_from('<III',data)
  assert magic==0xfeedfacf and cpu==0x100000c
  assert sub&0xffffff==(2 if arch=='arm64e' else 0),(arch,hex(sub))
  # No header edits, cross-architecture fallback, or dynamic-lookup imports.
  report=capture('xcrun','otool','-hv',out)+'\n'+capture('xcrun','otool','-L',out)+'\n'+capture('xcrun','otool','-l',out)
  (B/(mode+'-'+arch+'-macho.txt')).write_text(report)
  symbols=capture('xcrun','nm','-m','-u',out)
  assert 'dynamically looked up' not in symbols,symbols
  (B/(mode+'-'+arch+'-imports.txt')).write_text(symbols+'\n')
  run('codesign','--force','--sign','-','--timestamp=none',out)
  run('codesign','--verify','--strict','--verbose=2',out)
  summary.append({'mode':mode,'arch':arch,'subtype':hex(sub),'sha256':hashlib.sha256(out.read_bytes()).hexdigest()})
 run('xcrun','lipo','-create',B/(mode+'-arm64.dylib'),B/(mode+'-arm64e.dylib'),'-output',B/('FVIndependent-'+mode+'.dylib'))
 run('xcrun','lipo','-info',B/('FVIndependent-'+mode+'.dylib'))
(B/'build-results.json').write_text(json.dumps(summary,indent=2)+'\n')
(B/'NOT-FOR-INSTALLATION.txt').write_text('Apple toolchain validation artifacts only. No deb or runtime repair claim. First-frame client relayout, landscape integration, keyboard avoidance and device loading remain unverified.\n')
print('PASS Apple build and CPU subtype checks. No deb generated.',flush=True)
