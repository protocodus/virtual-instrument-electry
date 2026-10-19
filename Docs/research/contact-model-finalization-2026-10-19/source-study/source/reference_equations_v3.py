"""UNEXECUTED v3 independent direct q/v free solution, no qualified API import.

NaturalOmega and alpha are unchanged. Uses direct scalar-ODE q/v coefficients
rather than the candidate's energy-matrix API. Separate code is not a rigorous
libm certificate; first/refined and saved independent precision gates still own
numerical admission.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
import math

def state(case,qret,vret,elapsed,np,record_sink=None):
    elapsed=np.asarray(elapsed);q=np.full((2,len(case['n']),len(elapsed)),np.nan);v=np.full_like(q,np.nan)
    if record_sink is not None:record_sink.update({'attempted_reference_q':q,'attempted_reference_v':v,'elapsed':elapsed,'completed_reference_axis_mode':None})
    for axis in range(2):
        for j,w in enumerate(case['omega']):
            a=float(case['alpha'][axis,j]);w=float(w);q0=float(qret[axis,j]);v0=float(vret[axis,j])
            if a<w:
                beta=math.sqrt((w-a)*(w+a));z=beta*elapsed
                ratio=np.ones_like(z);np.divide(np.sin(z),z,out=ratio,where=z!=0)
                decay=np.exp(-a*elapsed);c=decay*np.cos(z);s=decay*elapsed*ratio
            elif a==w:
                c=np.exp(-a*elapsed);s=elapsed*c
            else:
                gamma=math.sqrt((a-w)*(a+w));slow=-w*w/(a+gamma);fast=-a-gamma
                e1=np.exp(slow*elapsed);e2=np.exp(fast*elapsed)
                c=(e1+e2)/2;s=e1*(-np.expm1(-2*gamma*elapsed))/(2*gamma)
            q[axis,j]=q0*c+(v0+a*q0)*s
            v[axis,j]=v0*c-(w*w*q0+a*v0)*s
            if record_sink is not None:record_sink['completed_reference_axis_mode']=[axis,j]
    return q,v

if __name__=='__main__':raise SystemExit('DRAFT only; no reference state evaluation authorized')
