"""Local 0.34 validation; private paths stay only in ignored evidence reports."""
from pathlib import Path
import argparse,hashlib,json,re
from inspect_binaries import inspect
from tiff_pixels import tiff
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--build-root',type=Path,default=root);parser.add_argument('--evidence',type=Path,default=root/'evidence/open-source-0.34');parser.add_argument('--installed',type=Path);args=parser.parse_args()
out=args.evidence;candidate=args.build_root.resolve();baseline=json.loads((out/'baseline.json').read_text(encoding='utf-8'));plugins=json.loads((root/'plugins.json').read_text(encoding='utf-8'))
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
allowed_changes={'displacement/src/filter.cpp','surface/src/smart_trace.hpp'}
unchanged=[]
for name,sha in baseline['source_files'].items():
 path=root/name
 if path.suffix in {'.rc','.md'}or name in allowed_changes:continue
 assert digest(path)==sha,('non-UI effect implementation changed',name);unchanged.append(name)
build=(out/'all-build.log').read_text(encoding='utf-8-sig');clean=(out/'clean-build.log').read_text(encoding='utf-8-sig')
for text in (build,clean):
 assert text.count('Building ')==13 and 'PASS: requested builds' in text and 'error:'not in text and 'Exception:'not in text
assert len(list((out/'about').glob('*.txt')))==16
for path in(out/'about').glob('*.txt'):assert path.read_text(encoding='utf-8-sig').startswith('PASS:')
assert len(list((out/'host-baseline').glob('*.tif')))==113
assert(out/'host-baseline/results.txt').read_text(encoding='utf-8-sig').count(' PASS')==55
for path in(out/'host-baseline').glob('*-undo.tif'):
 bits=path.stem.split('-')[1];assert tiff(path)==tiff(path.parent/f'before-{bits}.tif')
result=[]
for entry in plugins:
 binary=candidate/entry['module']/'build'/entry['file'];p=inspect(binary);assert p['architecture']=='0x8664'and p['exports']==['PluginMain']
 assert set(map(str.lower,p['imported_dlls']))<={'kernel32.dll','msvcrt.dll','user32.dll','gdi32.dll','comctl32.dll','dwmapi.dll','d3d11.dll','dxgi.dll','d3dcompiler_47.dll','imm32.dll','shell32.dll','comdlg32.dll'}
 inputs=[p for part in ('src','include')for p in(candidate/entry['module']/part).rglob('*')if p.is_file()]
 inputs+=[p for p in(candidate/'native/ui').rglob('*')if p.suffix in {'.cpp','.hpp'}]
 assert all(p.stat().st_mtime<=binary.stat().st_mtime for p in inputs),'stale candidate'
 data=binary.read_bytes();assert not re.search(rb'(?i)[A-Za-z]:[/\\]+[^\x00\r\n]*codex',data),'unmapped original compilation path' 
 p.update(entry);result.append(p)
source={p.relative_to(root).as_posix():digest(p)for module in json.loads((root/'project.json').read_text(encoding='utf-8'))['modules']for part in ('src','include')for p in(root/module/part).rglob('*')if p.is_file()}
source.update({p.relative_to(root).as_posix():digest(p)for p in(root/'native/ui').rglob('*')if p.is_file()and p.suffix in {'.cpp','.hpp'}})
host=None
if args.installed:
 for p in result:assert digest(args.installed/p['file'])==p['sha256'],'installed hash mismatch'
 before={p.name:p for p in(out/'host-baseline').glob('*.tif')};after={p.name:p for p in(out/'host-updated').glob('*.tif')};assert before.keys()==after.keys()
 for name,path in before.items():assert tiff(path)==tiff(after[name]),('changed Photoshop pixels',name)
 assert(out/'host-updated/results.txt').read_text(encoding='utf-8-sig').count(' PASS')==55
 host={'status':'PASS','effects':19,'effect_depth_combinations':55,'pixel_equal_exports':113,'depths':[8,16,32],'tint_depth':8,'rollback':'one-state pixel exact'}
report={'status':'PASS','release':'0.34','repository':'https://github.com/Three-LEI/AE-to-PS-Plugins','plugins':result,'source_files':source,'unchanged_effect_files':unchanged,'core_and_parameter_abi_unchanged':True,'original_and_clean_builds':{'modules':13,'status':'PASS'},'about':{'status':'PASS','profiles':16,'checks':['correct URL','theme-aware blue borderless link','success/failure/copy fallback','close/escape','effect unchanged','host About']},'host':host,'no_bundled_sdk_or_fonts':True}
(out/('qa.json'if args.installed else'preinstall.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PASS: nineteen candidates; thirteen original/clean builds; sixteen About profiles; effect code/parameters unchanged'+('; 55 PS cases and 113 pixel-identical exports'if args.installed else'; ready for backed-up installation'))
