from host_route_log import analyze
prefix='''FVAspect8 constructor reached (before Foundation)
KHROUTE abi-fix-1 installedMask=511
KB app status v0.1.20 bundle=example
KB app bridge: ticket published
'''
clean='''FVAspect8 constructor reached (before Foundation)
KHINSTALL status=installed
KHROUTE abi-fix-1 installedMask=511
KHROUTE abi-fix-1 seq=1 mode=native route=remote-did-handle-change
KHSTATE seq=1 plainReadable=1 iavReadable=1 iav=(0,0,430,320)
KHHOST seq=1 route=6 bundle=example hostScene=abc settings=(0,0,430,932)
KHWORKER seq=2 route=4 plain=(0,0,0,0); argument-only
KHROUTE abi-fix-1 seq=3 mode=ratio9x16 route=remote-did-handle-change
KHSTATE seq=3 plainReadable=1 iavReadable=1 iav=(0,0,430,320)
KHHOST seq=3 route=6 bundle=example hostScene=abc settings=(0,0,430,764.444)
'''
a=analyze(prefix+clean)
assert a['legacy_markers_absent_in_session'] and not a['functional_fix_proven']
assert a['session_start_line']==5 and len(a['state_host_samples'])==2
assert {s['mode'] for s in a['state_host_samples']}=={'native','ratio9x16'}
assert a['worker_routes']=={'4':1} and not a['has_correlation_records']
b=analyze(prefix+clean+'KB app bridge: new old-component marker\n')
assert not b['legacy_markers_absent_in_session'] and len(b['legacy_marker_lines_in_session'])==1
assert analyze(clean+'KHCORR seq=1 pid=7\n')['has_correlation_records']
assert not analyze('old log only')['has_installation']
assert not analyze(clean)['functional_fix_proven']
print('PASS session replay: old cumulative history excluded, current legacy markers retained, native/ratio linked, no inferred app delivery')
