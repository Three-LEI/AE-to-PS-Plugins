"""Generate typed screenshot settings from the existing demo parameter records."""
from pathlib import Path
import argparse,json
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);args=p.parse_args();root=Path(__file__).resolve().parents[1];args.output.mkdir(parents=True,exist_ok=True)
rows=json.loads((root/'docs/gallery-parameters.json').read_text('utf8'));manifest=json.loads((root/'docs/assets/gallery-manifest.json').read_text('utf-8-sig'));catalog=json.loads((root/'native/common/parameter_fields.json').read_text('utf8'));plugins=json.loads((root/'plugins.json').read_text('utf8'))
by_slug={r['slug']:r for r in manifest};by_file={e['file']:e for e in plugins}
surface={'property':5,'softness':4,'cut':21,'height':35,'displacement':55,'light_type':0,'intensity':100,'light_height':65,'light_x':17,'light_y':29,'direction':-45,'ambient':60,'diffuse':45,'specular':55,'roughness':.06,'metal':30,'light_color[0]':1,'light_color[1]':1,'light_color[2]':1}
creative={'type':0,'pinning':0,'edge':1,'shape':0,'reception':0,'mirroring':5,'quality':1,'cycle':True,'floating':False,'cx':384,'cy':330,'amount':35,'size':90,'complexity':2,'evolution':0,'cycles':2,'seed':23,'horizontal':85,'vertical':117,'mix':100,'direction':14,'width':100,'intensity':85,'edge_intensity':50,'thickness':3,'rotation':0,'color[0]':1,'color[1]':.9,'color[2]':.7}
roughen={'type':0,'border':26,'sharpness':1.25,'influence':1,'scale':64,'stretch':-28,'offset_x':-3,'offset_y':5,'complexity':2.5,'evolution':37.5,'cycles':2,'seed':23,'mix':100,'cycle':False,'color[0]':.9,'color[1]':.3,'color[2]':.15}
for row in rows:
 entry=by_file[by_slug[row['slug']]['file']];module=entry['module'];lines=['#pragma once','inline void configure_gallery(effect::Parameters& p,const std::vector<psui::HostLayer>& layers){']
 if row.get('legacy'):
  values=dict({'surface':surface,'creative':creative,'roughen':roughen}[row['legacy']])
  if row['slug']=='turbulent-displace':values.update(amount=85,size=70)
 else:
  fields={f['id']:f for f in catalog['modules'][module]['fields']};values={}
  for f in row['fields']:
   if f['id'] in fields and 'member' in fields[f['id']]:values[fields[f['id']]['member']]=f['value']
 for member,value in values.items():
  if isinstance(value,str):literal='layers[1].name_key' if value else '0'
  elif isinstance(value,bool):literal='true' if value else 'false'
  else:literal=repr(value)
  lines.append(f' p.{member}=static_cast<std::remove_cvref_t<decltype(p.{member})>>({literal});')
 if module=='displacement':lines.append(' p.layer_id=layers[1].id;')
 lines+=['}','inline constexpr const char* gallery_slug="'+row['slug']+'";']
 if module=='energy':
  data=bytes.fromhex(row['geometry']);lines+=['inline void configure_geometry(energy::Inputs& inputs){',' if(!energy::decode_path(std::vector<std::uint8_t>{'+','.join(map(str,data))+'},inputs.path))throw std::runtime_error("invalid gallery path");',' inputs.name="演示椭圆路径";','}']
 (args.output/(row['slug']+'.hpp')).write_text('\n'.join(lines)+'\n','utf8')
print(f'PASS: generated typed settings for {len(rows)} production parameter windows')
