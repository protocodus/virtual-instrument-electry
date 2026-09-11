#!/usr/bin/env python3
"""Fetch only named ordinary DI cells from the CC-BY-4.0 EG-IPT ZIP.
Uses HTTP ranges; checks Content-Range, ZIP CRC and records exact hashes.
No audio is written into tracked directories. Identifier 02 is not assumed
open E2: pitch verification is separate from the literal source selection.
"""
import argparse,hashlib,io,json,pathlib,ssl,urllib.request,zipfile
import certifi
URL='https://zenodo.org/api/records/15205644/files/EG-IPT.zip/content'
SIZE=23757983313
class Ranged(io.RawIOBase):
 def __init__(self,root): self.pos=0; self.root=root; self.requests=[]; self.cache={}
 def seekable(self): return True
 def readable(self): return True
 def tell(self): return self.pos
 def seek(self,offset,whence=0):
  self.pos=offset if whence==0 else self.pos+offset if whence==1 else SIZE+offset
  return self.pos
 def read(self,n=-1):
  if n<0:n=SIZE-self.pos
  if not n:return b''
  start=self.pos;end=min(SIZE,start+n)-1;key=(start,end)
  if key not in self.cache:
   request=urllib.request.Request(URL,headers={'Range':f'bytes={start}-{end}'})
   with urllib.request.urlopen(request,context=ssl.create_default_context(cafile=certifi.where()),timeout=60) as response:
    if response.status!=206 or not response.headers['Content-Range'].startswith(f'bytes {start}-{end}/'):raise RuntimeError(f'Range unsupported: {response.status} {dict(response.headers)}')
    data=response.read()
   if len(data)!=end-start+1:raise RuntimeError('Incomplete range')
   self.cache[key]=data;self.requests.append({'start':start,'end':end,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
  data=self.cache[key];self.pos+=len(data);return data
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('output',type=pathlib.Path);args=ap.parse_args();args.output.mkdir(parents=True,exist_ok=True)
 reader=Ranged(args.output);zf=zipfile.ZipFile(reader)
 names=[n for n in zf.namelist() if '/ordinario/' in n.lower() and '/di/' in n.lower() and '_6s_' in n.lower() and not n.startswith('__MACOSX/')]
 (args.output/'ordinary-sixth-string-names.json').write_text(json.dumps(names,indent=2)+'\n')
 print('\n'.join(names[:30]),flush=True)
 selected=[n for n in names if '02' in pathlib.Path(n).name and n.lower().endswith('.wav')]
 if len(selected)!=3:raise RuntimeError(f'Expected3 ordinary02 cells, found {selected}')
 outputs=[]
 for name in selected:
  data=zf.read(name);dest=args.output/pathlib.Path(name).name;dest.write_bytes(data)
  outputs.append({'member':name,'path':str(dest),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'zip_crc32':format(zf.getinfo(name).CRC,'08x')});print(outputs[-1],flush=True)
 result={'url':URL,'archive_bytes':SIZE,'license':'CC-BY-4.0','source':'https://zenodo.org/records/15205644','selection':'literal ordinario, DI, sixth-string, identifier02 filenames; no frequency or fret inference','files':outputs,'range_requests':reader.requests,'bytes_downloaded':sum(x['bytes'] for x in reader.requests),'script_sha256':hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest()}
 (args.output/'egipt-download.json').write_text(json.dumps(result,indent=2)+'\n')
