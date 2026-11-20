from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parent
source=(R/'FVKeyboardHostRouteProbe.inc').read_text()
for token in ('KHROUTE abi-fix-1','inputSetEmpty=%d','placeShowsKeyboard=%d','remoteFull=','KHREMOTE','keyboardScreenMatchesWindow','inputDestinationDidChange:withIAVHeight:isIAVRelevant:notifyRemote:forScene:','_didHandleKeyboardClientChange:shouldConsiderSnapshottingKeyboard:isLocalEvent:','queue_keyboardChangedWithCompletion:','inputDestinationDidChange:withIAVHeight:isIAVRelevant:notifyRemote:forScene:','_UIKeyboardArbiterClientSpringBoard','_UIKeyboardArbiterClientInputUIHost'):
 assert token in source,token
for forbidden in ('setCurrent','setFrame:','setBounds:','setPlacement:','setRequiredScene:','postNotification','object_setIvar','dispatch_after','dispatch_async','setObject:forKey:','updateSettings:'):
 assert forbidden not in source,forbidden
assert 'fvKHEncoding(actual,expected)' in source
assert 'method_getNumberOfArguments(method)!=expectedArgs' in source
assert '"B@:@BB"' in source
assert '"didHandleKeyboardChange:shouldConsiderSnapshottingKeyboard:isLocalEvent:"' in source
assert '"keyboardChanged:shouldConsiderSnapshottingKeyboard:isLocalEvent:"' not in source
assert 'class_copyMethodList' in source
subprocess.run(['python3',str(R/'test-keyboard-host-route-abi.py')],check=True)
subprocess.run(['python3',str(R/'test-keyboard-host-route-iav.py')],check=True)
assert 'weakObjectsHashTable' in source
# Compile the shared strict ABI core under warnings-as-errors.
with tempfile.TemporaryDirectory() as td:
 out=Path(td)/'test'
 subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',str(R/'test-keyboard-host-route.c'),'-o',str(out)],check=True)
 subprocess.run([str(out)],check=True)
print('PASS host-route policy: ABI-gated show/prepare/arbiter/client/IAV observation, no delayed work or geometry/state writes')
