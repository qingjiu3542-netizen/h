from pathlib import Path
R=Path(__file__).resolve().parent
bridge=(R/'FVKeyboardAppBridge.inc').read_text()
start=bridge.index('static void fvKDClearHostTicket(')
end=bridge.index('\n}\n#endif',start)+2
clear=bridge[start:end]
for key in ('path','scene','bundleID','stamp'):
 assert 'fvGPut(work,"'+key+'",' in clear,key
 assert clear.index('fvGPut(work,"'+key+'"')<clear.index('objc_setAssociatedObject(host,&fvKABSceneKey,0,1)'),key
assert 'dispatch_async_f(&_dispatch_main_q,work,fvKABRevokeDeferred)' in clear
assert 'fvKABOpenCaches(bundle' not in clear
geometry=(R/'FVGeometry.inc').read_text()
branch=geometry[geometry.index(' if(coverage){'):geometry.index(' if(state&&',geometry.index(' if(coverage){'))]
assert 'fvKDDropForHost(h);' in branch
assert 'fvKDClearHostTicket' not in branch
print('PASS fullscreen avoids synchronous ticket I/O; revocation retains ownership before clearing associations')
