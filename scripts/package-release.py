"""Package a validated stable snapshot; never performs network or Git writes."""
from pathlib import Path
import argparse,hashlib,json,re,zipfile

root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--report-dir',type=Path,required=True);args=p.parse_args()
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
 data=(root/entry['module']/'build'/entry['file']).read_bytes()
 assert digest(data)==entry['sha256'],entry['file']
 payload['Plug-ins/'+entry['file']]=data
assert len(payload)==count==len(json.loads((root/'plugins.json').read_text('utf8')))
for file,target in [('LICENSE','LICENSE'),('THIRD_PARTY.md','THIRD_PARTY.md'),('third_party/imgui/LICENSE.txt','Dear-ImGui-LICENSE.txt')]:payload[target]=(root/file).read_bytes()
payload['RELEASE-NOTES.md']=(root/'docs'/f'RELEASE_{release}.md').read_bytes()
payload['README.txt']=(f'AE-to-PS-Plugins {release} / stable / Windows x64 / {count} native Photoshop filters\n\n'
 '安装：退出Photoshop，备份同名文件，将Plug-ins中所需.8bf放入64位Photoshop的Plug-ins目录，然后重启。在“滤镜 → ZaYu-AE 效果复刻”中打开。避免不同目录存在重复副本。\n'
 '需要可编辑参数时先转智能对象，再应用效果并保存PSD。普通像素层直接应用仅产生像素结果。\n'
 '普通使用无需AE、原版插件、Adobe SDK或Python。Windows和Photoshop均需64位。\n\n'
 '本轮PS2020 21.2.9验证新增效果和性能改动；其余21款原有程序与0.37一致。PS2026仅安装；Win7/CC2018保留旧系统路径，仍需外机复测。\n'
 'CPU计算，GPU用于UI和预览显示。独立近似实现，不保证与原版逐像素一致。ASCII字体受系统影响。Lens Flare从正式版起采用版本2元素快照，早期未发布试验文件不承诺回读。\n'
 '内部组件编号沿用各自开发编号，整包正式版以本Release为准。详细改动与边界见RELEASE-NOTES.md。\n\n'
 'Install: quit Photoshop, back up previous files, copy the required .8bf files into Photoshop\'s Plug-ins directory and restart. Both Windows and Photoshop must be64-bit. Use Smart Objects for editable parameters. No AE, original plugins, SDK or Python is needed.\n'
 'PS2020 21.2.9 validation covers additions and changed effects;21 existing binaries are unchanged from0.37. PS2026 is installed-only for this round; Win7/CC2018 needs external retesting.\n\n'
 'Project: https://github.com/Three-LEI/AE-to-PS-Plugins\n'
 'Gallery: https://github.com/Three-LEI/AE-to-PS-Plugins/blob/main/docs/GALLERY.md\n'
 'Validation: https://github.com/Three-LEI/AE-to-PS-Plugins/blob/main/docs/VALIDATION.md\n'
 '代码MIT；照片演示素材不属于代码MIT范围。本程序包不含演示照片、SDK、字体或个人PSD。\n').encode('utf-8-sig')
manifest={'release':release,'channel':'stable','effects':count,'plugins':validation['plugins'],'source_archive_sha256':digest(source.read_bytes()),'validation':validation['release_validation'],'files':{file:digest(data)for file,data in payload.items()}}
payload['manifest.json']=(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n').encode('utf8')
archive=root/'dist'/f'{name}.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6)as z:
 for file,data in sorted(payload.items()):z.writestr(name+'/'+file,data)
with zipfile.ZipFile(archive)as z:
 assert z.testzip()is None and sum(n.endswith('.8bf')for n in z.namelist())==count
 for file,data in payload.items():assert z.read(name+'/'+file)==data
assets=[archive,source];checks=root/'dist'/f'SHA256SUMS-{release}.txt'
checks.write_text(''.join(digest(file.read_bytes())+'  '+file.name+'\n'for file in assets),'ascii');assets.append(checks)
report={'status':'PASS','release':release,'plugins':count,'files_verified':len(payload),'assets':[{'name':file.name,'bytes':file.stat().st_size,'sha256':digest(file.read_bytes())}for file in assets]}
(out/'package-check.json').write_text(json.dumps(report,indent=2)+'\n','utf8')
print(f'PASS: {count} verified binaries, source ZIP and SHA256SUMS; no publication')
