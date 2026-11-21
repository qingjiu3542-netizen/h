"""Fetch ONLY the successful Apple run matching the explicitly supplied commit.
Token stays in the environment/config stdin; signed redirects receive NO auth.
"""
from pathlib import Path
import os,sys,json,subprocess,tempfile,time,hashlib,zipfile,io
R=Path(__file__).resolve().parents[1]
commit=sys.argv[1];assert len(commit)==40 and all(x in '0123456789abcdef' for x in commit)
API='https://api.github.com/repos/qingjiu3542-netizen/h'
def api(path,redirect=False):
 with tempfile.TemporaryDirectory() as td:
  h=Path(td)/'headers'
  cfg='header = "Authorization: Bearer '+os.environ['GH_TOKEN']+'"\n'
  x=subprocess.run(['curl','-sS','--fail','--connect-timeout','8','--max-time','60','--resolve','api.github.com:443:20.205.243.168','-A','Minis-safearea29','-H','Accept: application/vnd.github+json','-D',str(h),'-K','-',API+path],input=cfg.encode(),capture_output=True)
  if x.returncode:raise RuntimeError('GitHub API request failed (no credential details emitted)')
  if redirect:
   headers=h.read_text();locations=[l.split(':',1)[1].strip() for l in headers.splitlines() if l.lower().startswith('location:')]
   assert len(locations)==1;return locations[0]
  return json.loads(x.stdout)
runs=api('/actions/runs?head_sha='+commit+'&per_page=20')['workflow_runs']
runs=[r for r in runs if r['head_sha']==commit and 'Apple toolchain' in r['name']]
assert runs,'No matching Apple CI run found'
run=max(runs,key=lambda r:r['id'])
print(json.dumps({k:run.get(k) for k in ('id','run_number','head_sha','status','conclusion','html_url')}),flush=True)
if run['status']!='completed':sys.exit(3)
assert run['conclusion']=='success','Apple CI did not succeed'
artifacts=api('/actions/runs/'+str(run['id'])+'/artifacts')['artifacts']
artifacts=[a for a in artifacts if not a['expired'] and a['name']=='floatingview-apple-build-'+str(run['run_number'])]
assert len(artifacts)==1;a=artifacts[0]
signed=api('/actions/artifacts/'+str(a['id'])+'/zip?nonce='+str(time.time_ns()),True)
x=subprocess.run(['curl','-fsS','--connect-timeout','10','--max-time','240','--retry','2','-L','-A','Minis-safearea29',signed],capture_output=True)
if x.returncode:raise RuntimeError('Signed artifact download failed (URL intentionally not logged)')
data=x.stdout;digest=hashlib.sha256(data).hexdigest()
assert a.get('digest')=='sha256:'+digest,'Artifact digest mismatch'
archive=R.parent/('floatingview-safearea29-apple-'+str(run['run_number'])+'.zip');archive.write_bytes(data)
z=zipfile.ZipFile(io.BytesIO(data));assert z.testzip() is None
assert len(z.namelist())==len(set(z.namelist()))
for n in z.namelist():assert not n.startswith('/') and '..' not in Path(n).parts
out=R/'build-safearea29';out.mkdir(exist_ok=True);z.extractall(out)
status={'run_id':run['id'],'build':run['run_number'],'head_sha':commit,'conclusion':run['conclusion'],'artifact_id':a['id'],'sha256':digest,'archive':str(archive),'bytes':len(data)}
(R/'ci/safearea29-run.json').write_text(json.dumps(status,indent=2)+'\n')
print(json.dumps(status,indent=2));print('PASS exact commit, success, fresh nonce, unsigned redirect request, archive digest')
