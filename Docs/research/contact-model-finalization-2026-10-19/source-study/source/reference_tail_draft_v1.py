"""UNEXECUTED analytical free reference tail with positive composite D.

No endpoint-energy-defined dissipation. No new general exponential evaluator.
Only the first offgrid interval needs direct velocity-square quadrature; all
subsequent canonical192k intervals use the already-qualified positive16/32
operators evaluated on independently analytical left states.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
import math
from source_inputs_v1 import require

def _state(case,q0,v0,elapsed,qualified_api,np):
    n=len(case['n']);elapsed=np.asarray(elapsed);q=np.empty((2,n,len(elapsed)));v=np.empty_like(q)
    for a in range(2):
        for j,w in enumerate(case['omega']):
            p=qualified_api.analytic_transition(float(w),float(case['alpha'][a,j]),elapsed)
            q[a,j]=p[:,0,0]*q0[a,j]+p[:,0,1]*v0[a,j]/w
            v[a,j]=p[:,1,0]*(w*q0[a,j])+p[:,1,1]*v0[a,j]
    return q,v

def tail(case,reference,times,qualified_api,np,roots_legendre,chunk_frames=4096):
    """Yield fixed-time evidence chunks; caller retains them without realigning.

    The administrative chunk default inherits the existing4096 witness size.
    A future manifest must bind this precise free-tail algorithm and API bytes.
    """
    require(reference['retired'],'cannot substitute a free tail for active contact')
    start=float(reference['retirement']['time_s']);q0=reference['q_retire_or_end'];v0=reference['v_retire_or_end']
    require(np.all(times>=start),'reference tail times precede its own retirement')
    h=1/192000;tab=case['tables'][h.hex()];D=np.asarray([reference['D_J'],reference['D_J']],dtype=float)
    qleft=q0.copy();vleft=v0.copy();tleft=start;first=True
    rules={order:roots_legendre(order)for order in(16,32)}
    for at in range(0,len(times),chunk_frames):
        tt=times[at:at+chunk_frames];q,v=_state(case,q0,v0,tt-start,qualified_api,np)
        cumulative=np.empty((len(tt),2));per_interval=np.empty_like(cumulative)
        for j,t in enumerate(tt):
            if first:
                width=float(t-start);di=[]
                for order in(16,32):
                    nodes,weights=rules[order]
                    if width==0:raw=np.zeros((2,len(case['n'])))
                    else:
                        _,vv=_state(case,q0,v0,(nodes+1)*width/2,qualified_api,np)
                        raw=case['mass']*case['alpha']*width*np.sum(vv*vv*weights[None,None,:],axis=2)
                    require(np.isfinite(raw).all() and np.all(raw>=0),'raw first-fractional reference D invalid')
                    di.append(float(np.sum(raw)))
                first=False
            else:
                # The clock is the declared canonical grid, not a fitted dt.
                require(abs((float(t)-tleft)-h)<=math.ulp(float(t))+math.ulp(tleft),'noncanonical reference tail interval')
                x=np.stack((case['omega'][None,:]*qleft,vleft),axis=-1)
                raw=case['mass']*np.einsum('ani,ankij,anj->ank',x,tab['KX'][:,:,1:3],x)
                require(np.isfinite(raw).all() and np.all(raw>=0),'raw composite reference D invalid')
                di=np.sum(raw,axis=(0,1))
            D+=di;cumulative[j]=D;per_interval[j]=di
            qleft=q[:,:,j];vleft=v[:,:,j];tleft=float(t)
        yield {'times_s':tt,'q':q,'v':v,'D16_32_J':cumulative,'interval_D16_32_J':per_interval,
            'work_J':reference['work_J'],'retirement_time_s':start,
            'fixed192k_clock_no_time_alignment':True,'no_energy_difference_D_definition':True}

if __name__=='__main__':raise SystemExit('DRAFT only; no analytical state/quadrature/tail calls authorized')
