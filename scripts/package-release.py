"""Package a validated stable snapshot; never performs network or Git writes."""
from pathlib import Path
import argparse,hashlib,json,re,zipfile

root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--report-dir',type=Path,required=True);p.add_argument('--binary-root',type=Path,default=root);p.add_argument('--screenshots',action='store_true');args=p.parse_args()
out=args.report_dir.resolve();project=json.loads((root/'project.json').read_text('utf8'));release=project['release'];count=project['effect_count']
assert re.fullmatch(r'\d+\.\d+(?:\.\d+)?',release)
assert project['release_policy']['channel']=='stable'and project['development_status']=='stable'
validation=json.loads((root/'docs/validation-summary.json').read_text('utf8'))
assert validation['release']==release and validation['status']=='PASS'
assert validation['build']['clean_export'].startswith('PASS')
source=root/'dist'/f'AE-to-PS-Plugins-{release}-source.zip'
source_report=json.loads((out/'source-export.json').read_text('utf8'))
digest=lambda data:hashlib.sha256(data).hexdigest()
assert digest(source.read_bytes())==source_report['archive']['sha256']
name=f'AE-to-PS-Plugins-{release}-Windows-x64';payload={}
for entry in validation['plugins']:
 data=(args.binary_root/entry['module']/'build'/entry['file']).read_bytes()
 assert digest(data)==entry['sha256'],entry['file']
 payload['Plug-ins/'+entry['file']]=data
assert len(payload)==count==len(json.loads((root/'plugins.json').read_text('utf8')))
for file,target in [('LICENSE','LICENSE'),('THIRD_PARTY.md','THIRD_PARTY.md'),('third_party/imgui/LICENSE.txt','Dear-ImGui-LICENSE.txt')]:payload[target]=(root/file).read_bytes()
payload['RELEASE-NOTES.md']=(root/'docs'/f'RELEASE_{release}.md').read_bytes()
payload['README.txt']=(f'AE-to-PS-Plugins {release} / stable / Windows x64 / {count} native Photoshop filters\n\n'
 '退出 Photoshop 并备份同名文件。将所需的 .8bf 放入 Photoshop 的 Plug-ins 目录，重启后从滤镜菜单的 ZaYu- 分组打开。需要再次编辑参数时，先把图层转为智能对象。\n\n'
 'Quit Photoshop and back up existing copies. Copy the required .8bf files into its Plug-ins directory, restart, and look for the ZaYu- groups in the Filter menu. Use Smart Objects to keep the filter settings editable.\n\n'
 '版本改动与验证范围见 RELEASE-NOTES.md。 See RELEASE-NOTES.md for changes and tested versions.\n'
 f'{project["repository"]}\n').encode('utf-8-sig')
manifest={'release':release,'channel':'stable','effects':count,'plugins':validation['plugins'],'source_archive_sha256':digest(source.read_bytes()),'validation':validation['release_validation'],'files':{file:digest(data)for file,data in payload.items()}}
payload['manifest.json']=(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n').encode('utf8')
archive=root/'dist'/f'{name}.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6)as z:
 for file,data in sorted(payload.items()):z.writestr(name+'/'+file,data)
with zipfile.ZipFile(archive)as z:
 assert z.testzip()is None and sum(n.endswith('.8bf')for n in z.namelist())==count
 for file,data in payload.items():assert z.read(name+'/'+file)==data
assets=[archive,source]
if args.screenshots:
 gallery=root/'docs/assets';captures=json.loads((gallery/'gallery-manifest.json').read_text('utf-8-sig'))
 assert len(captures)==count and len({row['slug'] for row in captures})==count
 screenshot_zip=root/'dist'/f'AE-to-PS-Plugins-{release}-screenshots.zip'
 prefix=screenshot_zip.stem+'/'
 pictures={prefix+row['slug']+'.png':(root/'docs'/row['image']).read_bytes() for row in captures}
 pictures[prefix+'LICENSE-NOTICE.md']=(gallery/'README.md').read_bytes()
 with zipfile.ZipFile(screenshot_zip,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for path,data in sorted(pictures.items()):z.writestr(path,data)
 with zipfile.ZipFile(screenshot_zip) as z:
  assert z.testzip() is None and all(z.read(path)==data for path,data in pictures.items())
 assets.append(screenshot_zip)
checks=root/'dist'/f'SHA256SUMS-{release}.txt'
checks.write_text(''.join(digest(file.read_bytes())+'  '+file.name+'\n'for file in assets),'ascii');assets.append(checks)
report={'status':'PASS','release':release,'plugins':count,'files_verified':len(payload),'assets':[{'name':file.name,'bytes':file.stat().st_size,'sha256':digest(file.read_bytes())}for file in assets]}
(out/'package-check.json').write_text(json.dumps(report,indent=2)+'\n','utf8')
print(f'PASS: {count} verified binaries and {len(assets)} release assets')
