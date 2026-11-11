"""Publish verified dist assets using the existing Git credential helper session.

Credentials stay in process memory and are sent only to GitHub's documented API.
API reference: https://docs.github.com/en/rest/releases/releases
"""
from pathlib import Path
import argparse,hashlib,json,os,subprocess,urllib.error,urllib.parse,urllib.request
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--commit',required=True);parser.add_argument('--asset',type=Path,action='append',required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
project=json.loads((root/'project.json').read_text(encoding='utf-8'));owner,repo=project['repository'].rstrip('/').split('/')[-2:];base='https://api.github.com/repos/'+owner+'/'+repo;tag='v'+project['release']
assets=[]
for path in args.asset:
 path=path.resolve();assert path.is_relative_to(root/'dist')and path.is_file()and path.suffix in {'.zip','.txt'};assets.append(path)
env=os.environ.copy();env['GCM_INTERACTIVE']='Never';env['GIT_TERMINAL_PROMPT']='0'
credential=subprocess.run(['git','credential','fill'],input='protocol=https\nhost=github.com\nusername='+owner+'\n\n',capture_output=True,text=True,env=env)
if credential.returncode:raise SystemExit('Existing Git credential session unavailable; no new login or credential was created.')
fields=dict(line.split('=',1)for line in credential.stdout.splitlines()if'='in line);token=fields.get('password')
if not token:raise SystemExit('Existing Git credential helper returned no usable authentication.')
class NoRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,*args,**kwargs):return None
client=urllib.request.build_opener(NoRedirect())
def request(url,method='GET',data=None,content_type='application/json',missing=False):
 parsed=urllib.parse.urlsplit(url);assert parsed.scheme=='https'and parsed.hostname in {'api.github.com','uploads.github.com'}
 payload=json.dumps(data,ensure_ascii=False).encode('utf-8')if isinstance(data,dict)else data
 headers={'Accept':'application/vnd.github+json','Authorization':'Bearer '+token,'X-GitHub-Api-Version':'2026-03-10','User-Agent':'AE-to-PS-Plugins-release','Content-Type':content_type}
 try:
  with client.open(urllib.request.Request(url,payload,headers,method=method),timeout=60)as response:return json.load(response)
 except urllib.error.HTTPError as error:
  if error.code==404 and missing:return None
  raise RuntimeError('GitHub API HTTP '+str(error.code)+'; authentication details were not logged')from None
account=request('https://api.github.com/user');assert account['login'].lower()==owner.lower(),'Git account differs from repository owner'
release=request(base+'/releases/tags/'+tag,missing=True)
if release is None:
 release=request(base+'/releases','POST',{'tag_name':tag,'target_commitish':args.commit,'name':'0.34 · 十九款滤镜开源与开源地址链接','body':'首次公开整理十九款 Photoshop Windows x64 原生滤镜源码，按 MIT 许可发布。\n\n- 全部关于窗口改为蓝色可点击开源地址。\n- 提供统一构建入口、依赖说明、贡献说明及干净源码包。\n- 生成内核及参数兼容性保持0.33；55个PS效果／位深组合、113份导出像素一致。\n- 插件包普通使用无需 AE、SDK 或 Python；Tint仅RGB8，其余位深及限制见模块说明。\n\n下载插件整包或源码ZIP，校验值见 SHA256SUMS-0.34.txt。','draft':True,'prerelease':False})
assert release['target_commitish']==args.commit,'existing release targets a different commit'
release_id=release['id'];assert isinstance(release_id,int)and release_id>0
uploaded={entry['name']:entry for entry in release['assets']};checks=[]
for path in assets:
 data=path.read_bytes();sha=hashlib.sha256(data).hexdigest();entry=uploaded.get(path.name)
 if entry is None:
  url='https://uploads.github.com/repos/'+owner+'/'+repo+'/releases/'+str(release_id)+'/assets?name='+urllib.parse.quote(path.name)
  entry=request(url,'POST',data,'application/zip'if path.suffix=='.zip'else'text/plain')
 assert entry['state']=='uploaded'and entry['size']==len(data)
 if entry.get('digest'):assert entry['digest']=='sha256:'+sha,'uploaded asset hash differs'
 else:
  with urllib.request.urlopen(entry['browser_download_url'],timeout=60)as response:assert hashlib.sha256(response.read()).hexdigest()==sha
 checks.append({'name':path.name,'bytes':len(data),'sha256':sha,'url':entry['browser_download_url']});print('Verified release asset: '+path.name,flush=True)
if release['draft']:release=request(base+'/releases/'+str(release_id),'PATCH',{'draft':False})
verified=request(base+'/releases/tags/'+tag);assert not verified['draft']and{a['name']for a in verified['assets']}=={p.name for p in assets}
report={'status':'PASS','release':project['release'],'repository':project['repository'],'commit':args.commit,'url':verified['html_url'],'assets':checks}
args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8');print('PASS: published '+verified['html_url'])
