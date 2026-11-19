from pathlib import Path
import plistlib
R=Path(__file__).resolve().parent
filter=plistlib.loads((R/'FVKeyboardAppAvoidance.plist').read_bytes())
assert filter=={'Filter':{'Classes':['UIApplication']}},filter
source=(R/'FVKeyboardAppAvoidance.c').read_text()
assert 'com.apple.springboard' in source and 'com.apple.Preferences' in source
assert filter['Filter']['Classes']==['UIApplication']
print('PASS app injection uses UIKit class filter; system processes skipped by runtime guard')
