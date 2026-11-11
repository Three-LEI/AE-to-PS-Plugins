"""Read uncompressed Photoshop QA sample bytes, ignoring changing TIFF metadata."""
import struct,math

def tiff(path):
 # Photoshop QA exports are uncompressed chunky RGB/RGBA TIFFs. Compare actual
 # sample bytes, not file metadata (timestamps/history can legitimately differ).
 data=path.read_bytes();endian='<' if data[:2]==b'II' else '>'
 def unpack(fmt,offset):return struct.unpack_from(endian+fmt,data,offset)
 assert unpack('H',2)[0]==42;directory=unpack('I',4)[0];tags={}
 for i in range(unpack('H',directory)[0]):
  entry=directory+2+i*12;tag,kind,count=unpack('HHI',entry)
  if kind not in (1,3,4):continue
  fmt,size={1:('B',1),3:('H',2),4:('I',4)}[kind];offset=entry+8 if size*count<=4 else unpack('I',entry+8)[0]
  tags[tag]=unpack(fmt*count,offset)
 assert tags[259]==(1,) and tags.get(284,(1,))==(1,)
 w,h,n,depth=tags[256][0],tags[257][0],tags[277][0],tags[258][0]
 assert depth in (8,16,32) and n in (3,4) and len(set(tags[258]))==1
 raw=b''.join(data[o:o+c] for o,c in zip(tags[273],tags[279]));assert len(raw)==w*h*n*(depth//8)
 if depth==32:
  assert set(tags[339])=={3} and all(math.isfinite(v[0]) for v in struct.iter_unpack(endian+'f',raw))
 return endian,w,h,n,depth,raw
