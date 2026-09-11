#!/usr/bin/env python3
"""Descriptive per-partial attack relaxation, with explicit quality exclusions.
A new independent estimator, not a reproduction of the missing August script.
"""
import argparse,hashlib,json,math,pathlib,warnings
import numpy as np
from scipy import signal
from scipy.io import wavfile
VERSION='electry-partial-pitch-20260911/v1'
PROTOCOL={'schema':VERSION,'analysis_rate':2000,'partials':list(range(1,7)),
 'late_carrier_seconds':[0.6,0.8],'early_seconds':[0.16,0.24],
 'slope_seconds':[0.16,0.44],'late_seconds':[0.6,0.8],
 'complex_demod_lowpass_f0_fraction':0.22,'fir_length_fundamental_cycles':8,
 'fir_window':['kaiser',8.6],'phase_derivative_savgol_seconds':0.041,'phase_derivative_order':2,
 'partial_prominence_min_db':15,'partial_level_min_db':-50,'max_cents_range':60,
 'min_common_partials':4,'coherent_attack_floor_cents':1.0,'cross_partial_attack_mad_max_cents':1.5,
 'onset':'first sample crossing 25% of first-second peak centered 2ms RMS',
 'scope':'descriptive original-PCM ordinary DI check; no coefficient fitting, velocity or string identity inferred; each stereo channel analyzed separately'}
def sha(p):return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
def onset(x,fs):
 width=max(1,round(.002*fs));env=np.sqrt(signal.convolve(x*x,np.ones(width)/width,mode='same'))
 ix=np.flatnonzero(env>=.25*np.max(env[:min(len(x),fs)]))
 return int(ix[0]) if len(ix) else 0

def analyze_array(x,fs,midi,onset_index=None):
 f0=440*2**((midi-69)/12);x=np.asarray(x,dtype=float);oi=onset(x,fs) if onset_index is None else onset_index
 active=x[oi:]; peak=float(np.max(np.abs(active))) if len(active) else 0
 out={'midi':midi,'nominal_hz':f0,'onset_seconds':oi/fs,'onset_censored':oi/fs<.003,'peak':peak,'partials':[]}
 if peak<1e-8 or len(active)<.91*fs:return out|{'sufficient':False,'reason':'silence or insufficient duration'}
 # Preserve real lead-in when available; pad only to permit a centred offline FIR.
 start=max(0,oi-round(.3*fs));end=min(len(x),oi+round(1.1*fs));xx=x[start:end]
 frac=(oi-start)/fs;rate=PROTOCOL['analysis_rate'];g=math.gcd(fs,rate)
 z=signal.resample_poly(xx,rate//g,fs//g);t=np.arange(len(z))/rate-frac
 late=(t>=.6)&(t<=.8);s=z[late]*np.hanning(np.sum(late));nfft=32768
 spec=np.abs(np.fft.rfft(s,nfft));freq=np.fft.rfftfreq(nfft,1/rate)
 taps=int(round(8*rate/f0))|1;fir=signal.firwin(taps,.22*f0,fs=rate,window=('kaiser',8.6))
 window=int(round(.041*rate))|1
 for h in PROTOCOL['partials']:
  center=h*f0
  if center+.32*f0>=rate/2:continue
  bins=np.flatnonzero((freq>(h-.3)*f0)&(freq<(h+.3)*f0));j=int(bins[np.argmax(spec[bins])]);a,b,c=np.log(np.maximum(spec[j-1:j+2],1e-30));delta=.5*(a-c)/(a-2*b+c);carrier=(j+delta)*rate/nfft
  floor_bins=bins[np.abs(freq[bins]-carrier)>.12*f0];noise=float(np.median(spec[floor_bins]));prom=20*np.log10(max(spec[j],1e-30)/max(noise,1e-30))
  analytic=2*signal.fftconvolve(z*np.exp(-2j*np.pi*carrier*np.arange(len(z))/rate),fir,mode='same')
  amplitude=np.abs(analytic);phase=np.unwrap(np.angle(analytic));derivative=signal.savgol_filter(phase,window,2,deriv=1,delta=1/rate)
  instantaneous=carrier+derivative/(2*np.pi);valid=(instantaneous>0)&(amplitude>peak*10**(-50/20));latevalid=late&valid
  level=20*np.log10(max(float(np.median(amplitude[late])),1e-30)/peak)
  part={'harmonic':h,'carrier_hz':float(carrier),'prominence_db':float(prom),'late_level_db':float(level)}
  if prom<15 or level<-50 or np.mean(valid[late])<.95:
   part|={'sufficient':False,'reason':'weak or poorly separated late partial'};out['partials'].append(part);continue
  reference=float(np.median(instantaneous[latevalid]));cents=1200*np.log2(np.maximum(instantaneous,1e-30)/reference)
  early=(t>=.16)&(t<=.24);fit=(t>=.16)&(t<=.44)
  if np.mean(valid[early|fit])<.95 or np.max(np.abs(cents[early|fit]))>60:
   part|={'sufficient':False,'reason':'unreliable early phase trajectory'};out['partials'].append(part);continue
  attack=float(np.median(cents[early]));slope=float(np.polyfit(t[fit],cents[fit],1)[0]);
  part|={'sufficient':True,'late_hz':reference,'attack_cents':attack,'slope_cents_per_second':slope,
    'trace':[{'seconds':round(float(tt),3),'cents':round(float(cc),6)} for tt,cc in zip(t[(t>=.14)&(t<=.85)][::20],cents[(t>=.14)&(t<=.85)][::20])]}
  out['partials'].append(part)
 good=[p for p in out['partials'] if p.get('sufficient')];fund=next((p for p in good if p['harmonic']==1),None)
 if len(good)<4 or fund is None:return out|{'sufficient':False,'reason':'requires fundamental and at least three additional partials'}
 attacks=np.array([p['attack_cents'] for p in good]);slopes=np.array([p['slope_cents_per_second'] for p in good]);median=float(np.median(attacks));mad=float(np.median(np.abs(attacks-median)));slope=float(np.median(slopes))
 coherent=median>=1 and slope<0 and fund['attack_cents']>=1 and fund['slope_cents_per_second']<0 and mad<=1.5 and np.sum((attacks>0)&(slopes<0))>=4
 return out|{'sufficient':True,'aggregate_attack_cents':median,'aggregate_slope_cents_per_second':slope,'cross_partial_attack_mad_cents':mad,'fundamental_attack_cents':fund['attack_cents'],'fundamental_slope_cents_per_second':fund['slope_cents_per_second'],'coherent_downward_relaxation':bool(coherent)}

def analyze_file(path,midi):
 with warnings.catch_warnings():warnings.simplefilter('ignore');fs,x=wavfile.read(path)
 if x.dtype.kind=='i':x=x.astype(float)/2**(np.iinfo(x.dtype).bits-1)
 elif x.dtype.kind=='u':x=(x.astype(float)-128)/128
 else:x=x.astype(float)
 if x.ndim==1:x=x[:,None]
 return {'file':str(path),'sha256':sha(path),'sample_rate':int(fs),'duration_seconds':len(x)/fs,'channels':[analyze_array(x[:,c],int(fs),midi)|{'channel':c+1} for c in range(x.shape[1])]}

def self_test():
 fs=12000;f0=440*2**((40-69)/12);t=np.arange(fs*2)/fs-.25;u=np.maximum(t,0);results={}
 for label,initial in [('static_differential_decay',0),('shared_glide',7)]:
  factor=np.exp2(initial*np.exp(-u/.3049)/1200);phase=np.cumsum(factor)/fs
  x=np.zeros_like(t)
  for h in range(1,7):x+=(t>=0)*(1/h)*np.exp(-(1+.6*h)*u)*np.sin(2*np.pi*h*f0*np.sqrt(1+.0008*h*h)*phase+.4*h)
  r=analyze_array(x,fs,40,onset_index=round(.25*fs));results[label]=r
  assert r['sufficient'],label
  if initial==0:assert abs(r['aggregate_attack_cents'])<.1 and not r['coherent_downward_relaxation'],r
  else:
   expected=float(np.median(initial*np.exp(-np.arange(.16,.24,1/fs)/.3049))-np.median(initial*np.exp(-np.arange(.6,.8,1/fs)/.3049)))
   assert abs(r['aggregate_attack_cents']-expected)<.25 and r['coherent_downward_relaxation'],(r,expected)
   results[label]['analytic_attack_cents']=expected
  rq=analyze_array(.01*x,fs,40,onset_index=round(.25*fs));assert abs(rq['aggregate_attack_cents']-r['aggregate_attack_cents'])<1e-8
 for label,x in [('silence',np.zeros_like(t)),('single_partial',(t>=0)*np.sin(2*np.pi*f0*t))]:
  r=analyze_array(x,fs,40,onset_index=round(.25*fs));results[label]=r;assert not r['sufficient'],label
 return results
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--manifest',type=pathlib.Path);ap.add_argument('--output',type=pathlib.Path,required=True);ap.add_argument('--self-test',action='store_true');args=ap.parse_args()
 result={'protocol':PROTOCOL,'analyzer_sha256':sha(__file__),'numpy':np.__version__}
 if args.self_test:result['self_test']=self_test()
 if args.manifest:result['manifest_sha256']=sha(args.manifest);result['recordings']=[analyze_file(x['file'],x['midi'])|{'label':x.get('label',pathlib.Path(x['file']).stem)} for x in json.loads(args.manifest.read_text())['files']]
 args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
 print(args.output)
