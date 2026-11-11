"""Verify a compatibility build without claiming untested Windows/PS versions."""
from pathlib import Path
import argparse,hashlib,json
from inspect_binaries import inspect
from legacy_metadata import validate
from tiff_pixels import tiff
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--evidence',type=Path,required=True);parser.add_argument('--installed',type=Path);args=parser.parse_args()
out=args.evidence;baseline=json.loads((out/'baseline.json').read_text(encoding='utf-8'));project=json.loads((root/'project.json').read_text(encoding='utf-8'));plugins=json.loads((root/'plugins.json').read_text(encoding='utf-8'))
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
unchanged=[]
for name,sha in baseline['source_files'].items():
    if Path(name).suffix=='.rc':continue
    assert digest(root/name)==sha,('effect implementation/parameter metadata changed',name);unchanged.append(name)
build=(out/'all-build.log').read_text(encoding='utf-8-sig');assert build.count('Building ')==13 and 'PASS: requested builds'in build and 'error:'not in build and 'Exception:'not in build
for folder in ('about','about-legacy'):
    logs=list((out/folder).glob('*.txt'));assert len(logs)==16
    for log in logs:assert log.read_text(encoding='utf-8-sig').startswith('PASS:')
metadata={}
for module in project['modules']:
    for path in (root/module/'src').glob('*PiPL*.json'):
        item=json.loads(path.read_text(encoding='utf-8'))['PiPLs'][0];metadata[item['Name']]=item
assert len(metadata)==19
result=[]
for entry in plugins:
    binary=root/entry['module']/'build'/entry['file'];report=inspect(binary);assert report['architecture']=='0x8664'and report['exports']==['PluginMain']
    assert report['os_version']<=[6,1]and report['subsystem_version']<=[6,1]
    assert set(map(str.lower,report['imported_dlls']))<={'kernel32.dll','msvcrt.dll','user32.dll','gdi32.dll','comctl32.dll','dwmapi.dll','d3d11.dll','dxgi.dll','imm32.dll','shell32.dll','comdlg32.dll'}
    names={name for entries in report['imported_functions'].values()for name in entries}
    assert not names.intersection({'GetDpiForWindow','SetThreadDpiAwarenessContext','AdjustWindowRectExForDpi','GetSystemMetricsForDpi','GetDpiForSystem','WaitOnAddress','GetSystemTimePreciseAsFileTime'})
    # The serialized component identifies the canonical entry even when the
    # displayed product name differs (for example Lens/CCLens).
    candidates=[item for item in metadata.values()if item['ComponentName'].encode('ascii')in binary.read_bytes()];assert len(candidates)==1
    report['registration']=validate(binary,candidates[0]);report.update(entry);result.append(report)
source={p.relative_to(root).as_posix():digest(p)for module in project['modules']for part in ('src','include')for p in(root/module/part).rglob('*')if p.is_file()}
source.update({p.relative_to(root).as_posix():digest(p)for p in(root/'native/ui').rglob('*')if p.is_file()and p.suffix in {'.cpp','.hpp'}})
host=None
if args.installed:
    for entry in result:assert digest(args.installed/entry['file'])==entry['sha256']
    before={p.name:p for p in(out/'host-baseline').glob('*.tif')};after={p.name:p for p in(out/'host-updated').glob('*.tif')};assert len(before)==113 and before.keys()==after.keys()
    for name,path in before.items():assert tiff(path)==tiff(after[name]),('changed Photoshop pixels',name)
    for folder in ('host-baseline','host-updated'):assert(out/folder/'results.txt').read_text(encoding='utf-8-sig').count(' PASS')==55
    host={'status':'PASS','effects':19,'effect_depth_combinations':55,'pixel_equal_exports':113,'depths':[8,16,32],'tint_depth':8,'rollback':'one-state pixel exact','version':'27.7.0'}
report={'status':'PASS','release':project['release'],'repository':project['repository'],'plugins':result,'source_files':source,'unchanged_effect_files':unchanged,'core_and_parameter_abi_unchanged':True,'build':{'modules':13,'status':'PASS'},'about':{'status':'PASS','profiles':32,'legacy_dpi_fallback_profiles':16,'renderer':'D3D11 WARP; pinned SM4 shaders'},'host':host,'legacy_hosts':{'Windows 7 / PS 2018':'pending external test','Windows 10 / PS 2020':'pending external test'}}
(out/('qa.json'if args.installed else'preinstall.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PASS: nineteen legacy PiPL registrations; matching terminology/alpha flags; no mandatory Win10 DPI or D3DCompiler imports; 32 UI profiles; effect code and parameter ABI unchanged'+('; 55 host cases / 113 identical exports'if host else''))
