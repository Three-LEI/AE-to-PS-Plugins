"""Export only public source and docs; keep the research workspace untouched."""
from pathlib import Path
import argparse,hashlib,json,shutil,zipfile
from source_checks import check,digest
parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);parser.add_argument('--archive',type=Path);parser.add_argument('--report',type=Path);args=parser.parse_args()
root=Path(__file__).resolve().parents[1];output=args.output.resolve();assert output!=root and root not in output.parents or output.is_relative_to(root/'publish')or output.is_relative_to(root/'dist'),'use a separate export folder'
project=json.loads((root/'project.json').read_text(encoding='utf-8'));selected=set()
for name in ('README.md','LICENSE','THIRD_PARTY.md','CONTRIBUTING.md','.gitignore','.gitattributes','project.json','plugins.json','dependencies.lock.json','build.ps1'):selected.add(root/name)
for folder in ('scripts','qa','docs','.github','third_party/imgui'):
 for path in (root/folder).rglob('*'):
  if path.is_file()and 'build'not in path.relative_to(root/folder).parts and '__pycache__'not in path.parts and path.suffix in {'.py','.ps1','.cpp','.hpp','.h','.jsx','.md','.yml','.yaml','.json','.txt'}:selected.add(path)
for module in project['modules']:
 for name in ('build.ps1','README.md','LICENSE'):
  if(root/module/name).exists():selected.add(root/module/name)
 for part in ('include','src','tests')+ (('common','ui')if module=='native'else()):
  for path in (root/module/part).rglob('*'):
   if path.is_file()and path.suffix in {'.cpp','.hpp','.h','.inc','.json','.rc','.md'}and 'build'not in path.relative_to(root/module/part).parts:selected.add(path)
selected.add(root/'native/tests/build-scrub-tests.ps1');selected.add(root/'native/ui/verify-contract.ps1');selected.add(root/'native/build-library-ui.ps1')
for source in sorted(selected):
 target=output/source.relative_to(root);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
report=check(output);report['files']={path.relative_to(root).as_posix():digest(path)for path in sorted(selected)}
if args.archive:
 args.archive.parent.mkdir(parents=True,exist_ok=True)
 with zipfile.ZipFile(args.archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6)as archive:
  for source in sorted(selected):archive.write(output/source.relative_to(root),output.name+'/'+source.relative_to(root).as_posix())
 with zipfile.ZipFile(args.archive)as archive:
  assert archive.testzip()is None
  for name,sha in report['files'].items():assert hashlib.sha256(archive.read(output.name+'/'+name)).hexdigest()==sha
 report['archive']={'file':args.archive.name,'bytes':args.archive.stat().st_size,'sha256':digest(args.archive)}
if args.report:args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PASS: public source export; '+str(report['source_files'])+' files; '+str(report['source_bytes'])+' bytes; original research files retained')
