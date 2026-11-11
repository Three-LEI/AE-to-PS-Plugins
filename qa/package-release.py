"""Package verified binaries without historical screenshots, SDKs or local reports."""
from pathlib import Path
import hashlib,json,shutil,subprocess,sys,zipfile
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'scripts'))
from source_checks import digest
out=root/'evidence/open-source-0.34';qa=json.loads((out/'qa.json').read_text(encoding='utf-8'));assert qa['status']=='PASS'and qa['host']['pixel_equal_exports']==113
project=json.loads((root/'project.json').read_text(encoding='utf-8'));folder=root/'dist'/('AE-to-PS-Plugins-'+project['release']);folder.mkdir(parents=True,exist_ok=True)
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
(folder/'安装说明.txt').write_text('''AE-to-PS-Plugins / 整包0.34 / 十九款

退出Photoshop，备份已有同名插件，将Plug-ins中的所需.8bf复制到Photoshop的Plug-ins目录并重新启动。
效果菜单：滤镜 → ZaYu-AE 效果复刻。普通使用无需AE、SDK、工具链或Python。

本次将全部插件“关于”底部改为蓝色无边框“开源地址”，点击用系统默认浏览器打开：
https://github.com/Three-LEI/AE-to-PS-Plugins
适用于滤镜窗口内关于和Photoshop关于增效工具入口。署名、效果版本、参数及生成公式保持不变。
源码按MIT开源；第三方组件与外部依赖按各自许可使用。

多数效果支持RGB8/16/32，Tint仅RGB8。新构建经过完整源码构建与宿主适配检查，PS的55个效果／位深组合、113份导出与0.33像素一致。
智能滤镜保存、透明度和尺寸边界以各模块说明为准。Glass／Blobbylize外部参考层仍需手动重开效果更新，未附加刷新脚本。

独立近似复刻，与原版不保证逐像素一致。同一种子不保证与AE相同随机纹理。详见效果说明和仓库文档。
已验证Windows x64、Photoshop 2026（27.7.0）。请先在副本或智能对象上进行小图验证。

by：淚泪泪
''',encoding='utf-8')
files={p.relative_to(folder).as_posix():digest(p)for p in sorted(folder.rglob('*'))if p.is_file()and p.name!='manifest.json'}
manifest={'release':project['release'],'repository':project['repository'],'source_commit':commit,'plugins':entries,'files':files,'core_and_parameter_abi_unchanged':True,'production_refresh_helper':False}
(folder/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');archive=folder.parent/(folder.name+'.zip')
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6)as z:
 for path in sorted(folder.rglob('*')):
  if path.is_file():z.write(path,folder.name+'/'+path.relative_to(folder).as_posix())
with zipfile.ZipFile(archive)as z:
 assert z.testzip()is None and sum(name.endswith('.8bf')for name in z.namelist())==19
 for name,sha in files.items():assert hashlib.sha256(z.read(folder.name+'/'+name)).hexdigest()==sha
source=root/'dist/AE-to-PS-Plugins-source-0.34.zip';assert source.exists();checksum=root/'dist/SHA256SUMS-0.34.txt'
checksum.write_text(''.join(digest(path)+'  '+path.name+'\n'for path in (archive,source)),encoding='ascii')
report={'status':'PASS','release':'0.34','source_commit':commit,'archives':{path.name:{'bytes':path.stat().st_size,'sha256':digest(path)}for path in (archive,source)},'binary_hashes_verified':19,'files_verified':len(files)}
(out/'package-check.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('PASS: 0.34 plugin/source archives; nineteen binary hashes; no historical research artifacts')
