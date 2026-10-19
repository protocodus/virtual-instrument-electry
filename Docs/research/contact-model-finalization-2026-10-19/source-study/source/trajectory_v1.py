"""Unexecuted saved-history replay for fixed mechanics comparisons.

Reads complete active q/v chunks and only reconstructs post-retirement states
from their own checkpoint/method. Never invents an active or missing state.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
import json
from source_inputs_v1 import require
from analytic_tail_v3 import state as model_state
from reference_equations_v3 import state as reference_state

class Trajectory:
    def __init__(self,case,descriptor,np,qualified_api):
        self.case=case;self.d=descriptor;self.np=np;self.api=qualified_api
        self.root=Path(descriptor['archive']);self.rate=descriptor['rate'];self.chunks=[]
        for path in sorted(self.root.glob('contact-qv-*.npz')):
            with np.load(path,allow_pickle=False)as z:
                frames=z['frames'].copy()
            if len(frames):self.chunks.append((path,frames))
        self.checkpoint=None
        if descriptor['retired']:
            with np.load(self.root/'replay-checkpoint.npz',allow_pickle=False)as z:
                self.checkpoint={k:z[k].copy()for k in ('q','v','event_time_s','frame')}

    def states(self,frames):
        np=self.np;frames=np.asarray(frames,dtype=np.int64)
        q=np.full((len(frames),2,len(self.case['n'])),np.nan);v=np.full_like(q,np.nan);filled=np.zeros(len(frames),bool)
        before=frames<self.d['entry_frame'];q[before]=0.;v[before]=0.;filled[before]=True
        for path,savedframes in self.chunks:
            ids=np.flatnonzero((frames>=savedframes[0])&(frames<=savedframes[-1]))
            if not len(ids):continue
            pos=np.searchsorted(savedframes,frames[ids]);valid=(pos<len(savedframes))
            ids=ids[valid];pos=pos[valid];valid=savedframes[pos]==frames[ids];ids=ids[valid];pos=pos[valid]
            if len(ids):
                with np.load(path,allow_pickle=False)as z:q[ids]=z['q'][pos];v[ids]=z['v'][pos]
                filled[ids]=True
        if self.checkpoint is not None:
            t=float(self.checkpoint['event_time_s']);ids=np.flatnonzero(~filled&(frames.astype(float)/self.rate>=t))
            if len(ids):
                if self.d['kind']=='model':
                    elapsed=(frames[ids]-int(self.checkpoint['frame'])).astype(float)/self.rate
                    qq,vv=model_state(self.case,self.checkpoint['q'],self.checkpoint['v'],elapsed,self.api,np)
                else:
                    elapsed=frames[ids].astype(float)/self.rate-t
                    qq,vv=reference_state(self.case,self.checkpoint['q'],self.checkpoint['v'],elapsed,np)
                q[ids]=np.moveaxis(qq,2,0);v[ids]=np.moveaxis(vv,2,0);filled[ids]=True
        require(np.all(filled),'missing active q/v or unowned tail sample; no repair')
        require(np.isfinite(q).all()and np.isfinite(v).all(),'nonfinite saved/reconstructed trajectory')
        return q,v

def scalar_summary(block,totals,np):
    """Same actual-work normalization; characterize first, normalize at end."""
    for key in ('string_energy_J','contact_U_J','work_J','D_truth_J','ledger_numerator_J'):
        require(np.isfinite(block[key]).all(),'nonfinite scalar evidence '+key)
    totals['max_abs_work_J']=max(totals['max_abs_work_J'],float(np.max(abs(block['work_J']))))
    totals['max_abs_ledger_numerator_J']=max(totals['max_abs_ledger_numerator_J'],float(np.max(abs(block['ledger_numerator_J']))))
    totals['all_D_nonnegative']=totals['all_D_nonnegative']and bool(np.all(block['D_truth_J']>=0))

def compare(case,left,right,np):
    """Original energy/Cauchy formulas on actual source and pickup read vectors.

    IDs are fixed before data: full_state, pick displacement_m/velocity_m_per_s,
    pre-flux linear pickup tap and its rate_s_inv. The pickup read contains
    (.85,.35)/.040 with units1/m, not calibrated SI displacement or voltage.
    No ratio/band observation or tone metric is defined here.
    """
    rate=min(left.rate,right.rate);last=(case['calls96']-1)*(rate//96000)
    S=max((case['Fq_N']*case['D_m'])**.5,1e-12);m=case['mass'];w=case['omega'][None,:]
    reads={'local_pick':case['B'],'physical_pickup':case['observer']}
    maxima={'full_state':0.,'pick_displacement_m':0.,'pick_velocity_m_per_s':0.,'linear_pickup_tap':0.,'linear_pickup_tap_rate_s_inv':0.}
    identifiers={('local_pick','displacement'):'pick_displacement_m',('local_pick','velocity'):'pick_velocity_m_per_s',('physical_pickup','displacement'):'linear_pickup_tap',('physical_pickup','velocity'):'linear_pickup_tap_rate_s_inv'}
    absolute={k:0. for k in maxima if k!='full_state'}
    for first in range(0,last+1,4096):
        f=np.arange(first,min(first+4096,last+1),dtype=np.int64)
        qa,va=left.states(f*(left.rate//rate));qb,vb=right.states(f*(right.rate//rate));dq=qa-qb;dv=va-vb
        err=m**.5*np.sqrt(np.sum((w[None,:,:]*dq)**2+dv*dv,axis=(1,2)))
        maxima['full_state']=max(maxima['full_state'],float(np.max(err))/S)
        for name,b in reads.items():
            for suffix,diff,weights in [('displacement',dq,b/w),('velocity',dv,b)]:
                key=identifiers[name,suffix];raw=float(np.max(abs(np.sum(diff*b[None,:,:],axis=(1,2)))))
                bound=S/m**.5*float(np.linalg.norm(weights))
                absolute[key]=max(absolute[key],raw);maxima[key]=max(maxima[key],raw/bound)
    return {'fixed_metrics':maxima,'absolute_observation_maxima':absolute,'state_normalization_sqrtJ':S,
        'characteristic_not_initial_storage_J':.5*case['Fq_N']*case['D_m'],'compared_rate_hz':rate,
        'no_time_or_phase_alignment':True}

if __name__=='__main__':raise SystemExit('DRAFT only; no saved-state or comparison calls authorized')
