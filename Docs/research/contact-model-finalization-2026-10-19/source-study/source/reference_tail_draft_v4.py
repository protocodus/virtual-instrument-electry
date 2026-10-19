"""UNEXECUTED complete v4 batched independent reference free tail; v1/v2 preserved.

Only the first offgrid interval uses direct positive velocity-square integral.
Full-h D evaluates literal qualified operators on independently analytical left
states in4096-frame administrative batches, preserving chronological cumsum.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
import hashlib,math
from source_inputs_v1 import require
from reference_equations_v3 import state

RATE=192000
CHUNK=4096

def _first_grid_at_or_after(t):
    f=math.floor(t*RATE)
    if f/RATE<t:f+=1
    if (f-1)/RATE>=t:f-=1
    require(f/RATE>=t and (f-1)/RATE<t,'first physical canonical sample proof')
    return f

def tail(case,reference,times,np,roots_legendre,record_sink):
    require(reference['retired'],'active contact cannot become a free tail')
    require(len(times)>0,'caller must explicitly record an empty final-crop tail')
    start=float(reference['retirement']['time_s']);q0=reference['q_retire_or_end'];v0=reference['v_retire_or_end']
    frames=np.rint(np.asarray(times)*RATE).astype(np.int64)
    require(np.array_equal(times,frames.astype(float)/RATE) and np.all(np.diff(frames)==1),'complete unchanged canonical192k grid')
    require(int(frames[0])==_first_grid_at_or_after(start),'no skipped or snapped first event-to-grid interval')
    h=1/RATE;table=case['tables'][h.hex()];D=np.asarray([reference['D_J'],reference['D_J']],dtype=float)
    # Retain event's own real time and q/v; neither is moved to a sample.
    width=float(times[0]-start);first_di=[]
    record_sink.update({'reference_retirement_q':q0,'reference_retirement_v':v0,'reference_event_time':start,'first_partial_width':width,'first_partial_D':first_di})
    for order in (16,32):
        nodes,weights=roots_legendre(order)
        if width==0:raw=np.zeros((2,len(case['n'])))
        else:
            _,vv=state(case,q0,v0,(nodes+1)*width/2,np,record_sink)
            raw=case['mass']*case['alpha']*width*np.sum(vv*vv*weights[None,None,:],axis=2)
        record_sink.setdefault('first_partial_raw_D_by_order',{})[str(order)]=raw
        require(np.isfinite(raw).all() and np.all(raw>=0),'invalid raw first partial reference D')
        first_di.append(float(np.sum(raw)))
    for at in range(0,len(times),CHUNK):
        tt=times[at:at+CHUNK];ff=frames[at:at+CHUNK]
        record_sink.pop('full_h_reference_raw_D',None)
        record_sink.update({'reference_frames':ff,'reference_times':tt,'incoming_D':D.copy()})
        q,v=state(case,q0,v0,tt-start,np,record_sink)
        interval=np.empty((len(tt),2))
        if at==0:
            interval[0]=first_di
            leftq=q[:,:,:-1];leftv=v[:,:,:-1];offset=1
        else:
            leftq=np.concatenate((qleft[:,:,None],q[:,:,:-1]),axis=2)
            leftv=np.concatenate((vleft[:,:,None],v[:,:,:-1]),axis=2);offset=0
        raw_min=0.
        if len(tt)>offset:
            X=np.stack((case['omega'][None,:,None]*leftq,leftv),axis=-1)
            raw=case['mass']*np.einsum('anti,ankij,antj->antk',X,table['KX'][:,:,1:3],X)
            record_sink['full_h_reference_raw_D']=raw
            require(np.isfinite(raw).all() and np.all(raw>=0),'invalid raw full-h reference D')
            interval[offset:]=np.sum(raw,axis=(0,1));raw_min=float(np.min(raw))
        # Include incoming cumulative total as the first chronological summand,
        # rather than adding it to an already-summed block afterward.
        cumulative=np.cumsum(np.concatenate((D[None,:],interval),axis=0),axis=0)[1:]
        E=.5*case['mass']*np.sum(v*v+(case['omega'][None,:,None]*q)**2,axis=(0,1))
        tap=np.sum(case['observer'][:,:,None]*q,axis=(0,1))
        require(np.isfinite(q).all() and np.isfinite(v).all() and np.isfinite(cumulative).all()
                and np.isfinite(E).all()and np.isfinite(tap).all()
                and np.isfinite(E-reference['work_J']+cumulative[:,1]).all(),'nonfinite analytical reference tail')
        ordered=np.stack((np.moveaxis(q,2,0),np.moveaxis(v,2,0)),axis=1)
        yield {'frames':ff,'times_s':tt,'q':q,'v':v,'D16_32_J':cumulative,'interval_D16_32_J':interval,
            'work_J':np.full(len(tt),reference['work_J']),'string_energy_J':E,'physical_tap':tap,
            'ledger_numerator_J':E-reference['work_J']+cumulative[:,1],
            'retirement_time_s':start,'first_partial_interval_s':width if at==0 else None,
            'raw_full_h_D_by_mode_minimum_J':raw_min,
            'ordered_frame_q_then_v_sha256':hashlib.sha256(ordered.tobytes()).hexdigest(),
            'last_q':q[:,:,-1].copy(),'last_v':v[:,:,-1].copy(),
            'own_reference_formula_no_candidate_API':True,'no_endpoint_energy_D_definition':True}
        qleft=q[:,:,-1].copy();vleft=v[:,:,-1].copy();D=cumulative[-1].copy()

if __name__=='__main__':raise SystemExit('DRAFT only; no reference/tail/quadrature calls authorized')
