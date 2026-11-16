"""Validate the public effect gallery without image-processing dependencies."""
from pathlib import Path
import argparse,json,re,struct,urllib.parse
p=argparse.ArgumentParser();p.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);args=p.parse_args();root=args.root.resolve()
manifest=json.loads((root/'docs/assets/gallery-manifest.json').read_text('utf-8-sig'));parameters=json.loads((root/'docs/gallery-parameters.json').read_text('utf8'));plugins=json.loads((root/'plugins.json').read_text('utf8'))
assert len(manifest)==len(parameters)==len(plugins)==23
assert {r['file'] for r in manifest}=={r['file'] for r in plugins};assert {r['slug'] for r in manifest}=={r['slug'] for r in parameters}
def jpeg_size(data):
 assert data[:2]==b'\xff\xd8';at=2
 while at<len(data):
  assert data[at]==255;marker=data[at+1];at+=2;size=struct.unpack_from('>H',data,at)[0]
  if marker in [192,193,194]:height,width=struct.unpack_from('>HH',data,at+3);return width,height
  at+=size
 raise AssertionError('JPEG size missing')
for row in manifest:
 file=root/'docs'/row['image'];data=file.read_bytes();assert jpeg_size(data)==(1024,616) and len(data)<1_000_000
hero=(root/'docs/assets/social-preview.jpg').read_bytes();assert jpeg_size(hero)==(1280,640) and len(hero)<1_000_000
photo=(root/'docs/assets/demo-source.png').read_bytes();assert photo[:8]==b'\x89PNG\r\n\x1a\n' and struct.unpack_from('>II',photo,16)==(768,768)
at=8
while at<len(photo):
 size=struct.unpack_from('>I',photo,at)[0];kind=photo[at+4:at+8];assert kind not in {b'eXIf',b'tEXt',b'iTXt',b'zTXt'};at+=size+12
assert 'MIT' in (root/'docs/assets/README.md').read_text('utf8')
events=set()
for module in json.loads((root/'project.json').read_text('utf8'))['modules']:
 for file in (root/module/'src').glob('*PiPL*.json'):events.add(json.loads(file.read_text('utf8'))['PiPLs'][0]['HasTerminology']['EventID'])
assert len(events)==23 and {r['event'] for r in parameters}==events
for name in ['README.md','README.en.md','docs/GALLERY.md','docs/EFFECTS.md','docs/PERFORMANCE.md','docs/RELEASE_NOTES.md','docs/RELEASE_0.37.md','docs/VALIDATION.md','docs/assets/README.md']:
 file=root/name;text=file.read_text('utf8');links=re.findall(r'\[[^\]]*\]\(([^)]+)\)|\bsrc="([^"]+)"',text)
 for pair in links:
  link=next(s for s in pair if s).strip('<>');parsed=urllib.parse.urlsplit(link)
  if parsed.scheme or not parsed.path:continue
  assert (file.parent/urllib.parse.unquote(parsed.path)).resolve().exists(),(name,link)
assert (root/'docs/GALLERY.md').read_text('utf8').count('before and after native Photoshop plugin output]')==23
print('PASS: 23 gallery cards/settings/events, image sizes, privacy metadata, asset notice and primary documentation links')
