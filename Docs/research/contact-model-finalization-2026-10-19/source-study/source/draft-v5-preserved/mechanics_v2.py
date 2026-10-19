"""UNEXECUTED v2 collocated two-axis source; raw endpoint failure sink. No state/force calls on import."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from fractions import Fraction
import hashlib,math
from source_inputs_v1 import K,require
from support_path_v1 import SupportPath

def state_hash(q,v):return hashlib.sha256(q.tobytes()+v.tobytes()).hexdigest()

def energy(case,q,v,np):
    return .5*case['mass']*float(np.sum(v*v+(case['omega'][None,:]*q)**2))

def original_numpy_DG(d0,d1,np):
    """Literal original ordinary residual callback, no new force solution."""
    d0=np.asarray(d0);d1=np.asarray(d1);f=np.empty_like(d1);der=np.empty_like(d1)
    pp=(d0>0)&(d1>0);nn=(d0<=0)&(d1<=0);pn=(d0>0)&(d1<=0);np_=(d0<=0)&(d1>0)
    f[pp]=.5*K*(d0[pp]+d1[pp]);der[pp]=.5*K
    f[nn]=0.;der[nn]=0.
    gap=d0[pn]-d1[pn];f[pn]=.5*K*d0[pn]**2/gap;der[pn]=.5*K*d0[pn]**2/gap**2
    gap=d1[np_]-d0[np_];f[np_]=.5*K*d1[np_]**2/gap;der[np_]=.5*K*d1[np_]*(d1[np_]-2*d0[np_])/gap**2
    return f,der

def attach(case,q,v,time,np,signal):
    """Fresh callers pass rest; a repick caller must pass its entire field.

    This function never resets, rescales, reseeds or replaces incoming q/v.
    """
    require(time==case['contact_frame96']/96000,'actual Contact clock')
    before=state_hash(q,v);B=case['B'];y0=float(np.sum(B*q));v0=float(np.sum(B*v))
    path=SupportPath(case,y0,v0,np,signal)
    require(state_hash(q,v)==before,'unstrained attachment changed field')
    return path,{'active':True,'qv_before_sha256':before,'qv_after_sha256':before,
        'y0_m':y0,'v0_m_per_s':v0,'contact_storage_J':0.,'initial_physical_string_energy_J':energy(case,q,v,np),
        'state_error_normalization_energy_J':.5*case['Fq_N']*case['D_m'],
        'normalization_is_not_initial_stored_energy':True}

def _dissipation(case,q,v,modal_force,h,np):
    table=case['tables'][float(h).hex()]
    x=np.stack((case['omega'][None,:]*q-modal_force/(case['mass']*case['omega'][None,:]),v),axis=-1)
    raw=case['mass']*np.einsum('ani,ankij,anj->ank',x,table['KX'],x)
    if not(np.isfinite(raw).all() and np.all(raw>=0)):
        error=RuntimeError('raw modal D invalid; no clipping')
        error.source_evidence={'q':q.tolist(),'v':v.tolist(),'modal_force':modal_force.tolist(),
            'raw_D':raw.tolist(),'raw_D_bytes_hex':raw.tobytes().hex(),'h':h}
        raise error
    return np.sum(raw,axis=(0,1))

def construction_facts(B,q,qfree,q1,z0,z1,mob,A,f,certificate,np):
    """Same exact stored-binary Gram/gather/application accounting as unit.

    It describes arithmetic mismatch; it cannot replace total applied work.
    """
    rational=lambda x:Fraction.from_float(float(x));zero=Fraction(0);Kr=rational(K)
    br=list(map(rational,B.ravel()));qr=list(map(rational,q.ravel()));qfr=list(map(rational,qfree.ravel()));q1r=list(map(rational,q1.ravel()));mr=list(map(rational,mob.ravel()));fr=rational(f)
    gather=lambda values:sum((a*b for a,b in zip(br,values)),zero)
    d0=rational(z0)-gather(qr);df=rational(z1)-gather(qfr);d1=rational(z1)-gather(q1r)
    gram=sum((b*m*b for b,m in zip(br,mr)),zero)
    stored0=rational(z0-float(np.sum(B*q)));storedfree=rational(z1-float(np.sum(B*qfree)))
    e0=abs(d0-stored0);ef=abs(df-storedfree);op=abs(gram-rational(A))
    if d0==d1:target=Kr*max(d0,zero)
    else:target=(Kr*max(d1,zero)**2/2-Kr*max(d0,zero)**2/2)/(d1-d0)
    applied=abs(fr-target);exact=Fraction(*certificate['exact_residual_inf_N'])
    pair=lambda x:[x.numerator,x.denominator]
    return {'binary_full_Gram_m_per_N':pair(gram),'d0_gather_error_m':pair(e0),
        'dfree_gather_error_m':pair(ef),'solve_vs_binary_Gram_error_m_per_N':pair(op),
        'binary_flow_target_residual_bound_N':pair(exact+(Kr/2)*(e0+ef+op*abs(fr))),
        'actual_applied_endpoint_residual_N':pair(applied),
        'contact_residual_work_defect_bound_J':pair(applied*abs(d1-d0)),
        'scope':'exact stored binary inputs, not ideal transcendental coefficients or whole-flow bound'}

def step(case,path,contact,q,v,t0,t1,h,np,native_solve,physical_lambda_bound):
    """One exact-held-force modal interval; caller retains every raw outcome.

    native_solve is the immutable qualified adapter, authenticated by a future
    gate. It must preserve exact/native/actual-original-NumPy rails and traces.
    """
    table=case['tables'][float(h).hex()];B=case['B']
    qfree=table['phi11']*q+table['phi12']*v
    vfree=table['phi21']*q+table['phi22']*v
    z0=path.value(t0,'right')[0];z1=path.value(t1,'right')[0]
    f=0.;status=None;A=float(np.sum(B*table['gamma_q']*B));record={}
    if contact['active']:
        require(Fraction.from_float(A)*Fraction.from_float(K/2)<=physical_lambda_bound,
                'actual two-axis source mobility exceeds immutable fixed kernel bound')
        d0=z0-float(np.sum(B*q));df=z1-float(np.sum(B*qfree))
        try:
            force,status=native_solve(np.asarray([[A]]),np.asarray([d0]),np.asarray([df]),K,
                lambda a,b:original_numpy_DG(a,b,np))
            f=float(force[0])
        except BaseException as error:
            error.source_evidence={'q':q.tolist(),'v':v.tolist(),'B':B.tolist(),
                'z0':z0,'z1':z1,'d0':d0,'dfree':df,'A':A,
                'native_failure_evidence':getattr(error,'native_certificate',None),'no_valid_endpoint':True}
            raise
        record={'solve_d0_m':d0,'solve_dfree_m':df,'solve_A_m_per_N':A,
            'actual_original_numpy_residual_N':status['residual_inf_N'],
            'native_status':status,'force_N':f,'no_repair_or_force_rescale':True}
    u=B*f;q1=qfree+table['gamma_q']*u;v1=vfree+table['gamma_v']*u
    try:
        D=_dissipation(case,q,v,u,h,np)
    except BaseException as error:
        error.source_evidence.update({'raw_applied_q1':q1.tolist(),'raw_applied_v1':v1.tolist(),
            'native_status':status,'source_equation':record,'time_interval_s':[t0,t1]})
        raise
    work=f*(z1-z0) if contact['active'] else 0.
    if not(np.isfinite(q1).all() and np.isfinite(v1).all()):
        error=RuntimeError('nonfinite applied source endpoint')
        error.source_evidence={'raw_applied_q1':q1,'raw_applied_v1':v1,'q':q,'v':v,
            'native_status':status,'source_equation':record,'time_interval_s':[t0,t1],'raw_D':D}
        raise error
    if contact['active']:
        cert=status['certificate'];cert['construction']=construction_facts(B,q,qfree,q1,z0,z1,table['gamma_q'],A,f,cert,np)
        y=float(np.sum(B*q1));vv=float(np.sum(B*v1));policy=path.policy_facts(t1,y,vv)
        delta=z1-y;storage=.5*K*max(delta,0.)**2
        record.update({'policy':policy,'endpoint_delta_m':delta,'endpoint_contact_U_J':storage,
            'physical_endpoint_force_N':K*max(delta,0.),'qv_endpoint_sha256':state_hash(q1,v1)})
        if policy['right']['eligible']:
            before=state_hash(q1,v1);require(storage==0.,'cannot delete positive contact storage')
            contact['active']=False;contact['retirement']={'time_s':t1,'U_J':storage,
                'qv_before_sha256':before,'qv_after_sha256':state_hash(q1,v1),'policy':policy,
                'sampled_bracket_s':[t0,t1]}
    return q1,v1,work,D,record

def free_step(case,q,v,h,np):
    """Identical ordered recurrence for reconstructible post-retirement tail."""
    tab=case['tables'][float(h).hex()]
    q1=tab['phi11']*q+tab['phi12']*v;v1=tab['phi21']*q+tab['phi22']*v
    return q1,v1,_dissipation(case,q,v,np.zeros_like(q),h,np)

if __name__=='__main__':raise SystemExit('DRAFT only; no physical/force/state execution authorized')
