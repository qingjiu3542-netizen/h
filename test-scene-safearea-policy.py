from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parent
s=(R/'FVSceneSafeArea.inc').read_text();p=(R/'FVIndependent.c').read_text();g=(R/'FVGeometry.inc').read_text()
body=s[s.index('static FVSAState'):]
for forbidden in ('setFrame:', 'setBounds:', 'setCenter:', 'setTransform:', 'appSetting', 'displayConfiguration', 'postNotification', 'updateUISettingsWithBlock:'):
 assert forbidden not in body,forbidden
for gate in ('sourceSceneIdentityString','sourceBundleIdentifier','requiredScene','_activeScreen','interfaceOrientation','isSideCard','innerKeyboardIsDisplay','recordOnlyInnerKeyboardIsShow','fvSAUnique','fvSAConsumed'):
 assert gate in body,gate
assert 'matches==1' in body and 'if(n>32)' in body
assert 'elapsed>=6' in body and 'elapsed>=10' in body and 'fvSATicks>=120' in body
assert 'target.bottom+=12' in body and 'fvSAAttempted=1' in body
assert 'if(restore)return 1;' in body
assert 'same Scene' not in body or 'fvSAScene' in body
for n,needle in [('FVRestore.inc','fvSAStopForHost(h)'),('FVLifecycle.inc','fvSAStopForHost(h)'),('FVKeyboardHostRouteProbe.inc','fvSAEvent(kind,receiver,state)')]:
 assert needle in (R/n).read_text()
assert '"sceneSafeAreaAB29"' in s and 'NSUUID' in p and 'removeObjectForKey:' in p
assert '#ifdef FV_SCENE_SAFEAREA_EXPERIMENT' in p
assert '#include "FVSceneSafeArea.inc"' in g
# A host-only macro without its dependencies MUST NOT compile.
x=subprocess.run(['clang','-fsyntax-only','-DFV_SCENE_SAFEAREA_EXPERIMENT=1',str(R/'FVIndependent.c')],capture_output=True,text=True)
assert x.returncode and 'requires host-only' in x.stderr
print('PASS policy: isolated/default-off request, exact source identity + unique host, no app/frame/cache/display mutation, bounded A/B/A, teardown/fullscreen/rotation/off/hide cleanup wiring')
