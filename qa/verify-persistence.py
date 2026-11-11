"""Candidate audit for the shared persistence upgrade; host checks are optional
until installation, then required before packaging.
"""
from pathlib import Path
import argparse,hashlib,json,sys,zipfile
from inspect_binaries import inspect
from legacy_metadata import validate
from tiff_pixels import tiff
root=Path(__file__).resolve().parents[1];parser=argparse.ArgumentParser();parser.add_argument('--evidence',type=Path,default=root/'evidence/persistence-0.36');parser.add_argument('--installed',type=Path);args=parser.parse_args();out=args.evidence
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
project=json.loads((root/'project.json').read_text(encoding='utf-8'));baseline=json.loads((out/'baseline.json').read_text(encoding='utf-8'));entries=json.loads((root/'plugins.json').read_text(encoding='utf-8'))
unchanged=[]
for name,sha in baseline['source_files'].items():
    if Path(name).stem.endswith('_core'):
        assert digest(root/name)==sha,('effect core/parameter ABI changed',name);unchanged.append(name)
assert 'PASS: 8642 checks'in(out/'parameter-store.log').read_text(encoding='utf-8-sig')
for scale in ('1','2'):assert(out/'font-reset'/(scale+'.txt')).read_text(encoding='utf-8-sig').startswith('PASS:')
for folder in ('about','about-legacy'):
    profiles=list((out/folder).glob('*.txt'));assert len(profiles)==16 and all(p.read_text(encoding='utf-8-sig').startswith('PASS:')for p in profiles)
assert 'PASS: requested builds'in(out/'clean-native-build.log').read_text(encoding='utf-8-sig')
log=(out/'all-build.log').read_text(encoding='utf-8-sig');assert log.count('Building ')==13 and 'PASS: requested builds'in log and 'error:'not in log and 'Exception:'not in log
metadata={}
for module in project['modules']:
    for p in(root/module/'src').glob('*PiPL*.json'):
        item=json.loads(p.read_text(encoding='utf-8'))['PiPLs'][0];metadata[item['ComponentName']]=item
with zipfile.ZipFile(root/'dist/AE-to-PS-Plugins-source-0.35-rc2.zip')as archive:
    for module in project['modules']:
        for p in(root/module/'src').glob('*PiPL*.json'):
            old=json.loads(archive.read('AE-to-PS-Plugins/'+p.relative_to(root).as_posix()).decode('utf-8-sig'))['PiPLs'][0];now=metadata[old['ComponentName']]
            for key,value in old.items():
                if key in {'HasTerminology','Terminology'}and module not in {'surface','creative','roughen'}:continue
                assert now[key]==value,('previous host contract changed',p.name,key)
result=[]
for entry in entries:
    binary=root/entry['module']/'build'/entry['file'];item=inspect(binary);assert item['architecture']=='0x8664'and item['exports']==['PluginMain']
    assert item['os_version']<=[6,1]and item['subsystem_version']<=[6,1]
    assert set(map(str.lower,item['imported_dlls']))<={'kernel32.dll','msvcrt.dll','user32.dll','gdi32.dll','comctl32.dll','dwmapi.dll','d3d11.dll','dxgi.dll','imm32.dll','shell32.dll','comdlg32.dll'}
    names={f for values in item['imported_functions'].values()for f in values};assert not names&{'GetDpiForWindow','SetThreadDpiAwarenessContext','AdjustWindowRectExForDpi','WaitOnAddress','GetSystemTimePreciseAsFileTime'}
    matches=[v for k,v in metadata.items()if k.encode('ascii')in binary.read_bytes()];assert len(matches)==1
    item['registration']=validate(binary,matches[0]);assert item['registration']['terminology_parameters']>0
    item.update(entry);result.append(item)
source={p.relative_to(root).as_posix():digest(p)for module in project['modules']for part in ('src','include')for p in(root/module/part).rglob('*')if p.is_file()}
source.update({p.relative_to(root).as_posix():digest(p)for folder in ('native/ui','native/common')for p in(root/folder).rglob('*')if p.is_file()and p.suffix in {'.cpp','.hpp','.json'}})
host=None
smart=None
if args.installed:
    for entry in result:assert digest(args.installed/entry['file'])==entry['sha256']
    before={p.name:p for p in(out/'host-baseline').glob('*.tif')};after={p.name:p for p in(out/'host-updated').glob('*.tif')};assert len(before)==113 and before.keys()==after.keys()
    for name,path in before.items():assert tiff(path)==tiff(after[name]),('changed pixels',name)
    assert(out/'host-updated/results.txt').read_text(encoding='utf-8-sig').count(' PASS')==55
    host={'status':'PASS','effects':19,'effect_depth_combinations':55,'pixel_equal_exports':113,'version':(out/'host-updated/results.txt').read_text(encoding='utf-8-sig').splitlines()[0][3:]}
    folder=out/'smart';expected_report=[[8,19,38,'PASS'],[16,18,36,'PASS'],[32,18,36,'PASS']]
    for phase in ('create','reopen'):assert json.loads((folder/('results-'+phase+'.json')).read_text(encoding='utf-8-sig'))==expected_report
    prior=json.loads((folder/'process-before-quit.json').read_text(encoding='utf-8-sig'));current=json.loads((folder/'process-reopened.json').read_text(encoding='utf-8-sig'))
    if isinstance(current,dict):current=[current]
    assert current and all(p['ProcessId']!=prior['Id']for p in current),'Photoshop was not restarted'
    cases=json.loads((folder/'cases.json').read_text(encoding='utf-8-sig'));types={'number':'DescValueType.DOUBLETYPE','integer':'DescValueType.INTEGERTYPE','flag':'DescValueType.BOOLEANTYPE','layer':'DescValueType.STRINGTYPE'}
    for bits in (8,16,32):
        for index,snapshots in json.loads((folder/f'smart-{bits}-expected.json').read_text(encoding='utf-8-sig')):
            row=cases[index]
            if 'legacy'in row:continue
            for variant,snapshot in enumerate(snapshots):
                assert snapshot[0]==row['event'];actual={key:(kind,value)for key,kind,value in snapshot[1:]}
                expected={f['id']:(types[f['type']],('鎏金_参考图层②'if variant and f['type']=='layer'else f['value']))for f in row['fixtures'][variant]}
                assert actual==expected,('non-default host parameters changed',bits,row['name'],variant)
    bases=list(folder.glob('*-base.tif'));assert len(bases)==55
    for path in bases:
        for suffix in ('reopened','forced'):assert tiff(path)==tiff(path.with_name(path.name.replace('-base','-'+suffix))),('Smart Filter pixels changed',path.name,suffix)
    legacy=json.loads((folder/'legacy-results.json').read_text(encoding='utf-8-sig'));assert len(legacy)==12 and all(row[1]>0 and row[2]=='PASS'for row in legacy)
    smart={'status':'PASS','effect_depth_combinations':55,'independent_instances':110,'psd_depths':[8,16,32],'pixel_equal_comparisons':110,'photoshop_restarted':True,'legacy_psds':12,'legacy_instances':sum(row[1]for row in legacy),'reference_name':'鎏金_参考图层'}
report={'status':'PASS','release':project['release'],'repository':project['repository'],'plugins':result,'source_files':source,'unchanged_core_files':unchanged,'core_and_parameter_abi_unchanged':True,'build':{'modules':13,'status':'PASS','clean_export_native':'PASS'},'host':host,'smart_filters':smart,'parameter_store':{'status':'PASS','checks':8642,'new_schema_effects':13,'legacy_schema_effects':6},'font_reset':{'status':'PASS','glyph':'鎏','dpi_scales':[1,2]},'about':{'status':'PASS','profiles':32,'legacy_dpi_fallback_profiles':16},'external_compatibility':'rc2 user-confirmed PS2018/2020; this candidate still needs external retest'}
(out/('qa.json'if args.installed else'preinstall.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PASS: nineteen registered parameter schemas; existing six wire contracts preserved; pixel cores/parameter layouts unchanged; typed storage and glyph/reset tests'+('; 113 identical Photoshop exports'if host else''))
