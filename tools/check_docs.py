#!/usr/bin/env python3
"""Validate generated docs, links/anchors, catalog and private-data exclusions."""
from pathlib import Path
import importlib.util,json,re,sys
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('tyrargo_docs',ROOT/'tools/update_docs.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
errors=[]
for name,expected in mod.outputs().items():
 p=ROOT/name
 if not p.exists() or p.read_text('utf-8')!=expected.rstrip()+'\n':errors.append('Stale generated document: '+name)
files=[x for x in ROOT.rglob('*.md') if '.git' not in x.parts and 'templates' not in x.parts]
for p in files:
 text=p.read_text('utf-8-sig')
 if '{{' in text:errors.append('Unexpanded marker: '+str(p.relative_to(ROOT)))
 if '\ufffd' in text:errors.append('Replacement glyph: '+str(p.relative_to(ROOT)))
 if re.search(r'\bCrysis\s*2\b',text,re.I):errors.append('Disallowed public presentation reference: '+str(p.relative_to(ROOT)))
 for target in re.findall(r'(?<!!)\[[^\]]+\]\(([^)]+)\)|!\[[^\]]*\]\(([^)]+)\)|(?:src|href)="([^"]+)"',text):
  value=next(x for x in target if x)
  if re.match(r'^(https?://|mailto:|#)',value):continue
  value=value.split(' ',1)[0].strip('<>');relative,_,anchor=value.partition('#')
  dest=(p.parent/relative).resolve()
  if not dest.is_file():errors.append(f'Missing link: {p.relative_to(ROOT)} → {value}')
  elif anchor and dest.suffix=='.md' and not re.fullmatch(r'L\d+',anchor):
   content=dest.read_text('utf-8-sig')
   if f'id="{anchor}"' not in content:errors.append(f'Missing explicit anchor: {p.relative_to(ROOT)} → {value}')
feature=json.loads((ROOT/'docs/data/features.json').read_text('utf-8'))
if len({x['id'] for x in feature['items']})!=len(feature['items']):errors.append('Duplicate feature IDs')
for x in feature['items']:
 if x['status']!='implemented':errors.append('Non-implemented item in implemented count: '+x['id'])
 if x['category'] not in feature['categories']:errors.append('Unknown category: '+x['id'])
 if set(x['title'])!={'en','es'}:errors.append('Missing translation: '+x['id'])
for x in (ROOT/'docs/media').rglob('*'):
 if not x.is_file():continue
 if x.stat().st_size>5*1024*1024:errors.append('Oversized documentation media: '+x.name)
if errors:print('\n'.join(errors).replace('→','->'));sys.exit(1)
print(f'PASS: {len(files)} Markdown files, bilingual generation, links/anchors, {len(feature["items"])} unique implemented entries and media limits.')
