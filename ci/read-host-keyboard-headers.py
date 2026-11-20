# Local-only research helper; never alters a device or the public repository.
import subprocess,json,base64
from pathlib import Path
R=Path('/tmp/fv-host-keyboard-headers');R.mkdir(exist_ok=True)
paths=[('UIKitCore','_UIRemoteKeyboards'),('UIKitCore','_UIKeyboardChangedInformation'),('UIKitCore','UIKeyboardSceneDelegate'),('UIKitCore','UIApplicationSceneSettings'),('UIKitCore','UIMutableApplicationSceneSettings'),('UIKitCore','UIInputViewSet'),('UIKitCore','_UIRemoteKeyboardController'),('SpringBoard','SBKeyboardFocusCoordinator')]
for framework,name in paths:
 p=R/(name+'.h')
 if p.exists():s=p.read_text()
 else:
  url='https://api.github.com/repos/nst/iOS-Runtime-Headers/contents/PrivateFrameworks/'+framework+'.framework/'+name+'.h'
  cfg='url = '+json.dumps(url)+'\n'
  response=subprocess.run(['curl','-fsS','--resolve','api.github.com:443:20.205.243.168','--connect-timeout','5','--max-time','14','-K','-'],input=cfg,text=True,capture_output=True)
  try:s=base64.b64decode(json.loads(response.stdout)['content']).decode()
  except Exception:print(name+': unavailable');continue
  p.write_text(s)
 print('\n'+name+' ('+str(len(s))+' bytes)')
 for line in s.splitlines():
  if any(term.lower() in line.lower() for term in ('keyboard','occlu','reference','display','screen','frame','coordinate','placement')):print(line)
