"""UNEXECUTED fixed algebraic test-input recipes, not guitar/finger calibration."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from fractions import Fraction as F
from itertools import permutations

def word(x,down=False):
    from oracle_draft_v1 import rn_positive_word,qbits
    sign=0x8000000000000000 if x<0 else 0;x=abs(x)
    u=rn_positive_word(x)
    if down and qbits(u)>x:u-=1
    return f'{u|sign:016x}'
def request(case_id,j,A,d0,df,expected='accept',scope='algebraic-only'):
    matrix=[['0000000000000000']*5 for _ in range(5)];a=['0000000000000000']*5;b=a.copy()
    for i in range(j):
        a[i]=word(d0[i]);b[i]=word(df[i])
        for k in range(j):matrix[i][k]=word(A[i][k])
    return dict(case_id=case_id,j=j,A=matrix,d0=a,dfree=b,expected=expected,scope=scope)
def case_families(j):
    s=F(2)**-10;small=F(2)**-20
    zero=[[F(0)]*j for _ in range(j)]
    v=[F(-1 if i%2 else 1) for i in range(j)]
    positive=[s*(i+1) for i in range(j)];reverse=[s*(j-i) for i in range(j)]
    rank=[[F(2)**-21]*j for _ in range(j)]
    dense=[[F(2)**-22*((i==k)+v[i]*v[k]) for k in range(j)] for i in range(j)]
    diagonal=[[F(2)**-20*(i==k) for k in range(j)] for i in range(j)]
    gram=[[F(2)**-22*v[i]*v[k] for k in range(j)] for i in range(j)]
    directed=[[(-F(2)**-20 if i==k else F(0))+(F(2)**-21 if k==(i+1)%j else F(0)) for k in range(j)] for i in range(j)]
    rows=[
      ('coupled-rank1-positive',rank,positive,reverse),
      ('coupled-dense-spd',dense,positive,reverse),
      ('diagonal-positive',diagonal,positive,reverse),
      ('signed-gram-mixed',gram,[s]*j,[2*s*v[i] for i in range(j)]),
      ('nonsymmetric-stored-only',directed,positive,reverse),
      ('broad-local-contraction',[[F(2)**-14]*j for _ in range(j)],[small]*j,[small*(i+1) for i in range(j)]),
      ('zero-operator-positive',zero,positive,reverse),
      ('zero-operator-equal-positive',zero,[s]*j,[s]*j),
      ('positive-to-negative',zero,[s]*j,[-2*s]*j),
      ('negative-to-positive',zero,[-2*s]*j,[s]*j),
      ('open',zero,[-s]*j,[-s]*j),
      ('signed-zero',zero,[F(0)]*j,[F(0)]*j),
      ('subnormal-cross',zero,[F(2)**-1074]*j,[-F(2)**-1074]*j)]
    out=[]
    for name,A,d0,df in rows:
        x=request(f'J{j}/{name}',j,A,d0,df)
        if name=='signed-zero':x['d0'][:j]=['8000000000000000' if i%2 else '0000000000000000' for i in range(j)]
        out.append(x)
    if j<5:
        x=request(f'J{j}/inactive-padding-poison',j,rank,positive,reverse)
        for i in range(5):
            if i>=j:x['d0'][i]='7ff8000000000001';x['dfree'][i]='fff0000000000000'
            for k in range(5):
                if i>=j or k>=j:x['A'][i][k]='7ff0000000000000'
        out.append(x)
    return out

def registered_cases():
    """Future authorized call only; order fixed before any results."""
    out=[];base={}
    for j in (3,5,4,0,1,2):
        if j:
            block=case_families(j);out.extend(block);base[j]=next(x for x in block if x['case_id'].endswith('coupled-dense-spd'))
        else:
            x=request('J0/empty',0,[],[],[]);out.append(x)
            x=request('J0/ignored-nonfinite-padding',0,[],[],[])
            x['A']=[['7ff8000000000001']*5 for _ in range(5)];x['d0']=['fff0000000000000']*5;x['dfree']=['7ff0000000000000']*5;out.append(x)
    for j in (3,5,4):
        b=base[j]
        for p in permutations(range(j)):
            x={**b,'case_id':f'J{j}/permutation/'+''.join(map(str,p)),
               'A':[['0000000000000000']*5 for _ in range(5)],'d0':['0000000000000000']*5,'dfree':['0000000000000000']*5,
               'permutation':list(p),'permutation_base_case_id':b['case_id']}
            for i in range(j):
                x['d0'][i]=b['d0'][p[i]];x['dfree'][i]=b['dfree'][p[i]]
                for k in range(j):x['A'][i][k]=b['A'][p[i]][p[k]]
            out.append(x)
    def neg(name,j=3):
        x=request('negative/'+name,j,[[F(0)]*j for _ in range(j)],[F(2)**-10]*j,[F(2)**-10]*j,expected='reject');out.append(x);return x
    for d in (6,4294967295):x=neg('dimension-'+str(d));x['j']=d;x['expected_status']='dimension'
    x=neg('active-third-A-NaN');x['A'][2][2]='7ff8000000000001';x['expected_status']='nonfinite_input'
    x=neg('active-third-d0-NaN');x['d0'][2]='7ff8000000000001';x['expected_status']='nonfinite_input'
    x=neg('active-fifth-dfree-Inf',5);x['dfree'][4]='7ff0000000000000';x['expected_status']='nonfinite_input'
    x=neg('active-fifth-A-negative-Inf',5);x['A'][4][0]='fff0000000000000';x['expected_status']='nonfinite_input'
    x=neg('noncontractive-operator');x['A'][0][0]=word(F(2)**-10);x['expected_status']='operator_bound';x['expected_exact_noncontraction']=True
    x=neg('maxfinite-positive-endpoint');x['d0'][0]=x['dfree'][0]='7fefffffffffffff';x['expected_status']='nonfinite_primitive';x['expected_branch']='positive_initial_scalar_overflow'
    x=neg('near-unit-cap-resource')
    a=(1-F(2)**-12)/3000
    for i in range(3):x['A'][i][i]=word(a,down=True);x['d0'][i]=x['dfree'][i]=word(F(2)**-42)
    x['expected_status']='cap_resource_limit';x['scope']='numerical administrative cap stress, not a material/contact constant'
    for n,x in enumerate(out):x['id']=n
    return out

if __name__=='__main__':raise SystemExit('DRAFT input recipes only: no generation, solver or binary call authorized')
