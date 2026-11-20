"""Read-only host-route log analysis. Never equate local state with app delivery."""
import re
from collections import Counter
INSTALL='KHROUTE abi-fix-1 installedMask='
CONSTRUCTOR='FVAspect8 constructor reached'
LEGACY=('KB app bridge:', 'KB app status v0.1.20')
def analyze(text):
 lines=text.splitlines()
 installations=[i for i,line in enumerate(lines) if INSTALL in line]
 if not installations:
  return {'has_installation':False,'functional_fix_proven':False,'reason':'no ABI-fix installation summary'}
 install=installations[-1]
 constructors=[i for i,line in enumerate(lines[:install+1]) if CONSTRUCTOR in line]
 start=constructors[-1] if constructors else install
 session=lines[start:]
 legacy=[start+i+1 for i,line in enumerate(session) if any(x in line for x in LEGACY)]
 state={};route={};host={};counts=Counter();worker=Counter()
 for line in session:
  m=re.search(r'\bseq=(\d+)',line)
  if not m:continue
  seq=int(m[1])
  if line.startswith('KHROUTE ') and 'route=' in line:
   r=re.search(r'route=(\S+)',line);mode=re.search(r'mode=(\S+)',line)
   if r:counts[r[1]]+=1;route[seq]={'route':r[1],'mode':mode[1] if mode else 'unknown'}
  elif line.startswith('KHSTATE '):
   r=re.search(r'\biav=\(([^)]*)\)',line)
   if r:state[seq]=r[1]
  elif line.startswith('KHHOST seq='):
   bundle=re.search(r'bundle=(\S+)',line);settings=re.search(r'settings=\(([^)]*)\)',line)
   scene=re.search(r'hostScene=(\S+)',line)
   if bundle and settings:host.setdefault(seq,[]).append({'bundle':bundle[1],'settings':settings[1],'scene':scene[1] if scene else None})
  elif line.startswith('KHWORKER '):
   r=re.search(r'route=(\S+)',line)
   if r:worker[r[1]]+=1
 samples=[]
 for seq,rect in state.items():
  for h in host.get(seq,[]):samples.append(dict(h,seq=seq,state=rect,**route.get(seq,{})))
 return {'has_installation':True,'session_start_line':start+1,'installation_line':install+1,
  'session_line_count':len(session),'legacy_marker_lines_in_session':legacy,
  'legacy_markers_absent_in_session':not legacy,'routes':dict(counts),'worker_routes':dict(worker),
  'state_host_samples':samples,'has_correlation_records':any(l.startswith('KHCORR ') for l in session),
  'functional_fix_proven':False,
  'evidence_limit':'Observed receiver/current state is not demonstrated target-app delivery; absent legacy markers are not a package/process inventory.'}
if __name__=='__main__':
 import argparse,json
 from pathlib import Path
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path);args=p.parse_args()
 print(json.dumps(analyze(args.log.read_text(errors='replace')),indent=2,ensure_ascii=False))
