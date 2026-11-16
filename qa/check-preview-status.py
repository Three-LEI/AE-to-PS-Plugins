from pathlib import Path
import json
import struct

root=Path(__file__).resolve().parents[1]
p=root/'evidence/ui-buttons-v01/overlay'
data=(p/'busy.bmp').read_bytes();assert data[:2]==b'BM'
offset=struct.unpack_from('<I',data,10)[0]
size,w,h,planes,bits,compression=struct.unpack_from('<IiiHHI',data,14)
assert planes==1 and bits in (24,32) and compression==0
stride=((w*bits+31)//32)*4
def pixel(x,y):
 row=h-1-y if h>0 else y
 b,g,r=data[offset+row*stride+x*(bits//8):][:3]
 return r,g,b
samples=json.loads((p/'samples.json').read_text())
colors=[(0,0,0),(255,255,255),(128,128,128),(200,80,10)]
rows=[]
for sample,color in zip(samples,colors):
 a=sample['coverage']/255
 expected=tuple(round(v+a*(255-2*v)) for v in color)
 actual=pixel(sample['x'],sample['y'])
 error=max(abs(x-y) for x,y in zip(expected,actual))
 assert error<=3,(color,sample,expected,actual)
 rows.append({'background':color,'coverage':sample['coverage'],'expected':expected,'actual':actual,'maximum_error':error})
(p/'pixels.json').write_text(json.dumps({'status':'PASS','samples':rows},indent=2)+'\n')
print('PASS four GPU pixel oracles for antialiased white Difference text; black/white/gray/color backgrounds')
