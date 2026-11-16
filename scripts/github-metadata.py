"""Inspect or apply repository discovery metadata using existing Git credentials."""
from pathlib import Path
import argparse,json,os,re,subprocess,urllib.request,urllib.error
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--apply',action='store_true');p.add_argument('--report',type=Path,required=True);args=p.parse_args()
project=json.loads((root/'project.json').read_text('utf8'));match=re.fullmatch(r'https://github\.com/([A-Za-z0-9-]+)/([A-Za-z0-9_.-]+)',project['repository']);assert match
owner,repo=match.groups();base=f'https://api.github.com/repos/{owner}/{repo}'
env=os.environ.copy();env.update(GCM_INTERACTIVE='Never',GIT_TERMINAL_PROMPT='0')
r=subprocess.run(['git','credential','fill'],input=f'protocol=https\nhost=github.com\nusername={owner}\n\n',capture_output=True,text=True,env=env)
fields=dict(line.split('=',1) for line in r.stdout.splitlines() if '=' in line);token=fields.get('password')
if r.returncode or not token:raise SystemExit('Existing Git credential session unavailable; no credential was created.')
class NoRedirect(urllib.request.HTTPRedirectHandler):
 def redirect_request(self,*args,**kwargs):return None
client=urllib.request.build_opener(NoRedirect());scopes=''
def request(url,method='GET',data=None):
 global scopes
 assert url.startswith('https://api.github.com/')
 payload=None if data is None else json.dumps(data,ensure_ascii=False).encode('utf8')
 headers={'Authorization':'Bearer '+token,'Accept':'application/vnd.github+json','Content-Type':'application/json','User-Agent':'AEPS-repository-metadata'}
 try:
  with client.open(urllib.request.Request(url,payload,headers,method=method),timeout=60) as response:
   scopes=response.headers.get('X-OAuth-Scopes',scopes);return json.load(response)
 except urllib.error.HTTPError as error:raise RuntimeError('GitHub API HTTP '+str(error.code)+'; authentication details omitted') from None
account=request('https://api.github.com/user');assert account['login'].lower()==owner.lower()
before=request(base);assert before['permissions']['push']
plan=json.loads((root/'docs/repository-metadata.json').read_text('utf8'))
assert len(plan['description'])<=350 and len(plan['topics'])<=20 and len(set(plan['topics']))==len(plan['topics'])
assert all(re.fullmatch(r'[a-z0-9][a-z0-9-]{0,49}',topic) for topic in plan['topics'])
if args.apply:
 request(base,'PATCH',{'description':plan['description'],'homepage':plan['homepage']})
 request(base+'/topics','PUT',{'names':plan['topics']})
after=request(base)
if args.apply:assert all(after[key]==plan[key] for key in ['description','homepage']) and sorted(after['topics'])==sorted(plan['topics'])
report={'status':'PASS','applied':args.apply,'repository':project['repository'],'account':account['login'],'author_email':str(account['id'])+'+'+account['login']+'@users.noreply.github.com','workflow_scope_available':'workflow' in [s.strip() for s in scopes.split(',')],
 'before':{key:before[key] for key in ['description','homepage','topics']},'after':{key:after[key] for key in ['description','homepage','topics']}}
args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf8')
print('PASS: '+('updated' if args.apply else 'inspected')+' repository metadata; authenticated owner '+account['login']+'; workflow scope '+str(report['workflow_scope_available']))
