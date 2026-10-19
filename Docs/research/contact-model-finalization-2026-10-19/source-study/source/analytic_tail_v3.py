"""UNEXECUTED complete v3 batched MODEL free tail.

This replaces the v1 binary recurrence only AFTER zero-storage retirement.
It is the same declared free ODE, not bit-identical to recurrence arithmetic.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
import hashlib,math
from source_inputs_v1 import require

CHUNK=4096 # Administrative evidence batch, never a physical/tonal parameter.

def state(case,qret,vret,elapsed,qualified_api,np,record_sink=None):
    """Stable qualified analytic API; each method owns its own checkpoint."""
    elapsed=np.asarray(elapsed);q=np.full((2,len(case['n']),len(elapsed)),np.nan);v=np.full_like(q,np.nan)
    if record_sink is not None:record_sink.update({'attempted_q':q,'attempted_v':v,'elapsed':elapsed,'completed_axis_mode':None})
    for a in range(2):
        for j,w in enumerate(case['omega']):
            p=qualified_api.analytic_transition(float(w),float(case['alpha'][a,j]),elapsed)
            q[a,j]=p[:,0,0]*qret[a,j]+p[:,0,1]*vret[a,j]/w
            v[a,j]=p[:,1,0]*(w*qret[a,j])+p[:,1,1]*vret[a,j]
            if record_sink is not None:record_sink['completed_axis_mode']=[a,j]
    return q,v

def batches(case,qret,vret,retirement_frame,last_frame,rate,W,D_initial,qualified_api,np,record_sink):
    require(rate in (96000,192000),'fixed prospective rates')
    require(0<=retirement_frame<=last_frame,'owned retirement within crop')
    h=1/rate;tab=case['tables'][h.hex()];D=np.asarray(D_initial).copy()
    qleft=qret.copy();vleft=vret.copy();tret=retirement_frame/rate
    for first in range(retirement_frame+1,last_frame+1,CHUNK):
        frames=np.arange(first,min(first+CHUNK,last_frame+1),dtype=np.int64)
        times=frames.astype(float)/rate
        record_sink.clear()
        record_sink.update({'retirement_q':qret,'retirement_v':vret,'retirement_frame':retirement_frame,'frames':frames,'times':times,'incoming_D':D.copy(),'constant_W':W})
        q,v=state(case,qret,vret,(frames-retirement_frame).astype(float)/rate,qualified_api,np,record_sink)
        leftq=np.concatenate((qleft[:,:,None],q[:,:,:-1]),axis=2)
        leftv=np.concatenate((vleft[:,:,None],v[:,:,:-1]),axis=2)
        X=np.stack((case['omega'][None,:,None]*leftq,leftv),axis=-1)
        # Raw per-mode three-integral contributions on ACTUAL analytic left
        # states. No transition-energy subtraction, clipping or sum-first test.
        raw=case['mass']*np.einsum('anti,ankij,antj->antk',X,tab['KX'],X)
        record_sink['raw_per_mode_D']=raw
        require(np.isfinite(raw).all() and np.all(raw>=0),'invalid raw analytic-tail D')
        interval_D=np.sum(raw,axis=(0,1));cumulative=D[None,:]+np.cumsum(interval_D,axis=0)
        E=.5*case['mass']*np.sum(v*v+(case['omega'][None,:,None]*q)**2,axis=(0,1))
        tap=np.sum(case['observer'][:,:,None]*q,axis=(0,1))
        ledger=E-W+cumulative[:,0]
        require(np.isfinite(q).all() and np.isfinite(v).all() and np.isfinite(cumulative).all()
                and np.isfinite(E).all() and np.isfinite(tap).all() and np.isfinite(ledger).all(),'nonfinite analytic free tail')
        ordered=np.stack((np.moveaxis(q,2,0),np.moveaxis(v,2,0)),axis=1)
        yield {'frames':frames,'times_s':times,'q':q,'v':v,'physical_tap':tap,
            'string_energy_J':E,'work_J':np.full(len(frames),W),
            'interval_D_Gramian_16_32_J':interval_D,'cumulative_D_Gramian_16_32_J':cumulative,
            'ledger_numerator_J':ledger,'ordered_frame_q_then_v_sha256':hashlib.sha256(ordered.tobytes()).hexdigest(),
            'last_q':q[:,:,-1].copy(),'last_v':v[:,:,-1].copy(),
            'raw_D_by_mode_minimum_J':float(np.min(raw)),
            'retirement_checkpoint_time_s':tret,'no_recurrence_bitidentity_claim':True}
        qleft=q[:,:,-1].copy();vleft=v[:,:,-1].copy();D=cumulative[-1].copy()

if __name__=='__main__':raise SystemExit('DRAFT only; no analytic state/D/tap calls authorized')
