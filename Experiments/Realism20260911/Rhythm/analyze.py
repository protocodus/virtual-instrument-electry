import argparse
import json
import pathlib

import numpy as np

parser = argparse.ArgumentParser(description="Compare paired rhythm renderer exports.")
parser.add_argument("--audio-dir", type=pathlib.Path,
                    default=pathlib.Path(__file__).resolve().parents[3]
                    / "build-realism-20260911" / "palm-src")
parser.add_argument("--output-dir", type=pathlib.Path,
                    default=pathlib.Path(__file__).resolve().parent)
args = parser.parse_args()
p = args.audio_dir
args.output_dir.mkdir(parents=True, exist_ok=True)
rows=[]
for sr in [44100,48000,96000]:
 for s,name in enumerate(['open_palm','fret5_palm','down_strum','up_strum','fret12_pressure','zero_spread_chord','palm_riff']):
  for fx in ['dry','modern']:
   a=np.fromfile(p/'baseline-audio'/f'{sr}-{s}-{fx}.f32',dtype='float32').astype('float64');b=np.fromfile(p/'candidate-audio'/f'{sr}-{s}-{fx}.f32',dtype='float32').astype('float64')
   rms=lambda x:np.sqrt(np.mean(x*x)+1e-30)
   row=dict(rate=sr,scenario=name,path=fx,null_db=float(20*np.log10(rms(a-b)/rms(a))),level_db=float(20*np.log10(rms(b)/rms(a))),peak=float(np.max(np.abs(b))),exact=bool(np.array_equal(a,b)))
   rows.append(row)
   if sr==48000:print(sr,name,fx,'null %.2f dB level %+.2f dB peak %.3f'%(row['null_db'],row['level_db'],row['peak']))
(args.output_dir/'comparison-metrics.json').write_text(json.dumps(rows,indent=2)+'\n')
rows=[];sr=48000
for scenario,note in [(1,33),(4,40)]:
 f0=440*2**((note-69)/12)
 for variant in ['baseline','candidate']:
  y=np.fromfile(p/f'{variant}-audio'/f'{sr}-{scenario}-dry.f32',dtype=np.float32)
  vals=[]
  for a,b in [(.03,.13),(.15,.3),(.3,.5),(.6,.8)]:
   x=y[int(a*sr):int(b*sr)];spec=np.abs(np.fft.rfft(x*np.hanning(len(x)),131072))/len(x);f=np.fft.rfftfreq(131072,1/sr)
   h=np.array([max(spec[(f>f0*i*.97)&(f<f0*i*1.03)]) for i in range(1,7)])
   vals.append(20*np.log10(np.maximum(h,1e-15)))
  print(scenario,variant,'late minus early',np.round(vals[-1]-vals[0],2).tolist())
  rows.append(dict(scenario=scenario,variant=variant,harmonic_db=[v.tolist() for v in vals],late_minus_early=(vals[-1]-vals[0]).tolist()))
(args.output_dir/'partial-metrics.json').write_text(json.dumps(rows,indent=2)+'\n')
