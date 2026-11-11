"""Small, dependency-free checks for the public source tree, never credential contents."""
from pathlib import Path
import hashlib,json,re,subprocess

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def source_files(root):
 try:
  result=subprocess.run(['git','-C',str(root),'ls-files','-c','-o','--exclude-standard','-z'],capture_output=True,check=True)
  return sorted({root/name for name in result.stdout.decode('utf-8').split('\0')if name and (root/name).is_file()})
 except (OSError,subprocess.CalledProcessError):
  excluded={'.git','build','downloads','dist','publish','evidence','backups','__pycache__'}
  return sorted(p for p in root.rglob('*')if p.is_file()and not excluded.intersection(p.relative_to(root).parts)and p.name!='build.local.json')

def check(root):
 root=Path(root).resolve();project=json.loads((root/'project.json').read_text(encoding='utf-8'));plugins=json.loads((root/'plugins.json').read_text(encoding='utf-8'))
 assert len(plugins)==19 and len({p['file']for p in plugins})==19,'nineteen distinct effects required'
 assert len(project['modules'])==13 and {p['module']for p in plugins}==set(project['modules'])
 url=project['repository'];assert re.fullmatch(r'https://github\.com/[A-Za-z0-9-]+/[A-Za-z0-9_.-]+',url)
 assert url in (root/'native/ui/include/psui/source_repository.hpp').read_text(encoding='utf-8'),'About URL differs from project metadata'
 assert 'MIT License' in (root/'LICENSE').read_text(encoding='utf-8')
 lock=json.loads((root/'dependencies.lock.json').read_text(encoding='utf-8'))
 for name,sha in lock['imgui']['files'].items():assert digest(root/'third_party/imgui'/name)==sha,('vendored ImGui changed',name)
 assert lock['photoshop_sdk']['bundled'] is False
 files=source_files(root);issues=[]
 banned={'.8bf','.exe','.dll','.lib','.obj','.o','.a','.ttf','.ttc','.otf','.psd','.psb','.aep','.zip','.pem','.key'}
 private=re.compile(rb'(?<![A-Za-z0-9])[A-Za-z]:[/\\]+(?:Users[/\\]+|[^ \r\n"\']*(?:codex|Adobe))',re.I)
 secret=re.compile(rb'(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{30,}|-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----)')
 for path in files:
  rel=path.relative_to(root).as_posix()
  if path.suffix.lower()in banned or path.name in {'PIFilter.h','PIActions.h','SPBasic.h','build.local.json'}:issues.append((rel,'non-source or local dependency'))
  if path.stat().st_size>5_000_000:issues.append((rel,'oversized source file'))
  data=path.read_bytes()
  if private.search(data):issues.append((rel,'workstation path'))
  if secret.search(data):issues.append((rel,'credential pattern'))
  if path.suffix in {'.hpp','.cpp','.inc'}:
   for include in re.findall(rb'^\s*#include\s*"((?:\.\.?/)[^"]+)"',data,re.M):
    if not (path.parent/include.decode('utf-8')).resolve().exists():issues.append((rel,'missing relative include'))
 assert not issues,issues
 return {'status':'PASS','project':project['name'],'repository':url,'effects':19,'modules':13,'source_files':len(files),'source_bytes':sum(p.stat().st_size for p in files),'checks':['MIT root license','nineteen-effect catalog','shared About URL','pinned ImGui hashes','no bundled SDK/fonts/binaries','no original workstation paths or credential patterns','relative C++ include closure']}
