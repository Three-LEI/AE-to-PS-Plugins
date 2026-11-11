"""Check logical field coverage independently of the generated visitor."""
from pathlib import Path
import json,re
root=Path(__file__).resolve().parents[1];catalog=json.loads((root/'native/common/parameter_fields.json').read_text(encoding='utf-8'))
def body(text,name):
    at=text.index('{',text.index('struct '+name));depth=1;end=at+1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[at+1:end-1]
def split(text):
    result=[];start=0;depth=0
    for i,c in enumerate(text):
        depth+=(c in '<{[')-(c in '>}]')
        if c==','and depth==0:result.append(text[start:i]);start=i+1
    return result+[text[start:]]
for module,schema in catalog['modules'].items():
    text=next((root/module/'include').glob('*core.hpp')).read_text(encoding='utf-8');source=re.sub(r'//[^\n]*','',body(text,'Parameters'))
    members=set()
    for declaration in source.split(';'):
        if not declaration.strip()or'operator'in declaration:continue
        for item in split(declaration):
            item=re.split(r'[={]',item)[0];item=re.sub(r'\[\d+\]','',item).strip();match=re.search(r'(\w+)$',item)
            assert match,(module,item);members.add(match[1])
    fields=schema['fields'];covered={re.split(r'[.\[]',f['member'])[0]for f in fields}|{f['transient']for f in fields if 'transient'in f}
    assert members==covered,(module,'uncovered',members-covered,'unknown',covered-members)
    for f in fields:assert f['id']not in {'ZpSv','ZpId'}
    if module=='chromatic':
        for i in range(3):
            assert {f['member'].split('].')[1]for f in fields if f['member'].startswith('channels['+str(i)+']')}=={'amount','x','y','scale','rotation','blur'}
print('PASS: all parameter members covered; three complete channel records; stable IDs without reserved-key collisions')
