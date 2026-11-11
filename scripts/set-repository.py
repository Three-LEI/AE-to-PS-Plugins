"""Update the project metadata and compiled About link for a GitHub fork."""
import argparse,json,re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('url');args=parser.parse_args()
url=args.url.rstrip('/')
if url.endswith('.git'):url=url[:-4]
if not re.fullmatch(r'https://github\.com/[A-Za-z0-9][A-Za-z0-9-]*/[A-Za-z0-9_.-]+',url):parser.error('Use a public HTTPS GitHub repository URL without credentials, query, or fragment.')
path=root/'project.json';data=json.loads(path.read_text(encoding='utf-8'));data['repository']=url;data['name']=url.rsplit('/',1)[1]
path.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(root/'native/ui/include/psui/source_repository.hpp').write_text('#pragma once\n// Updated with scripts/set-repository.py when publishing a fork.\nnamespace psui {\ninline constexpr const char *source_repository = '+json.dumps(url)+';\n}\n',encoding='utf-8')
print('Updated repository metadata and About link; rebuild plugins to apply.')
