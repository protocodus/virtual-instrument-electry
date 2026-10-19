# Capacity-five stored-equation rounding and cap review

This is an independent **algebra and core-source review**, with no compile or
execution authority. It evaluates no matrix, force, budget, cap, state, package,
native kernel or recording. It binds the source snapshot SHA256
`5a12530a3f93e1da2835f01721ff027a8b0ece212be4b03d83944b59b3a42ac5` from
`build-multicontact-passive-kernel/contact_interval_j5_draft_v1.hpp`. The new
map, budget, cap, ordinary residual and solve graph have been statically read.
The unchanged primitive/normalized-scalar block is byte-identical to the old
J<=2 header. This does not transfer the old global lambda/ceiling525 or any
physical qualification to the new implementation.

## Exact target and contraction

For exact real values of the stored binary64 inputs, keep

```
T_i(f) = g(d0_i, dfree_i - sum_(j=0..J-1) A_ij*f_j),
U(d) = (K/2)*max(d,0)^2, K=6000 exactly, 0<=J<=5.
```

The continuous discrete gradient has these exact branches:

* both endpoints nonpositive: zero;
* both positive: `(K/2)*(a+b)`;
* one positive p and other nonpositive -c: `(K/2)*p^2/(p+c)`.

For fixed a, the derivative with respect to b is between0 and K/2, including
the corresponding one-sided limits. In the mixed regions the normalized
derivatives are `a^2/(a-b)^2` when a>0,b<0 and
`1-a^2/(b-a)^2` when a<0,b>0. Thus the global Lipschitz bound holds across
the branch boundaries; it does not require dividing two nearby potentials.

Compute an outward **input-specific** bound

```
L_U >= (K/2)*max_i sum_j abs(A_ij),    finite 0<=L_U<1.
```

All row terms must be included. This gives a contraction on the complete
nonnegative cone (indeed on all real force vectors), with one unique fixed
point. No symmetry, PSD, inverse or full rank is needed for this stored-target
argument. Physical Gram construction, passive field/storage and actual work
remain separately owned. Signed cross terms must not be removed to pass L_U.
Failure of an upper row bound is a rejected certificate, not a proof of
physical nonpassivity.

Let `a_U >= ||T(0)||inf`, obtained from the same enclosing map. With
`d_L <= 1-L_U`, d_L>0, select a finite outward
`R >= a_U/d_L`. Then exact T maps `[0,R]^J` into itself. A nonnegative
enclosure lower endpoint satisfies `0<=T.lower(f)<=T(f)<=R`; selecting it
preserves this box without clipping forces. An initial `a_U==0` proves the
unique zero fixed point. J0 is a distinct empty-system case.

## J-term endpoint excess

The strict host contract remains correctly rounded binary64 primitives,
gradual underflow, no FMA/reassociation/excess precision, finite outward
neighbors and the entry/exit arithmetic-environment checks. For a widened
point primitive, the old derivation supplies

```
e(X) = 4*u*X + 2*sigma, u=2^-53, sigma=2^-1074.
```

Every operation used to construct an upper budget is itself outward. This is
not ordinary nearest arithmetic applied to a symbolic error formula. A finite
result whose outward neighbor is infinite is rejected. Sigma/2 appears only
in the proof; it must not be encoded as a positive binary64 constant.

For a row and the exact sequential reduction define, with all right-hand
nonnegative bounds rounded upward,

```
s_l = sum_(j=1..l) abs(A_ij),   S_l = R*s_l,
p_l = e(abs(A_il)*R),
E_1 = p_1,
E_l = E_(l-1) + p_l + e(S_l + E_(l-1) + p_l), l>=2,
Egap_i = E_J + e(abs(dfree_i) + S_J + E_J).
```

Proof by induction: the previous partial interval differs from its exact
partial sum by at most Eprev per endpoint; the new product contributes p_l.
The exact sum of those stored endpoints has magnitude at most
S_l+Eprev+p_l, so one outward interval addition adds the stated e term.
The final subtraction has magnitude at most |dfree|+S_J+E_J. Absolute operand
bounds retain cancellation cases. A reduction initialized at the first
product has J products and J-1 additions. A zero-seed reduction is equivalent
only if its initial addition is the exact identity shortcut; otherwise count
that extra primitive. S_l is already multiplied by R: do not multiply it by R
again. A reordered/tree/FMA reduction requires its own matched recurrence.

The exact gap is enclosed with endpoint excess Egap and diameter<=2Egap.
The finite stored endpoint envelope is

```
P >= max_i(abs(d0_i), abs(dfree_i)+S_J+Egap_i),
Q >= (K/2)*P,     Q2 >= 2*Q.
```

P bounds the stored gap endpoints sent to scalar g, not only the ideal gap.
Q/Q2 must be stored outward values; a mathematically unrepresentable product
cannot be treated as an exact range cap.

## Scalar and residual budgets (unchanged graph)

The normalized scalar branches use t in[0,1], d=1+t in[1,2], r in[0,1].
Intersecting with these proved ranges preserves the target. The old scalar
argument is dimension independent when its primitive graph is unchanged:

```
et = e(1), ed = et+e(2), er = et+ed+e(1),
es = P*er+e(P),
emixed = (K/2)*es+e(Q),
epos = 2*e(Q)+e(Q2), eg = max(emixed,epos),
W >= max_i(K*Egap_i + 2*eg).
```

The mixed quotient perturbation uses denominator>=1 and numerator<=1. The
two exact endpoint g values differ by at most `(K/2)*2Egap`; their scalar
enclosures add at most eg each. Hence W bounds the force interval's actual
endpoint diameter. It also bounds the T.lower update error.

The existing runtime width check outwardly subtracts hi-lo. That measured
upper can conservatively exceed W at a boundary even if the exact diameter
is<=W. Retaining this rejecting check is safe. Do not claim a finite-L_U,
positive-slack input is guaranteed to avoid every width/range rejection.

For f in[0,R], exact T is in[0,R], and an admitted enclosure lies in[0,R+W].
The widened residual subtraction therefore satisfies

```
C >= W + e(R+W),
Rcert(f) <= ||f-T(f)||inf + C.
```

Independently, direct residual interval containment proves
`||f-T(f)||inf<=Rcert`; this latter implication supplies actual certification.
The remaining cap calculation is a termination sufficiency argument, not a
substitute for that direct final test.

## Generic log-free sufficient cap

Write r_k=||f_k-T(f_k)||inf. Because the selected lower map has error<=W,

```
r_(k+1) <= L_U*r_k + (1+L_U)*W,
r_k <= L_U^k*a_U + b*(1-L_U^k),
b >= (1+L_U)*W/(1-L_U).
```

Construct b upward using d_L in the denominator, C upward, and the positive
slack `S_L <= tau-b-C` downward, where tau is the unchanged exact stored
binary64 2e-11 N rail. Reject nonfinite bounds or S_L<=0.

The proposed source may use this bounded, input-only algorithm before any
force update:

```
q_0 = a_U
if q_0<=S_L: kcap=0
else for k=1..ADMIN_LIMIT:
    q_k = outward_upper_multiply(L_U,q_(k-1))
    reject a nonfinite primitive/neighbor
    if q_k<=S_L: kcap=k; stop
otherwise: explicit administrative-cap rejection
```

Induction proves `q_k >= L_U^k*a_U`. A derived finite kcap with q_k<=S_L
therefore yields `Rcert(f_kcap)<=tau` provided all intermediate environment,
range/box/width checks remain admitted. No logarithm, rounded exponentiation,
old lambda<1/16 assumption or old525 ceiling is needed.

With L_U0, the exact-zero multiplication shortcut gives q1=0; a0 and cap0
remain separate cases. For L_U near1, upward rounding may stagnate/increase;
the exact contraction does not guarantee this rounded envelope reaches slack
within a chosen administrative limit. Such an input must reject visibly,
never use the work ceiling as if it were a proved cap, and never select a
different L_U/timestep/force after outcomes.

The kernel owner's prospective ADMIN_LIMIT4096 is a resource policy, not a
mathematical convergence constant. At most4096 q-products and at most4096
force updates are allowed; a cap failure happens before force iteration.
Caller-owned trace capacity4097 stores f0 through f_kcap. Nonempty success
has `map_evaluations=trace_rows+1` if the initial a_U map is counted separately;
the maximum is4098 maps including that initial map. Check counter/storage
overflow and reject dimension>5 before indexing. A count of enclosure
primitives is not total FP cost or a real-time bound.

## Independent final obligations

* Keep exact stored residual<=original tau, posterior upper
  `Rcert/d_L`, finite/nonnegative candidates, all input-specific row sums,
  ranges, width, slack, cap products and every selected T.lower transition.
* Keep the separate ordinary C++ diagnostic and actual original NumPy
  callback rail. The cap theorem covers the exact stored target only; an
  ordinary diagnostic failure is not repaired with further updates.
* A kernel certificate does not certify the construction/applied endpoint,
  full coupled q/v, dissipation, contact storage or original5e-10 work rail.
* No physical stiffness/material/friction, new geometry or performance
  parameters are supplied by this proof. A unit K=6000 law remains a fixture.

Review status: the stated algebra and reviewed core source agree. The sole
concrete source comment is closed: `record_exit_environment` now clears the
new `budgets.cap_bound_certified` flag along with the other arithmetic-derived
qualification flags; raw budgets stay diagnostic. Driver/transport, final
implementation manifest, compiler contract and independent exact-rational
qualification still require their own binding/review and fresh root authority.
No compile/execution/physical/RT approval is given here.


## Source-count bound, not a timing result

For this snapshot, one scalar endpoint uses at most15 counted enclosing Ops
(the mixed branch); one map therefore uses at most
`M(J)=J*(3J+30)` counted Ops. Each main residual iteration adds at most3J
Ops. Preparation excluding the cap-products uses at most
`14*J^2+33*J+36` counted Ops, including the initial map. Thus a conservative
whole-call bound is

```
14*J^2+33*J+36 + 4096
    + 4097*(J*(3*J+30)+3*J) + 1
```

for J<=5. Identity branches and early failure reduce these counts. The bound
only counts `Ops.count` enclosures. It excludes ordinary legacy RN operations,
neighbor bitwork, comparisons, environment access and all caller activity. It
is not total floating-point work, allocation cost or a real-time/time bound.
The generic loops/counters remain bounded without tuning to iteration results.
