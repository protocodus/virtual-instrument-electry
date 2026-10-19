"""UNEXECUTED actual-clock C1 press and fixed96k PWL withdrawal hypothesis."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
import math
from source_inputs_v1 import PHYSICAL_RATE,require

class SupportPath:
    def __init__(self,case,y0,v0,np,signal):
        self.case=case;self.y0=float(y0);self.v0=float(v0);self.D=case['D_m']
        self.contact=case['contact_frame96']/PHYSICAL_RATE
        self.release=case['release_frame96']/PHYSICAL_RATE
        self.duration=(case['release_frame96']-case['contact_frame96'])/PHYSICAL_RATE
        count=case['calls96']-case['release_frame96']
        # This is the original normalized pulse and captured one-pole, evaluated
        # to the original crop end. No endpoint renormalization or R clipping.
        a=case['release_coefficient'];pulse=case['pulse']
        self.rate=signal.lfilter([1-a],[1,-a],np.pad(pulse,(0,count-len(pulse))))
        self.R=np.cumsum(self.rate)
        self.z=self.y0+self.D-2*self.D*self.R
        require(float(self.R[0])==0.0,'withdrawal first knot must meet press endpoint')
        self.knots=(case['release_frame96']+np.arange(count,dtype=float))/PHYSICAL_RATE
        self.slopes=np.diff(self.z)*PHYSICAL_RATE
        require(np.isfinite(self.z).all() and np.isfinite(self.slopes).all(),'finite declared path')
        self.unapplied_kernel_mass=1-float(self.R[-1])

    def value(self,t,side='right'):
        """Exact side of each physical knot is declared, not smoothed away.

        No evaluation beyond the fixed original crop: no held-tail extension.
        """
        require(side in ('left','right'),'support sidedness')
        if t<self.contact:return math.nan,math.nan,math.nan
        if t<self.release or (t==self.release and side=='left'):
            s=(t-self.contact)/self.duration
            if t==self.contact:s=0.0
            if t==self.release:s=1.0
            z=self.y0+self.duration*self.v0*(s-2*s*s+s*s*s)+self.D*(3*s*s-2*s*s*s)
            zd=self.v0*(1-4*s+3*s*s)+self.D*(6*s-6*s*s)/self.duration
            zdd=self.v0*(-4+6*s)/self.duration+self.D*(6-12*s)/self.duration**2
            return float(z),float(zd),float(zdd)
        require(t<=float(self.knots[-1]),'support cannot extend beyond original crop')
        # Search the actual stored canonical knot values, avoiding floor(t*Fs)
        # ambiguity at integer-frame times. Last knot has no right interval.
        import bisect
        j=bisect.bisect_left(self.knots,t)
        exact=j<len(self.knots) and float(self.knots[j])==t
        if exact:
            i=j-1 if side=='left' else j
            if j==0 and side=='left':return self.y0+self.D,0.0,0.0
            slope=float(self.slopes[i]) if 0<=i<len(self.slopes) else math.nan
            return float(self.z[j]),slope,0.0
        i=j-1;require(0<=i<len(self.slopes),'PWL interval in fixed crop')
        return float(self.z[i]+(t-self.knots[i])*self.slopes[i]),float(self.slopes[i]),0.0

    def policy_facts(self,t,y,v):
        """Both sides of every support velocity jump, with unchanged field.

        The final crop has no declared right slope, so it cannot invent an
        otherwise-unproven right-sided retirement at that terminal knot.
        """
        sides={}
        for side in ('left','right'):
            z,zd,_=self.value(t,side);delta=z-y;relative=zd-v
            sides[side]={'delta_m':delta,'relative_velocity_m_per_s':relative,
                'policy_g_m':max(delta,self.duration*relative) if math.isfinite(relative) else None,
                'eligible':bool(t>=self.release and math.isfinite(relative) and delta<=0 and relative<=0)}
        return {'time_s':t,'left':sides['left'],'right':sides['right'],
            'newly_eligible_on_slope_jump':not sides['left']['eligible'] and sides['right']['eligible'],
            'no_state_change_or_smooth_root_assumption':True}

if __name__=='__main__':raise SystemExit('DRAFT only; no support/force/state evaluation authorized')
