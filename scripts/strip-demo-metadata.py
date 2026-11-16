"""Remove PNG text/EXIF metadata while preserving pixel and color chunks."""
from pathlib import Path
import argparse,struct,zlib
p=argparse.ArgumentParser();p.add_argument('image',type=Path);args=p.parse_args()
data=args.image.read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n';position=8;clean=data[:8]
allowed={b'IHDR',b'PLTE',b'IDAT',b'IEND',b'tRNS',b'iCCP',b'sRGB',b'gAMA',b'cHRM',b'pHYs'}
while position<len(data):
 size=struct.unpack_from('>I',data,position)[0];kind=data[position+4:position+8];chunk=data[position:position+size+12];assert len(chunk)==size+12
 assert struct.unpack_from('>I',chunk,len(chunk)-4)[0]==zlib.crc32(chunk[4:-4])&0xffffffff
 if kind in allowed:clean+=chunk
 position+=size+12
assert position==len(data);args.image.write_bytes(clean)
print('PASS: PNG pixel/color chunks preserved; text and EXIF metadata removed')
