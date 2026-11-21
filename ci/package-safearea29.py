from pathlib import Path
import hashlib,json,plistlib,struct,subprocess,tempfile,zipfile
R=Path(__file__).resolve().parents[1]
status=json.loads((R/'ci/safearea29-run.json').read_text())
COMMIT=subprocess.check_output(['git','rev-parse','HEAD'],cwd=R,text=True).strip()
assert status['head_sha']==COMMIT and status['conclusion']=='success'
RUN=status['run_id'];ARTIFACT=status['artifact_id'];BUILD=status['build'];DIGEST=status['sha256']
ARCHIVE=Path(status['archive']);MODE='host-safearea-experiment';NAME='zzFVIndependentKeyboardRouteProbe'
ARCHES=[('arm64',0),('arm64e',0x80000002)]
def sha(b):return hashlib.sha256(b).hexdigest()
assert sha(ARCHIVE.read_bytes())==DIGEST
MARKERS=[b'SA29 A1',b'SA29 B host-readback',b'SA29 A2',b'SA29 REJECT',b'cleanup-unconfirmed',b'sceneSafeAreaAB29',b'safeAreaInsetsPortrait',b'setSafeAreaInsetsPortrait:',b'UIEdgeInsetsValue',b'objectForSetting:',b'setObject:forSetting:',b'KHSESSION',b'KB dock applied']
FORBIDDEN=[b'KB app bridge',b'KB app status v0.1.20',b'fvKHLayoutKeyboardViews',b'fvKHPrepareHostedWindow',b'portrait real-event adapter installed']
def signature(b,subtype):
 assert struct.unpack_from('<4I',b)==(0xfeedfacf,0x100000c,subtype,6)
 ncmds,sizeofcmds=struct.unpack_from('<II',b,16);pos=32;commands={}
 for _ in range(ncmds):
  cmd,length=struct.unpack_from('<II',b,pos);assert length>=8 and pos+length<=32+sizeofcmds<=len(b)
  assert cmd not in commands or cmd not in (0x1d,0x80000034)
  commands[cmd]=pos;pos+=length
 assert pos==32+sizeofcmds and 0x1d in commands and 0x80000034 in commands
 sig,limit=struct.unpack_from('<II',b,commands[0x1d]+8);assert sig+limit<=len(b)
 magic,total,slots=struct.unpack_from('>III',b,sig);assert magic==0xfade0cc0 and 12+8*slots<=total<=limit
 dirs=[]
 for i in range(slots):
  slot,offset=struct.unpack_from('>II',b,sig+12+8*i);assert offset+8<=total
  bloblen=struct.unpack_from('>I',b,sig+offset+4)[0];assert offset+bloblen<=total
  if slot==0:dirs.append(offset)
 assert len(dirs)==1;cd=sig+dirs[0]
 magic,cdlen,version,flags,hashes=struct.unpack_from('>5I',b,cd)
 special,pages,codelimit=struct.unpack_from('>III',b,cd+24)
 hs,kind,platform,exp=struct.unpack_from('>4B',b,cd+36)
 assert magic==0xfade0c02 and kind==2 and hs==32 and codelimit==sig and exp<=20
 assert hashes>=special*hs and hashes+pages*hs<=cdlen and cd+cdlen<=sig+total
 page=1<<exp;assert pages==(codelimit+page-1)//page
 for i in range(pages):assert hashlib.sha256(b[i*page:min(codelimit,(i+1)*page)]).digest()==b[cd+hashes+hs*i:cd+hashes+hs*(i+1)]
 return {'signature_pages':pages,'page_size':page,'sha256':sha(b),'bytes':len(b),'cpu_subtype':hex(subtype),'code_directory_sha256':sha(b[cd:cd+cdlen])}
def fat_slices(b):
 magic,n=struct.unpack_from('>II',b);assert magic==0xcafebabe and n==2
 out={};intervals=[]
 for i,(arch,subtype) in enumerate(ARCHES):
  cpu,sub,offset,size,align=struct.unpack_from('>5I',b,8+20*i)
  assert cpu==0x100000c and sub==subtype and align<=20 and offset%(1<<align)==0 and offset>=48 and offset+size<=len(b)
  out[arch]=b[offset:offset+size];intervals.append((offset,offset+size))
 assert intervals[0][1]<=intervals[1][0]
 return out
with zipfile.ZipFile(ARCHIVE) as z:
 assert len(z.namelist())==len(set(z.namelist()))
 rows=json.loads(z.read('build-results.json'));manifest={(x['mode'],x['arch']):x['sha256'] for x in rows}
 assert len(rows)==len(manifest)==20
 for row in rows:assert sha(z.read(row['mode']+'-'+row['arch']+'.dylib'))==row['sha256']
 slices={};slice_results={}
 for arch,subtype in ARCHES:
  b=z.read(MODE+'-'+arch+'.dylib');assert sha(b)==manifest[(MODE,arch)]
  assert all(m in b for m in MARKERS),arch
  assert all(m not in b for m in FORBIDDEN),arch
  assert b'dynamically looked up' not in z.read(MODE+'-'+arch+'-imports.txt')
  slice_results[arch]=signature(b,subtype);slices[arch]=b
  print('Verified',arch,json.dumps(slice_results[arch]))
 fat=z.read('FVIndependent-'+MODE+'.dylib');assert fat_slices(fat)==slices
 toolchain=z.read('toolchain.txt').decode()
package='com.minis.floatingview.keyboard-host-route-test';version='0.1.29'
out=R/(package+'_'+version+'_iphoneos-arm64e.deb')
control=f'''Package: {package}
Name: floatingView HOST Scene SafeArea A-B-A EXPERIMENT
Version: {version}
Architecture: iphoneos-arm64e
Maintainer: Minis
Section: Tweaks
Depends: com.be-huge.floating-view (>= 1.6.0-6), firmware (>= 16.1)
Description: Default-OFF host-only per-Scene safe-area A/B/A experiment from Apple build {BUILD}. One reversible bottom+12 logical-point probe on a uniquely matched portrait inner-keyboard Scene. Explicit-zero/presence and strict runtime ABI gates; absent backing fields are refused. Restore only the original inset on the same retained Scene while still owned using freshest settings. Preserves existing ratio/dock and app blacklist; no app adapter/injection. NOT proven keyboard avoidance or device validated. Disable, hide keyboard, close Scene and confirm restore before uninstall; respring if cleanup is unconfirmed. Uninstall does not run runtime cleanup.
'''
with tempfile.TemporaryDirectory() as td:
 stage=Path(td)/'stage';lib=stage/'Library/MobileSubstrate/DynamicLibraries';deb=stage/'DEBIAN';lib.mkdir(parents=True);deb.mkdir()
 (deb/'control').write_text(control)
 (lib/(NAME+'.dylib')).write_bytes(fat)
 plist=plistlib.dumps({'Filter':{'Bundles':['com.apple.springboard','com.apple.Preferences']}})
 (lib/(NAME+'.plist')).write_bytes(plist)
 candidate=Path(td)/out.name
 subprocess.run(['dpkg-deb','--root-owner-group','-Zgzip','-b',str(stage),str(candidate)],check=True)
 check=Path(td)/'check';subprocess.run(['dpkg-deb','-R',str(candidate),str(check)],check=True)
 expected={str(p.relative_to(stage)):p.read_bytes() for p in stage.rglob('*') if p.is_file()}
 actual={str(p.relative_to(check)):p.read_bytes() for p in check.rglob('*') if p.is_file()}
 assert expected==actual and len(actual)==3
 payload=['Library/MobileSubstrate/DynamicLibraries/'+NAME+ext for ext in ['.dylib','.plist']]
 assert set(actual)==set(payload+['DEBIAN/control'])
 assert not any('var/jb' in path or (check/path).is_symlink() for path in actual)
 assert plistlib.loads(actual[payload[1]])=={'Filter':{'Bundles':['com.apple.springboard','com.apple.Preferences']}}
 final_slices=fat_slices(actual[payload[0]]);assert final_slices==slices
 for arch,subtype in ARCHES:assert signature(final_slices[arch],subtype)==slice_results[arch]
 for field,value in [('Package',package),('Version',version),('Architecture','iphoneos-arm64e')]:
  assert subprocess.check_output(['dpkg-deb','-f',str(candidate),field],text=True).strip()==value
 listing=subprocess.check_output(['dpkg-deb','-c',str(candidate)],text=True)
 assert all('root/root' in line for line in listing.splitlines())
 out.write_bytes(candidate.read_bytes())
 file_checks={path:{'sha256':sha(body),'bytes':len(body)} for path,body in actual.items()}
evidence={'package':package,'version':version,'commit':COMMIT,'run':RUN,'build':BUILD,'ci_conclusion':'success','artifact':{'id':ARTIFACT,'bytes':ARCHIVE.stat().st_size,'sha256':DIGEST},'deb':{'file':out.name,'bytes':out.stat().st_size,'sha256':sha(out.read_bytes())},'slices':slice_results,'all_manifest_entries_verified':len(rows),'fat_slices_equal_original':True,'final_deb_files_verified':file_checks,'filter_bundles':['com.apple.springboard','com.apple.Preferences'],'root_owner_group_verified':True,'toolchain':toolchain,'device_validation':False,'confirmed_avoidance_fix':False,'notes':'New default-OFF per-Scene safe-area experiment. Existing geometry/dock retained; no added frame/cache/display or app-process mutation. Absent field refused, explicit zero restored. Device/client behavior unverified.'}
(R/'ci/safearea29-delivery-evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
print('PACKAGE',out);print('BYTES',out.stat().st_size);print('SHA256',sha(out.read_bytes()))
print('PASS 20 manifest entries, original two-architecture signed slices, all code pages, fat identity, exact deb payload/control/filter/owner. Device behavior unverified.')
