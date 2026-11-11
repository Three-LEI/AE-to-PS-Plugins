import argparse,json
from pathlib import Path
from source_checks import check
parser=argparse.ArgumentParser();parser.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1]);parser.add_argument('--report',type=Path);args=parser.parse_args()
report=check(args.root)
if args.report:args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('PASS: '+str(report['source_files'])+' source files; nineteen effects; source/URL/dependency checks')
