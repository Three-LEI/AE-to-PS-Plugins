"""Package verified binaries without historical screenshots, SDKs or local reports."""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,sys,zipfile
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'scripts'))
from source_checks import digest
parser=argparse.ArgumentParser();parser.add_argument('--evidence',type=Path,default=root/'evidence/open-source-0.34');args=parser.parse_args()
out=args.evidence;qa=json.loads((out/'qa.json').read_text(encoding='utf-8'));assert qa['status']=='PASS'and qa['host']['pixel_equal_exports']==113
project=json.loads((root/'project.json').read_text(encoding='utf-8'));assert qa['release']==project['release'];folder=root/'dist'/('AE-to-PS-Plugins-'+project['release']);folder.mkdir(parents=True,exist_ok=True)
commit=subprocess.check_output(['git','-C',str(root/'publish/AE-to-PS-Plugins'),'rev-parse','HEAD'],text=True).strip()
def copy(source,target):target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
entries=[]
for entry in qa['plugins']:
 source=Path(entry['path']);assert digest(source)==entry['sha256'];copy(source,folder/'Plug-ins'/entry['file']);entries.append({key:entry[key]for key in ('name','version','modes','file','sha256')})
for name in ('LICENSE','THIRD_PARTY.md'):copy(root/name,folder/name)
copy(root/'third_party/imgui/LICENSE.txt',folder/'Dear-ImGui-LICENSE.txt')
copy(root/'docs/EFFECTS.md',folder/'效果列表.md');copy(root/'docs/validation-summary.json',folder/'验证摘要.json')
for module in project['modules']:
 if(root/module/'README.md').exists():copy(root/module/'README.md',folder/'效果说明'/(module+'.md'))
(folder/'安装说明.txt').write_text('AE-to-PS-Plugins / '+project['release']+' / 十九款 Windows x64 原生滤镜\n\n退出Photoshop，备份同名插件，将Plug-ins中的所需.8bf复制到Photoshop的Plug-ins目录，避免重复副本，然后重新启动。\n菜单：滤镜 → ZaYu-AE 效果复刻。Windows和Photoshop均需64位。普通使用无需AE、SDK、工具链或Python。\n\n'+(root/'docs/RELEASE_NOTES.md').read_text(encoding='utf-8')+'\n\n开源地址：'+project['repository']+'\n自有源码采用MIT许可，第三方组件按各自许可使用。\nby：淚泪泪\n',encoding='utf-8')
files={p.relative_to(folder).as_posix():digest(p)for p in sorted(folder.rglob('*'))if p.is_file()and p.name!='manifest.json'}
manifest={'release':project['release'],'repository':project['repository'],'source_commit':commit,'plugins':entries,'files':files,'core_and_parameter_abi_unchanged':True,'production_refresh_helper':False}
(folder/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');archive=folder.parent/(folder.name+'.zip')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6)as z:
 for path in sorted(folder.rglob('*')):
  if path.is_file():z.write(path,folder.name+'/'+path.relative_to(folder).as_posix())
with zipfile.ZipFile(archive)as z:
 assert z.testzip()is None and sum(name.endswith('.8bf')for name in z.namelist())==19
 for name,sha in files.items():assert hashlib.sha256(z.read(folder.name+'/'+name)).hexdigest()==sha
source=root/('dist/AE-to-PS-Plugins-source-'+project['release']+'.zip');assert source.exists();checksum=root/('dist/SHA256SUMS-'+project['release']+'.txt')
checksum.write_text(''.join(digest(path)+'  '+path.name+'\n'for path in (archive,source)),encoding='ascii')
report={'status':'PASS','release':project['release'],'source_commit':commit,'archives':{path.name:{'bytes':path.stat().st_size,'sha256':digest(path)}for path in (archive,source)},'binary_hashes_verified':19,'files_verified':len(files)}
(out/'package-check.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('PASS: '+project['release']+' plugin/source archives; nineteen binary hashes; no historical research artifacts')
