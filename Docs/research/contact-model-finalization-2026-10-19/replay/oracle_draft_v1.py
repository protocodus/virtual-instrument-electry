"""UNEXECUTED dimension-general exact stored-equation checker; never solves a force.

Only functions are defined. The future exact-authority wrapper owns raw transport,
package gates, saved inputs and compressed evidence. No producer module imports.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from fractions import Fraction as F
import struct

STATUS_NAMES=(
    'stored_certificate_and_cpp_diagnostic_pass','empty_system','unsupported_host',
    'arithmetic_environment','dimension','nonfinite_input','invalid_interval',
    'nonpositive_denominator','nonfinite_primitive','nonfinite_neighbor',
    'operator_bound','endpoint_envelope','invalid_budget','nonpositive_slack',
    'cap_resource_limit','trace_capacity','negative_or_nonfinite_candidate',
    'force_box','interval_width','exhausted_cap','nonfinite_cpp_diagnostic',
    'cpp_legacy_residual')
BUDGET_VECTORS=('row_sum_upper','dot_excess_upper','gap_excess_upper')
BUDGET_SCALARS=('local_lambda_upper','a_upper','one_minus_lambda_lower','box_R',
    'endpoint_P','scaled_Q','scaled_Q2','scalar_excess_upper','width_W',
    'recurrence_b_upper','residual_C_upper','slack_lower','geometric_upper_at_cap')
FLAGS=('analytic_zero_solution','exact_stored_residual_certified',
    'cpp_legacy_diagnostic_pass','posterior_error_bound_valid',
    'original_numpy_legacy_bit_identity_owned','physical_endpoint_admitted')

def need(ok,label):
    if not ok:raise ValueError(label)
def bits(word):
    need(type(word) is str and len(word)==16 and all(c in '0123456789abcdef' for c in word),'raw binary64 word')
    return int(word,16)
def finite_word(word):return (bits(word)>>52)&2047!=2047
def words(xs):
    need(type(xs) is list and len(xs)==5,'five stored lanes')
    for x in xs:bits(x)
def iteration_shape(t):
    words(t['force'])
    for name in ('mapped','residual'):
        need(type(t[name]) is list and len(t[name])==5,'five interval lanes')
        for p in t[name]:
            need(type(p) is list and len(p)==2,'stored interval pair')
            for x in p:bits(x)
    for name in ('certificate_residual','measured_width_upper'):bits(t[name])
    need(type(t['updates']) is int and 0<=t['updates']<=4096,'iteration update domain')
def schema_check(req,r):
    need(r['schema']=='electry-multicontact-interval-kernel-result/v1','exact native schema')
    need(type(r['maximum_contacts']) is int and r['maximum_contacts']==5 and
         type(r['administrative_maximum_updates']) is int and r['administrative_maximum_updates']==4096,'native capacity constants')
    need(type(req['id']) is int and 0<=req['id']<2**64 and type(r['id']) is int and r['id']==req['id'],'raw request-response identity')
    need(type(req['j']) is int and 0<=req['j']<2**32,'unsigned request dimension')
    for x in (req,r['input']):
        need(type(x['A']) is list and len(x['A'])==5,'five matrix rows')
        for row in x['A']:words(row)
        words(x['d0']);words(x['dfree'])
    need(type(r['input']['j']) is int and r['input']=={k:req[k] for k in ('j','A','d0','dfree')},'all35 raw echoed equation words and J')
    s=r['status'];need(type(s) is int and 0<=s<len(STATUS_NAMES) and r['status_name']==STATUS_NAMES[s],'native status enum-name correspondence')
    for name in FLAGS:need(type(r[name]) is bool,'native boolean '+name)
    words(r['force']);b=r['budgets']
    for name in BUDGET_VECTORS:words(b[name])
    for name in BUDGET_SCALARS:bits(b[name])
    need(type(b['cap_bound_certified']) is bool,'cap flag boolean')
    for name in ('cap_products','cap_updates'):need(type(b[name]) is int and 0<=b[name]<=4096,'cap counter domain')
    for name in ('primitive_operations','entry_fpcr','exit_fpcr','map_evaluations','actual_updates','trace_used'):
        need(type(r[name]) is int and r[name]>=0,'nonnegative native counter '+name)
    need(r['map_evaluations']<=4098 and r['actual_updates']<=4096,'native map/update resource bounds')
    need(type(r['trace']) is list and r['trace_used']==len(r['trace'])<=4097,'trace_used owns complete emitted trace')
    iteration_shape(r['final_iteration'])
    for t in r['trace']:iteration_shape(t)
    for name in ('posterior_error_upper','cpp_legacy_residual'):bits(r[name])
def inactive_zero_check(j,r):
    need(all(bits(w)==0 for w in r['force'][j:]),'inactive returned forces are positive zero')
    for name in BUDGET_VECTORS:need(all(bits(w)==0 for w in r['budgets'][name][j:]),'inactive budget lanes are positive zero')
    for t in [r['final_iteration'],*r['trace']]:
        need(all(bits(w)==0 for w in t['force'][j:]),'inactive iteration forces are positive zero')
        for name in ('mapped','residual'):
            need(all(all(bits(w)==0 for w in p) for p in t[name][j:]),'inactive map/residual lanes are positive zero')
def qbits(u):
    e=(u>>52)&2047;m=u&((1<<52)-1)
    need(e!=2047,'finite exact binary64')
    if e:m|=1<<52;power=e-1023-52
    else:power=-1074
    x=F(m)*(F(2)**power)
    return -x if u>>63 else x
def qw(word):return qbits(bits(word))
def norm(xs):return max(map(abs,xs),default=F(0))
def rail():return qw('3db5fd7fe1796495')
def secant(a,b):
    # U(d)=3000*max(d,0)^2; this is the exact potential quotient.
    if a==b:return 6000*max(a,F(0))
    return 3000*(max(b,F(0))**2-max(a,F(0))**2)/(b-a)
def target(A,d0,df,f):
    return [secant(a,b-sum((c*x for c,x in zip(row,f)),F(0))) for row,a,b in zip(A,d0,df)]
def decode_request(req):
    j=req['j'];need(type(j) is int and 0<=j<=5,'active J0..5')
    need(len(req['A'])==len(req['d0'])==len(req['dfree'])==5 and all(len(r)==5 for r in req['A']),'full capacity5 raw input storage')
    return j,[[qw(x) for x in row[:j]] for row in req['A'][:j]],[qw(x) for x in req['d0'][:j]],[qw(x) for x in req['dfree'][:j]]
def pair(x):return [x.numerator,x.denominator]
def rn_positive_word(x):
    """Exact positive Fraction -> RN-even binary64 via integer-word search.

    No host float/log/libm rounding assumption is used. For cap products the
    operands are nonnegative and LU<1, so the exact product is finite-range.
    """
    need(x>=0 and x<=qbits(0x7fefffffffffffff),'finite nonnegative cap product')
    low,high=0,0x7fefffffffffffff
    while low<high:
        mid=(low+high+1)//2
        if qbits(mid)<=x:low=mid
        else:high=mid-1
    if qbits(low)==x:return low
    upper=low+1;dl=x-qbits(low);du=qbits(upper)-x
    return low if dl<du or (dl==du and low%2==0) else upper

def up_mul(a,b):
    # Exactly the declared positive Ops.upper_mul graph, including zero identity.
    if a==0 or b==0:return F(0)
    n=rn_positive_word(a*b)+1
    need(n<=0x7fefffffffffffff,'finite upper neighbor in cap')
    return qbits(n)
def up_add(a,b):
    if a==0:return b
    if b==0:return a
    n=rn_positive_word(a+b)+1
    need(n<=0x7fefffffffffffff,'finite upper neighbor in nonnegative sum')
    return qbits(n)

def cap_check(v,b,resource_rejection=False):
    a,lu,s=v['a_upper'],v['local_lambda_upper'],v['slack_lower']
    need(a>0 and 0<=lu<1 and s>0,'cap prerequisites')
    q=a;k=0
    while q>s and k<4096:q=up_mul(lu,q);k+=1
    need(k==b['cap_updates']==b['cap_products'],'exact first outward cap count')
    need(q==v['geometric_upper_at_cap'],'exact outward cap endpoint')
    need(lu**k*a<=q,'geometric enclosure')
    if resource_rejection:
        need(k==4096 and q>s and b['cap_bound_certified'] is False,'explicit cap resource rejection')
    else:
        need(q<=s and b['cap_bound_certified'] is True,'certified first crossing')
        need(lu**k*a<=s,'direct exact geometric cap proof')
    return k

def budget_check(A,d0,df,r,resource_rejection=False):
    j=len(d0);b=r['budgets'];v={k:qw(x) for k,x in b.items() if isinstance(x,str)}
    for k in ['row_sum_upper','dot_excess_upper','gap_excess_upper']:need(len(b[k])==5,'capacity5 budget vector')
    rows=[sum(map(abs,row),F(0)) for row in A]
    need(all(qw(b['row_sum_upper'][i])>=rows[i] for i in range(j)),'row norm enclosure')
    lu=v['local_lambda_upper'];need(3000*max(rows,default=F(0))<=lu<1,'actual local contraction')
    actual_a=norm(target(A,d0,df,[F(0)]*j));need(v['a_upper']>=actual_a,'initial map bound')
    if r['analytic_zero_solution']:
        need(actual_a==v['a_upper']==v['box_R']==v['endpoint_P']==v['width_W']==0,'analytic zero ownership')
        need(b['cap_updates']==b['cap_products']==0 and b['cap_bound_certified'] is True and v['geometric_upper_at_cap']==0,'analytic zero cap')
        return v,0
    D,R,P,W=v['one_minus_lambda_lower'],v['box_R'],v['endpoint_P'],v['width_W']
    need(0<D<=1-lu and R>=v['a_upper']/D,'invariant force box')
    err=lambda x:F(1,2**51)*x+F(2,2**1074)
    for i,row in enumerate(A):
        prefix=abs(row[0]);dot=err(prefix*R)
        for a in row[1:]:
            prefix+=abs(a);ep=err(abs(a)*R);summed=dot+ep
            dot=summed+err(prefix*R+summed)
        saved_dot=qw(b['dot_excess_upper'][i]);need(saved_dot>=dot,'full J-term sequential dot excess')
        sr=qw(b['row_sum_upper'][i])*R;endpoint=abs(df[i])+sr;gap=qw(b['gap_excess_upper'][i])
        need(gap>=saved_dot+err(endpoint+saved_dot),'gap excess')
        need(P>=max(abs(d0[i]),endpoint+gap),'endpoint envelope')
    Q,Q2=v['scaled_Q'],v['scaled_Q2'];need(Q>=3000*P and Q2>=2*Q,'scalar finite ranges')
    et=err(F(1));ed=et+err(F(2));er=et+ed+err(F(1));es=P*er+err(P)
    scalar=max(3000*es+err(Q),2*err(Q)+err(Q2))
    need(v['scalar_excess_upper']>=scalar,'normalized scalar error graph')
    need(all(W>=6000*qw(b['gap_excess_upper'][i])+2*v['scalar_excess_upper'] for i in range(j)),'width envelope')
    need(v['recurrence_b_upper']>=(1+lu)*W/D,'local recurrence floor')
    need(v['residual_C_upper']>=W+err(R+W),'residual enclosure arithmetic')
    need(0<v['slack_lower']<=rail()-v['recurrence_b_upper']-v['residual_C_upper'],'unchanged residual rail slack')
    k=cap_check(v,b,resource_rejection)
    if not resource_rejection:need(lu**k*v['a_upper']+v['recurrence_b_upper']+v['residual_C_upper']<=rail(),'cap sufficiency at unchanged rail')
    return v,k

def check_claimed_certificate(req,r):
    """Verify claims even if separate ordinary-CPP/global admission later fails."""
    schema_check(req,r)
    j,A,d0,df=decode_request(req)
    need(r['original_numpy_legacy_bit_identity_owned'] is False and r['physical_endpoint_admitted'] is False,'stored-only scope')
    need(r['entry_fpcr']==r['exit_fpcr']==0,'certified environment')
    need(len(r['force'])==5,'full returned capacity5')
    inactive_zero_check(j,r)
    if j==0:
        need(r['status']==1 and r['analytic_zero_solution'] and not r['trace'] and r['actual_updates']==r['map_evaluations']==0,'empty no-map path')
        need(all(bits(w)==0 for w in r['force']) and qw(r['posterior_error_upper'])==qw(r['cpp_legacy_residual'])==0,'empty exact output')
        need(r['exact_stored_residual_certified'] and r['posterior_error_bound_valid'] and r['cpp_legacy_diagnostic_pass'],'empty flags')
        return dict(exact_residual=pair(F(0)),trace_rows=0,cap=0,globally_admitted=True)
    v,k=budget_check(A,d0,df,r)
    trace=r['trace'];need(len(trace)==r['actual_updates']+1 and r['map_evaluations']==len(trace)+1,'complete trace and initial-map count')
    need(0<=r['actual_updates']<=k<=4096,'bounded actual updates')
    previous=['0000000000000000']*j
    for n,t in enumerate(trace):
        need(t['updates']==n and len(t['force'])==len(t['mapped'])==len(t['residual'])==5,'full indexed trace')
        need(t['force'][:j]==previous,'raw lower-endpoint transitions')
        need(all(bits(w)==0 for w in t['force'][j:]),'inactive force lanes remain fresh zero')
        f=[qw(w) for w in t['force'][:j]];need(all(0<=x<=v['box_R'] for x in f),'force invariant box')
        g=target(A,d0,df,f);cert=width=F(0)
        for i in range(j):
            lo,hi=map(qw,t['mapped'][i]);rl,rh=map(qw,t['residual'][i])
            need(0<=lo<=g[i]<=hi,'exact component map enclosure')
            need(rl<=f[i]-hi<=f[i]-g[i]<=f[i]-lo<=rh,'exact residual enclosure')
            cert=max(cert,abs(rl),abs(rh));width=max(width,hi-lo)
        need(width<=qw(t['measured_width_upper'])<=v['width_W'],'actual/measured/fixed width')
        need(qw(t['certificate_residual'])==cert,'certificate infinity norm')
        need(cert>rail() if n+1<len(trace) else cert<=rail(),'first certified stopping, no extra iterations')
        previous=[x[0] for x in t['mapped'][:j]]
    need(trace[-1]==r['final_iteration'] and trace[-1]['force']==r['force'],'returned final trace identity')
    f=[qw(w) for w in r['force'][:j]];res=norm([x-y for x,y in zip(f,target(A,d0,df,f))])
    need(res<=qw(trace[-1]['certificate_residual'])<=rail(),'exact stored residual2e-11')
    if r['posterior_error_bound_valid']:
        post=qw(r['posterior_error_upper'])
        need(post>=res/(1-v['local_lambda_upper']),'actual-root posterior bound')
        if not r['analytic_zero_solution']:need(post>=qw(trace[-1]['certificate_residual'])/v['one_minus_lambda_lower'],'reported posterior graph')
        else:need(post==0,'analytic zero posterior')
    admitted=r['status']==0
    if admitted:
        need(r['posterior_error_bound_valid'] and r['cpp_legacy_diagnostic_pass'],'all global native flags')
        need(0<=qw(r['cpp_legacy_residual'])<=rail(),'separate ordinaryCPP2e-11 rail')
    return dict(exact_residual=pair(res),trace_rows=len(trace),cap=k,updates=r['actual_updates'],globally_admitted=admitted)

def rejection_check(req,r):
    """Prove the registered rejection branch, not merely any unsuccessful status."""
    s=r['status']
    need(not r['exact_stored_residual_certified'] and not r['posterior_error_bound_valid'] and
         not r['cpp_legacy_diagnostic_pass'] and not r['budgets']['cap_bound_certified'],'registered preparation/entry rejection has no accepted claims')
    need(not r['trace'] and r['actual_updates']==0 and all(bits(x)==0 for x in r['force']),'registered rejection precedes force updates')
    if s==4:
        need(req['j']>5 and r['map_evaluations']==r['primitive_operations']==0,'dimension guard before all operations/maps')
    elif s==5:
        j=req['j'];need(0<j<=5,'nonfinite active-input domain')
        active=req['d0'][:j]+req['dfree'][:j]+[w for row in req['A'][:j] for w in row[:j]]
        need(any(not finite_word(w) for w in active) and r['map_evaluations']==r['primitive_operations']==0,'active nonfinite guard before operations/maps')
    elif s==10:
        j,A,d0,df=decode_request(req);need(j>0 and r['map_evaluations']==0,'operator rejection before initial map')
        rows=[]
        for i,row in enumerate(A):
            q=abs(row[0])
            for x in row[1:]:q=up_add(q,abs(x))
            need(q==qw(r['budgets']['row_sum_upper'][i]),'exact outward row sum at operator rejection')
            rows.append(q)
        lu=max(up_mul(F(3000),q) for q in rows)
        need(lu==qw(r['budgets']['local_lambda_upper']) and lu>=1,'exact native operator-bound rejection')
        if req.get('expected_exact_noncontraction'):
            need(3000*max(sum(map(abs,row),F(0)) for row in A)>=1,'registered operator is exactly noncontractive')
    elif s==14:
        j,A,d0,df=decode_request(req)
        need(j>0 and r['map_evaluations']==1,'cap rejection after one initial map')
        v,k=budget_check(A,d0,df,r,resource_rejection=True)
        return dict(cap_resource_rejection_exactly_verified=True,cap=k)
    elif s==8:
        j,A,d0,df=decode_request(req)
        need(req.get('expected_branch')=='positive_initial_scalar_overflow' and j>0 and
             all(x==0 for row in A for x in row) and all(x>0 for x in d0+df),'registered initial-positive overflow branch')
        # Positive RN-even overflow starts at maxfinite + half its final ulp.
        need(any(3000*x>=F(2)**1024-F(2)**970 for x in d0+df) and
             r['map_evaluations']==1,'exact finite-range failure in the initial scalar map')
    elif s==3:
        need(req.get('expected_branch')=='entry_environment' and r['map_evaluations']==r['primitive_operations']==0,'registered environment rejection before map')
    else:
        raise ValueError('no registered independently checked rejection branch: '+r['status_name'])
    return dict(registered_rejection_branch_verified=True)

def check_saved_record(req,r,numpy_diagnostic=None):
    """Own schema, expectation and complete acceptance conjunction.

    The authorized wrapper supplies actual_numpy_residual's original-callback
    diagnostic for every expected accept. Missing/false diagnostics cannot admit.
    A truthful exact certificate on a CPP diagnostic failure is retained as a
    separate fact, never a qualified force or successful expected-accept result.
    """
    schema_check(req,r)
    need(r['original_numpy_legacy_bit_identity_owned'] is False and r['physical_endpoint_admitted'] is False,'all-status scope')
    expectation=req.get('expected','accept');need(expectation in ('accept','reject'),'registered expectation')
    if expectation=='reject':
        need(req.get('expected_status') in STATUS_NAMES and r['status_name']==req['expected_status'],'exact registered negative status')
    if r['status']==3:
        need(not r['exact_stored_residual_certified'] and not r['cpp_legacy_diagnostic_pass'] and
             not r['posterior_error_bound_valid'] and not r['budgets']['cap_bound_certified'],'environment failure clears claims')
    if r['status']==20:
        need(not finite_word(r['cpp_legacy_residual']) and not r['cpp_legacy_diagnostic_pass'],'nonfinite CPP diagnostic status owns its cause')
    elif r['status']==21:
        need(finite_word(r['cpp_legacy_residual']) and qw(r['cpp_legacy_residual'])>rail() and
             not r['cpp_legacy_diagnostic_pass'],'CPP residual failure remains outside global admission')
    if r['exact_stored_residual_certified']:
        facts=check_claimed_certificate(req,r)
    else:
        need(r['status'] not in (0,1),'accepted status lacks stored certificate')
        facts=dict(globally_admitted=False,native_status=r['status'],claimed_stored_certificate=False)
    if expectation=='reject':
        facts.update(rejection_check(req,r))
        facts.update(expected_behavior_PASS=True,stored_equation_qualification_PASS=False)
        return facts
    need(facts['globally_admitted'],'expected accept must have complete native global admission')
    need(type(numpy_diagnostic) is dict,'actual original NumPy diagnostic required for acceptance')
    value=qw(numpy_diagnostic['raw_binary64'])
    need(numpy_diagnostic['finite'] is True and numpy_diagnostic['rail_PASS'] is True and
         0<=value<=rail(),'actual original NumPy finite2e-11 rail')
    facts.update(expected_behavior_PASS=True,actual_original_numpy_residual_N=pair(value),stored_equation_qualification_PASS=True)
    return facts

def actual_numpy_residual(np,owned_original_dg,req,r):
    """Future post-authority injected original callback; no import or solve here."""
    j=req['j'];decode=lambda w:struct.unpack('>d',bytes.fromhex(w))[0]
    A=np.array([[decode(w) for w in a[:j]] for a in req['A'][:j]],dtype=np.float64)
    d0=np.array([decode(w) for w in req['d0'][:j]],dtype=np.float64)
    df=np.array([decode(w) for w in req['dfree'][:j]],dtype=np.float64)
    force=np.array([decode(w) for w in r['force'][:j]],dtype=np.float64)
    value=float(np.max(np.abs(force-owned_original_dg(d0,df-A@force)[0]))) if j else 0.0
    return dict(raw_binary64=struct.pack('>d',value).hex(),finite=bool(np.isfinite(value)),rail_PASS=bool(np.isfinite(value) and value<=2e-11))

def permutation_posterior_check(base_req,base_r,req,r):
    """Derived exact bound, never a chosen force-equality tolerance."""
    j=req['j'];p=req['permutation']
    need(sorted(p)==list(range(j)) and base_req['j']==j,'permutation domain')
    for i in range(j):
        need(req['d0'][i]==base_req['d0'][p[i]] and req['dfree'][i]==base_req['dfree'][p[i]],'exact permuted gaps')
        for k in range(j):need(req['A'][i][k]==base_req['A'][p[i]][p[k]],'exact permuted operator')
    # Each record is independently certified first. No old force equality is used.
    check_claimed_certificate(base_req,base_r);check_claimed_certificate(req,r)
    difference=norm([qw(r['force'][i])-qw(base_r['force'][p[i]]) for i in range(j)])
    bound=qw(base_r['posterior_error_upper'])+qw(r['posterior_error_upper'])
    need(difference<=bound,'permutation consistency from certified posterior sum')
    return dict(force_difference=pair(difference),derived_posterior_sum=pair(bound),bitidentity_not_required=True)

if __name__=='__main__':raise SystemExit('DRAFT functions only: future exact root+peer protocol/driver/input authorization required')
