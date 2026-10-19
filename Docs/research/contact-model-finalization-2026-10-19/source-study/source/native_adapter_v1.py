"""Unexecuted source native T.lower adapter; routing-only copy of qualified adapter. Importing starts no child or force/state work."""
if not __debug__:
    raise SystemExit('Optimized Python is forbidden')
from pathlib import Path
from fractions import Fraction
import hashlib,importlib.util,json,math,struct,subprocess,sys
P=Path(__file__).resolve().parent;R=P.parent
ORIGINAL_PHYSICAL_LAMBDA=Fraction(171136785840078848,3141016124975315625)
TAU=Fraction.from_float(2e-11)
NATIVE_LAMBDA=Fraction.from_float(float.fromhex('0x1.be56571018a6bp-5'))
_session=None
def rational(x):
    x=float(x)
    if not math.isfinite(x):raise ValueError('nonfinite exact diagnostic input')
    return Fraction.from_float(x)
def pair(x):return [x.numerator,x.denominator]
def maximum(x):return max(x,default=Fraction(0))
def dg(a,b,K):
    # Independently exact potential quotient, also used by unchanged construction.
    if a==b:return K*max(a,Fraction(0))
    return (K*max(b,Fraction(0))**2/2-K*max(a,Fraction(0))**2/2)/(b-a)
def word(x):return struct.pack('>d',float(x)).hex()
def value(x):return struct.unpack('>d',bytes.fromhex(x))[0]
def require(c,msg):
    if not c:raise RuntimeError(msg)
class NativeFailure(RuntimeError):pass
class Session:
    def __init__(self,manifest,pointer):
        from gate_v1 import sha
        declared=manifest['native_driver'];self.binary=R/declared['path']
        require(sha(self.binary)==declared['sha256'],'immutable native binary changed')
        # Frozen exact checker is imported only after fresh full-flow gate.
        path=R/'build-certified-contact-rt-replay/exact_check_v2.py'
        spec=importlib.util.spec_from_file_location('full_flow_frozen_exact_checker',path)
        self.checker=importlib.util.module_from_spec(spec);sys.modules[spec.name]=self.checker;spec.loader.exec_module(self.checker)
        self.output=P/'run-v1'/'native';require(self.output.is_dir(),'exclusive source native output missing')
        self.stderr_path=self.output/'native-child-stderr-v1.txt';require(not self.stderr_path.exists(),'native child already attempted')
        self.stderr=self.stderr_path.open('x');self.sequence=0;self.closed=False
        self.manifest_sha=sha(P/'implementation-manifest-v1.json');self.pointer=pointer
        self.child=subprocess.Popen([str(self.binary)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=self.stderr,text=True,bufsize=1)
    def request(self,A,d0,df):
        J=len(d0);pad=lambda v:list(v)+[0.]*(2-len(v))
        words=[word(A[i,j]) if i<J and j<J else word(0.) for i in range(2) for j in range(2)]+list(map(word,pad(d0)))+list(map(word,pad(df)))
        line=str(self.sequence)+' '+str(J)+' '+' '.join(words)+'\n';raw=''
        try:
            self.child.stdin.write(line);self.child.stdin.flush();raw=self.child.stdout.readline()
            require(bool(raw),'native child ended before response')
            result=json.loads(raw);require(result['id']==self.sequence,'native sequence mismatch')
            self.sequence+=1;return result
        except BaseException as e:
            record={'schema':'electry-native-full-flow-transport-failure/v1','sequence':self.sequence,'request_raw':line,'response_raw':raw,'failure':repr(e),'no_retry_or_valid_endpoint':True}
            path=self.output/'native-transport-failure-v1.json';require(not path.exists(),'existing transport failure evidence')
            path.write_text(json.dumps(record,indent=2,allow_nan=False)+'\n')
            error=NativeFailure('native transport failed: '+repr(e));error.native_certificate=record;raise error from e
    def close(self):
        if self.closed:return
        self.closed=True;tail='';failure=None
        try:
            self.child.stdin.close();tail=self.child.stdout.read();code=self.child.wait()
            require(code==0 and not tail,'native child completion failure')
        except BaseException as e:
            failure=repr(e);code=self.child.poll()
        finally:self.stderr.close()
        record={'schema':'electry-native-full-flow-transport-completion/v1','implementation_manifest_sha256':self.manifest_sha,'pointer':self.pointer,'acknowledged_nonempty_equations':self.sequence,'child_returncode':code,'unconsumed_stdout':tail,'failure':failure,'PASS':failure is None,'no_retries_restarts_or_force_repairs':True}
        path=self.output/'native-transport-completion-v1.json';require(not path.exists(),'native completion already recorded');path.write_text(json.dumps(record,indent=2,allow_nan=False)+'\n')
        require(failure is None,'native child failed to close; no complete qualification')
        return record
def start():
    global _session
    require(_session is None,'native session already exists')
    from gate_v1 import require_review
    manifest,pointer=require_review();_session=Session(manifest,pointer)
def finish():
    global _session
    require(_session is not None,'native session not started')
    session=_session;_session=None;return session.close()
def abort_close():
    if _session is not None:return finish()
def solve(A_float,d0_float,dfree_float,K_float,legacy_force):
    """One native request, exact diagnostics and ordinary NumPy rail, no repair."""
    import numpy as np
    J=len(d0_float)
    require(float(K_float)==6000.,'hard-coded native stiffness mismatch')
    if J==0:return np.zeros(0),{'success':True,'analytic_zero':True,'nfev':0,'residual_inf_N':0.,'message':'original analytic empty equation, no native call','certificate':None}
    require(_session is not None,'fresh authorized native session not started')
    require(1<=J<=2 and A_float.shape==(J,J) and len(dfree_float)==J,'native equation dimension')
    require(np.isfinite(A_float).all() and np.isfinite(d0_float).all() and np.isfinite(dfree_float).all(),'nonfinite new native equation')
    native=_session.request(A_float,d0_float,dfree_float)
    f=np.array([value(x) for x in native['force'][:J]],dtype=np.float64)
    certificate={'schema':'electry-native-Tlower-applied-equation/v1','target':'exact binary64 stored A/d0/dfree and K6000; actual new physical step','algorithm':'immutable strict native interval T.lower, no nearest-rounding history','native_result':native,'equation_raw':{'A':[[word(x) for x in row] for row in A_float],'d0':list(map(word,d0_float)),'dfree':list(map(word,dfree_float))},'nfev_semantics':'native map_evaluations, not rational-map/SciPy history','native_interval_checker_PASS':False,'native_interval_checker_failure':None}
    if not np.isfinite(f).all():
        error=NativeFailure('native returned nonfinite force, no valid physical endpoint');error.native_certificate=certificate;raise error
    # Preserve the actual exact stored-equation residual for unchanged Gram/gather
    # diagnostics; do not rename the outward interval Rcert an exact value.
    K=rational(K_float);A=[[rational(x) for x in row] for row in A_float];d0=list(map(rational,d0_float));df=list(map(rational,dfree_float));fr=list(map(rational,f))
    target=[dg(a,b-sum((x*y for x,y in zip(row,fr)),Fraction(0)),K) for row,a,b in zip(A,d0,df)]
    exact_res=maximum(abs(x-y) for x,y in zip(fr,target))
    try:_session.checker.check(A_float.tolist(),d0_float.tolist(),dfree_float.tolist(),native);certificate['native_interval_checker_PASS']=True
    except Exception as e:certificate['native_interval_checker_failure']=repr(e)
    # This is the actual original NumPy callback from unchanged mechanics.
    ordinary=float(np.max(abs(f-legacy_force(d0_float,dfree_float-A_float@f)[0])))
    ordinary_pass=math.isfinite(ordinary) and ordinary<=2e-11
    finite_nonnegative=bool(np.isfinite(f).all() and np.all(f>=0))
    exact_pass=exact_res<=TAU
    success=bool(native['status']==0 and native['exact_stored_residual_certified'] and native['cpp_legacy_diagnostic_pass'] and native['posterior_error_bound_valid'] and certificate['native_interval_checker_PASS'] and exact_pass and ordinary_pass and finite_nonnegative)
    failures=[]
    if native['status']!=0:failures.append('native status '+str(native['status']))
    if not certificate['native_interval_checker_PASS']:failures.append('independent exact native interval check failed')
    if not exact_pass:failures.append('exact stored residual rail failed')
    if not ordinary_pass:failures.append('actual original NumPy ordinary residual rail failed')
    if not finite_nonnegative:failures.append('native candidate not finite/nonnegative')
    certificate.update(exact_residual_inf_N=pair(exact_res),exact_residual_PASS=exact_pass,exact_posterior_diagnostic_N=pair(exact_res/(1-NATIVE_LAMBDA)),actual_original_numpy_residual_N=ordinary if math.isfinite(ordinary) else None,actual_original_numpy_residual_raw=word(ordinary),actual_original_numpy_residual_PASS=ordinary_pass,finite_nonnegative_force=finite_nonnegative,success=success,failure=failures or None,scope='Stored equation certificate; actual full applied field/work admission remains separate.')
    status={'success':success,'analytic_zero':native['analytic_zero_solution'],'nfev':native['map_evaluations'],'residual_inf_N':ordinary,'message':'native interval T.lower plus actual NumPy rail' if success else '; '.join(failures),'negative_force_count':int(sum(f<0)),'force_below_negative_1e_minus12_N_count':int(sum(f< -1e-12)),'minimum_f_N':float(min(f)),'certificate':certificate}
    return f,status
