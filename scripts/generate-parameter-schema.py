"""Generate typed field visitors and validate matching Photoshop terminology.

Field IDs are checked in explicitly; never derive them from field order or names.
--update-metadata is an explicit maintainer operation, not a normal build step.
"""
from pathlib import Path
import argparse,json
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--metadata',type=Path,required=True);parser.add_argument('--output',type=Path);parser.add_argument('--update-metadata',action='store_true');args=parser.parse_args()
catalog=json.loads((root/'native/common/parameter_fields.json').read_text(encoding='utf-8'))
metadata=args.metadata.resolve();relative=metadata.relative_to(root).as_posix();profile=catalog['profiles'].get(relative)
if not profile:raise SystemExit(0)
module=relative.split('/')[0];schema=catalog['modules'][module];fields=schema['fields'];assert len({f['id']for f in fields})==len(fields)
for f in fields:assert len(f['id'])==4 and f['id'].isascii()
document=json.loads(metadata.read_text(encoding='utf-8-sig'));item=document['PiPLs'][0]
params=[{'ParamName':'schemaVersion','ParamKeyword':'ZpSv','ParamType':'long','ParamDescription':'Parameter schema version'}, {'ParamName':'effectIdentity','ParamKeyword':'ZpId','ParamType':'long','ParamDescription':'Stable effect identity'}]
for f in fields:params.append({'ParamName':f['name'],'ParamKeyword':f['id'],'ParamType':{'number':'doub','integer':'long','flag':'bool','layer':'TEXT'}[f['kind']],'ParamDescription':f['name']})
has={'ClassID':'ZpEf','EventID':profile['event']}
term={'MajorVersion':1,'MinorVersion':0,'Language':0,'ScriptCode':0,'Suites':[{'SuitName':'AEPS Saved Parameters','SuitID':'ZpEf','SuitLevel':1,'SuitVersion':1,'SuitDescription':'Persistent native effect settings','Events':[{'EventName':item['Name'],'EventDescription':'Editable '+item['Name'],'ClassID':'ZpEf','EventType':profile['event'],'ReplyType':'null','DirectParamType':'#ImR','DirectParamFlagBits':{'DirectParamOptional':True,'Enumerated':True,'ChangesState':True},'Parameters':params}]}]}
if args.update_metadata:
 item['HasTerminology']=has;item['Terminology']=term;metadata.write_text(json.dumps(document,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
else:
 assert item.get('HasTerminology')==has and item.get('Terminology')==term,'Parameter metadata is stale; run this script with --update-metadata'
if args.output:
 text='#pragma once\n// Generated from native/common/parameter_fields.json; do not edit.\n#include "../../native/common/descriptor_parameters.hpp"\nnamespace psfilter::parameters {\n'
 text+='template<> struct Schema<'+schema['type']+'> {\n static constexpr bool available=true, unicode='+('true'if any(f['kind']=='layer'for f in fields)else'false')+';\n template<class Archive,class P>static void visit(Archive& a,P& p){\n'
 for f in fields:text+='  a.'+f['kind']+'("'+f['id']+'",p.'+f['member']+(',&p.'+f['transient']if f.get('transient')else'')+');\n'
 text+=' }\n};\n}\n';args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(text,encoding='utf-8')
