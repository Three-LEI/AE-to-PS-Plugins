"""Compile owned JSON metadata to legacy Windows PiPL/aete using the SDK tool.

The JSON remains the single metadata source. Cnvtpipl.exe is obtained separately
with the Adobe SDK and is never copied into the source or runtime distribution.
"""
from pathlib import Path
import argparse,ast,json,subprocess,re,struct
parser=argparse.ArgumentParser();parser.add_argument('--metadata',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--sdk',type=Path,required=True);args=parser.parse_args()
item=json.loads(args.metadata.read_text(encoding='utf-8-sig'))['PiPLs'][0]
localized_strings={}
def string(value,localized=False):
 data=str(value).encode('cp936');assert len(data)<256
 if localized and not str(value).isascii():
  token='__AEPS_STRING_'+str(len(localized_strings))+'__';localized_strings[token.encode('ascii')]=data;return '"'+token+'"'
 assert str(value).isascii() and not any(c in data for c in (0,10,13,34,92)),'nonlocalized metadata must be ASCII'
 return '"'+str(value)+'"'
def kind(value):assert len(value)==4 and value.isascii();return "'"+value+"'"
mode_names=['Bitmap','GrayScale','IndexedColor','RGBColor','CMYKColor','HSLColor','HSBColor','Multichannel','Duotone','LABColor']
parts=['Kind { Filter }','Name { '+string(item['Name'])+' }','Category { '+string(item['Category'])+' }','Version { '+str((item['Version']<<16)|item.get('SubVersion',0))+' }','CodeWin64X86 { '+string(item['CodeEntryPointWin64'])+' }']
parts+=['SupportedModes { '+', '.join(('doesSupport'if item['SupportedModes'].get(name,False)else'no')+name for name in mode_names)+' }','EnableInfo { '+string(item['EnableInfo'])+' }']
version=(item.get('ComponentVersionShortNum',0)<<16)|(item.get('ComponentVersionMinorRevNum',0)<<8)|item.get('ComponentVersionDotRevNum',0)
parts+=['Component { '+str(version)+', '+string(item['ComponentName'])+' }']
if item.get('MonitorScalingAware'):parts+=['MonitorScalingAware { true }']
if item.get('FilterLayerSupport'):parts+=['FilterLayerSupport { doesSupportFilterLayers }']
if item.get('PlugInMaxSize'):parts+=['PlugInMaxSize { '+', '.join(map(str,item['PlugInMaxSize']))+' }']
cases=[]
for case in item['FilterCaseInfo']:
 values=[case['InputHandling'][0].lower()+case['InputHandling'][1:],case['OutputHandling'][0].lower()+case['OutputHandling'][1:]]
 for field,yes,no in [('WriteOutsideSelection','writeOutsideSelection','doNotWriteOutsideSelection'),('FiltersLayerMasks','filtersLayerMasks','doesNotFilterLayerMasks'),('WorksWithBlankData','worksWithBlankData','doesNotWorkWithBlankData'),('DontCopySourceToDestination','doNotCopySourceToDestination','copySourceToDestination')]:values.append(yes if case.get(field,False)else no)
 cases.append(', '.join(values))
assert len(cases)==7;parts+=['FilterCaseInfo { {\n'+',\n'.join(cases)+'\n} }']
if item.get('HasTerminology'):
 h=item['HasTerminology'];parts+=['HasTerminology { '+kind(h['ClassID'])+', '+kind(h['EventID'])+', 16000, "" }']
source="resource 'PiPL' (16000) { {\n"+',\n'.join(parts)+'\n} };\n'
def flags(optional,list_items=False,enumerated=False,changes=False):
 return ', '.join([optional,'listOfItems'if list_items else'singleItem','enumerated'if enumerated else'notEnumerated']+['changesState'if changes else'reserved']+['reserved']*12)
if item.get('Terminology'):
 term=item['Terminology'];suites=[]
 for suite in term['Suites']:
  events=[]
  for event in suite['Events']:
   direct=event.get('DirectParamFlagBits',{});params=[]
   for param in event['Parameters']:
    f=param.get('ParamFlagBits',{});assert not any(f.get(name,False)for name in ['LabeledParam','Feminine','Masculine','Plural'])
    params.append(', '.join([string(param['ParamName']),kind(param['ParamKeyword']),kind(param['ParamType']),string(param.get('ParamDescription','')),flags('optional'if f.get('Optional',True)else'required',f.get('ListOfItems',False),f.get('Enumerated',False))]))
   events.append(',\n'.join([string(event['EventName']),string(event.get('EventDescription',''),localized=True),kind(event['ClassID']),kind(event['EventType']),kind(event.get('ReplyType','null')),'""',flags('replyOptional'),kind(event.get('DirectParamType','#ImR')),'""',flags('directParamOptional'if direct.get('DirectParamOptional',False)else'directParamRequired',direct.get('ListOfItems',False),direct.get('Enumerated',False),direct.get('ChangesState',False)),'{\n'+',\n'.join(params)+'\n}']))
  suites.append(',\n'.join([string(suite['SuitName']),string(suite.get('SuitDescription','')),kind(suite['SuitID']),str(suite.get('SuitLevel',1)),str(suite.get('SuitVersion',1)),'{\n'+',\n'.join(events)+'\n}','{}','{}','{}']))
 source+="resource 'aete' (16000) {\n"+', '.join(str(term.get(key,default))for key,default in [('MajorVersion',1),('MinorVersion',0),('Language',0),('ScriptCode',0)])+',\n{\n'+',\n'.join(suites)+'\n}\n};\n'
args.output.parent.mkdir(parents=True,exist_ok=True);rez=args.output.with_suffix('.rr');rez.write_text(source,encoding='ascii')
tool=args.sdk/'samplecode/resources/Cnvtpipl.exe';assert tool.is_file(),'Adobe SDK Cnvtpipl.exe is required for legacy resources'
result=subprocess.run([str(tool),str(rez),str(args.output)],capture_output=True)
if result.returncode:raise SystemExit((result.stdout+result.stderr).decode('cp936','replace'))
# Resource source stays ASCII; non-ASCII metadata bytes must survive the UTF-8
# resource compiler without another codepage conversion.
raw=args.output.read_text(encoding='ascii');localized=[]
if localized_strings:
 # Cnvtpipl supplies the AETE layout. Replace only localized Pascal strings,
 # then emit raw DWORDs so llvm-rc cannot reinterpret the legacy CP936 bytes.
 def aete_bytes(match):
  body=re.sub(r'/\*.*?\*/','',match[2],flags=re.S);data=b''
  token_pattern=r'"(?:\\.|[^"\\])*"|0[xX][0-9a-fA-F]+|\d+'
  assert not re.sub(token_pattern,'',body).replace(',','').strip(),'unsupported SDK AETE token'
  for token in re.findall(token_pattern,body):
   if token.startswith('"'):
    value=ast.literal_eval(token).encode('latin1')
    if value[1:]in localized_strings:
     value=localized_strings[value[1:]];value=bytes([len(value)])+value
    data+=value
   else:data+=struct.pack('<H',int(token,16 if token.lower().startswith('0x')else 10))
  data+=b'\0'*(-len(data)%4)
  words=',\n'.join('0x%08XL'%int.from_bytes(data[i:i+4],'little')for i in range(0,len(data),4))
  return match[1]+'\n'+words+match[3]
 raw,count=re.subn(r'(16000\s+aete\s+DISCARDABLE\s+BEGIN)(.*?)(\nEND)',aete_bytes,raw,flags=re.S);assert count==1
for field,key in [('ZStringName','znam'),('ZStringCategory','zcat')]:
 if not item.get(field):continue
 value=item[field].encode('cp936');assert len(value)<256
 payload=bytes([len(value)])+value+b'\0';payload+=b'\0'*(-len(payload)%4)
 literal=', '.join('0x%08XL'%int.from_bytes(payload[i:i+4],'little')for i in range(0,len(payload),4))
 localized.append('"MIB8", "'+key[::-1]+'", 0L, '+str(len(payload))+'L,\n'+literal+',\n')
if localized:
 raw=re.sub(r'(\d+)L, /\* Property count \*/',lambda m:str(int(m[1])+len(localized))+'L, /* Property count */',raw,count=1)
 raw=raw.replace('\nEND','\n'+''.join(localized)+'END',1)
args.output.write_text(raw,encoding='ascii')
print('PASS: legacy PiPL'+('/aete'if item.get('Terminology')else'')+' metadata: '+item['Name'])
