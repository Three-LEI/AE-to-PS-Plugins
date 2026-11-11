"""Validate compiled legacy host registration against canonical JSON metadata."""
import struct
from inspect_binaries import resources

def pipl(data):
    assert struct.unpack_from('<HI',data)==(1,0),'PiPL marker/version'
    count=struct.unpack_from('<I',data,6)[0];at=10;result={}
    for _ in range(count):
        vendor,key,index,size=struct.unpack_from('<4s4sII',data,at);at+=16
        assert vendor==b'MIB8'and index==0 and size%4==0
        key=key[::-1].decode('ascii');assert key not in result and at+size<=len(data)
        result[key]=data[at:at+size];at+=size
    assert at==len(data);return result

class Reader:
    def __init__(self,data):self.data=data;self.at=0
    def take(self,n):
        result=self.data[self.at:self.at+n];assert len(result)==n;self.at+=n;return result
    def word(self):return int.from_bytes(self.take(2),'little')
    def text(self):return self.take(self.take(1)[0]).decode('cp936')
    def code(self):return self.take(4)[::-1].decode('ascii')

def flags(item,optional='Optional'):
    return sum(bit for key,bit in [(optional,0x8000),('ListOfItems',0x4000),('Enumerated',0x2000),('ChangesState',0x1000)]if item.get(key,key=='Optional'))

def terminology(data,item):
    r=Reader(data);assert r.word()==1
    assert r.word()==item.get('MajorVersion',1)|(item.get('MinorVersion',0)<<8)
    assert r.word()==item.get('Language',0)and r.word()==item.get('ScriptCode',0)
    suites=item['Suites'];assert r.word()==len(suites);total=0
    for suite in suites:
        assert r.text()==suite['SuitName']and r.text()==suite.get('SuitDescription','')and r.code()==suite['SuitID']
        assert r.word()==suite.get('SuitLevel',1)and r.word()==suite.get('SuitVersion',1)
        events=suite['Events'];assert r.word()==len(events)
        for event in events:
            assert r.text()==event['EventName']and r.text()==event.get('EventDescription','')
            assert r.code()==event['ClassID']and r.code()==event['EventType']
            assert r.code()==event.get('ReplyType','null')and r.text()==''and r.word()==0x8000
            assert r.code()==event.get('DirectParamType','#ImR')and r.text()==''
            assert r.word()==flags(event.get('DirectParamFlagBits',{}),'DirectParamOptional')
            parameters=event['Parameters'];assert r.word()==len(parameters)
            for param in parameters:
                assert r.text()==param['ParamName']and r.code()==param['ParamKeyword']and r.code()==param['ParamType']
                assert r.text()==param.get('ParamDescription','')and r.word()==flags(param.get('ParamFlagBits',{}))
            total+=len(parameters)
        assert (r.word(),r.word(),r.word())==(0,0,0),'unexpected classes/comparisons/enums'
    assert len(data)-r.at<4 and not any(data[r.at:]);return total

def validate(binary,item):
    embedded=resources(binary);assert not any(key[0]=='JSON'for key in embedded),'binary-only registration expected'
    raw=[data for key,data in embedded.items()if key[:2]==('PIPL',16000)];assert len(raw)==1
    props=pipl(raw[0]);text=lambda key:props[key].split(b'\0',1)[0].decode('ascii')
    pstring=lambda key:props[key][1:1+props[key][0]]
    assert props['kind']==b'MFB8'and text('8664')==item['CodeEntryPointWin64']
    for field,key in [('Name','name'),('Category','catg'),('ZStringName','znam'),('ZStringCategory','zcat')]:
        if field in item:assert pstring(key)==item[field].encode('cp936'),field
    assert struct.unpack('<I',props['vers'])[0]==item['Version']<<16|item.get('SubVersion',0)
    # All nineteen effects are RGB-only. The SDK FlagSet places RGB in bit 4
    # of its first byte (the first mode occupies the most significant bit).
    assert {name for name,value in item['SupportedModes'].items()if value}=={'RGBColor'}
    assert struct.unpack('<I',props['mode'])[0]==0x10
    assert text('enbl')==item['EnableInfo']
    version=(item.get('ComponentVersionShortNum',0)<<16)|(item.get('ComponentVersionMinorRevNum',0)<<8)|item.get('ComponentVersionDotRevNum',0)
    assert struct.unpack_from('<I',props['cmpt'])[0]==version and props['cmpt'][4:].split(b'\0',1)[0].decode('ascii')==item['ComponentName']
    assert struct.unpack('<I',props['pmsa'])[0]==int(item['MonitorScalingAware'])
    assert struct.unpack('<I',props['flly'])[0]==0x80
    if 'PlugInMaxSize'in item:assert list(struct.unpack('<II',props['ms32']))==item['PlugInMaxSize']
    handling={'CantFilter':0,'StraightData':1,'BlackMat':2,'GrayMat':3,'WhiteMat':4,'Defringe':5,'BlackZap':6,'GrayZap':7,'WhiteZap':8,'FillMask':9,'BackgroundZap':10,'ForegroundZap':11}
    expected=b''
    for case in item['FilterCaseInfo']:
        f=sum(1<<bit for field,bit in [('DontCopySourceToDestination',0),('WorksWithBlankData',1),('FiltersLayerMasks',2),('WriteOutsideSelection',3)]if case.get(field,False))
        expected+=bytes([handling[case['InputHandling'][2:]],handling[case['OutputHandling'][3:]],f,0])
    assert props['fici']==expected,'filter alpha/selection case flags changed'
    terms=0
    if 'HasTerminology'in item:
        h=item['HasTerminology'];data=props['hstm']
        assert struct.unpack_from('<I',data)[0]==0 and data[4:8][::-1].decode('ascii')==h['ClassID']and data[8:12][::-1].decode('ascii')==h['EventID']
        assert struct.unpack_from('<H',data,12)[0]==16000
        raw=[data for key,data in embedded.items()if key[:2]==('AETE',16000)];assert len(raw)==1
        terms=terminology(raw[0],item['Terminology'])
    return {'binary_registration':'PiPL','properties':len(props),'terminology_parameters':terms,'metadata_preserved':True}
