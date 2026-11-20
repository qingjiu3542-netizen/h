from pathlib import Path
import plistlib
R=Path(__file__).resolve().parent
bridge=(R/'FVKeyboardAppBridge.inc').read_text()
app=(R/'FVKeyboardAppAvoidance.c').read_text()
fil=plistlib.loads((R/'FVKeyboardAppAvoidance.plist').read_bytes())
assert fil=={'Filter':{'Classes':['UIApplication']}}
for token in ('currentActivityApp','bundleIdentifier','LSApplicationProxy','applicationProxyForIdentifier:','applicationIdentifier','dataContainerURL','Library','Caches','bundleID','"schema",2','openat','renameat','O_NOFOLLOW','dataWithPropertyList:format:options:error:'):
 assert token in bridge,token
assert 'writeToFile:atomically:' not in bridge
for token in ('NSHomeDirectory()','Library','Caches','"bundleID"','mainBundleID()','number(schema)!=2','isEqualToString:'):
 assert token in app,token
assert '/var/mobile/Library/Preferences/com.minis.floatingview.keyboard-layout.plist' not in bridge
assert '/var/mobile/Library/Preferences/com.minis.floatingview.keyboard-layout.plist' not in app
assert 'dictionaryWithContentsOfFile:',app
print('PASS per-app ticket: LaunchServices container writer and matching app-cache reader, schema 2')
