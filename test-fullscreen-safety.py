from pathlib import Path
s=Path(__file__).with_name('FVKeyboardDock.inc').read_text()
a=s.index('static void fvKDRestoreLayersForHost(');b=s.index('\n}\nstatic void fvKDDropForHost',a)
restore=s[a:b]
assert 'objectAtIndex:",i' in restore
assert 'fvKDClearHostTicket' not in restore
print('PASS Drop callback only restores owned keyboard layers; no ticket/cache I/O')
