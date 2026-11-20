from pathlib import Path
import plistlib
R=Path(__file__).resolve().parent
app=(R/'FVKeyboardAppAvoidance.c').read_text()
writer=(R/'FVKeyboardAppStatus.inc').read_text()
host=(R/'FVKeyboardAppStatusHost.inc').read_text()
bridge=(R/'FVKeyboardAppBridge.inc').read_text()
assert plistlib.loads((R/'FVKeyboardAppAvoidance.plist').read_bytes())=={'Filter':{'Classes':['UIApplication']}}
assert 'syslog(5,"[FVKeyboardApp v0.1.20 pid=%d] library constructor reached' in app
assert 'NSTemporaryDirectory()' in app
assert 'Caches log open failed errno=%d tmpOpened=%d' in app
for stage in (1,2,3,4):assert 'writeAppStatus('+str(stage)+')' in app
assert 'if(stage<appStatusStage)return' in writer
assert 'keyboard-layout.plist' not in writer+host
assert 'com.minis.floatingview.keyboard-status.plist' in writer and 'com.minis.floatingview.keyboard-status.plist' in host
assert 'fvKAStatusValid' in host and 'isEqualToString:' in host
assert 'O_NOFOLLOW' in host and 'length<sizeof bytes' in host
assert 'not proof of no injection' in host
assert 'fvKABReadAppStatus(host,cacheFD,bundle)' in bridge
assert bridge.index('fvKABReadAppStatus(host,cacheFD,bundle)')<bridge.index('BOOL ok=fvKABAtomicWrite(cacheFD,packet)')
for file in ('FVKeyboardAppFirstEvent.inc','FVKeyboardAppMeasuredCore.h','FVKeyboardAppFrameCore.h','FVKeyboardAppNativeLayout.inc'):
 assert 'keyboard-status.plist' not in (R/file).read_text()
assert 'com.tencent.mqq' not in app+host+writer and 'com.tencent.xin' not in app+host+writer
print('PASS diagnostic policy: generic injection, startup markers, nofollow bounded receipt, strict validation and no geometry authorization')
