Prospective controller and repick retirement review, 2026-10-19.

This is an algebraic source review, not an experiment or an observed failure
of the current 96-case source run. No E state, force trace, PCM, original
recording or archived trace was inspected. The current fresh-from-rest
qualification and its criteria remain unchanged.

The controller in `build-passive-source-tone-candidate-iofix/support_path_v1.py`
filters the stored pulse with coefficients `[float(1-a)]/[1,-a]`, accumulates
its output, and constructs `z = y_attach + D - 2*D*R`. Its original withdrawal
is defined only to the fixed recording's final knot. An indefinite event-local
implementation must declare the additional tail and cannot borrow a missing
right-hand slope from that final knot.

Let `a` be the exact real value of the stored binary pole, `beta` the exact
real value of the stored binary result `float(1-a)`, and `p[k]` the exact
stored pulse values. Assume the prospective stable domain `0 <= a < 1`,
zero initial filter state, finite pulse length and an unrounded real
recurrence `r[k] = beta*p[k] + a*r[k-1]`. Summing the recurrence gives

    R_infinity = beta / (1-a) * sum(p[k])
    z_infinity = y_attach + D - 2*D*R_infinity.

The actual domain and original pulse/pole bits still need checking for all 96
controller inputs. Stored pulse normalization must not be replaced by the
assertion that its exact real sum is one. Likewise `beta` is a stored
subtraction result; equality to the exact real complement is conditional.
The sequential binary recurrence and cumsum add further rounding, so the
displayed equations are mathematical limits, not binary tail certificates.
There is no clipping, renormalization or arbitrary finite tail cutoff here.

When the kernel mass is close to one, the endpoint is approximately
`y_attach-D`, rather than a fixed location relative to the unstressed string.
A strong ringing field followed by a weaker stroke can have positive
`y_attach` larger than `D` in the new stroke's signed normal. A positive final
support position is therefore an admitted possibility. This alone does not
prove that a particular trajectory fails to separate: an oscillation can
separate earlier. It does mean that fresh-from-rest retirement cannot be
promoted into a guarantee for all incoming phases and amplitudes.

There is a simple reason why a positive, stationary final support cannot
provide a general zero-storage certificate. For the model's fixed modal
basis, scalar port and positive modal mass/frequencies, define the static
compliance

    C = sum_over_axes_and_modes B[n]^2 / (mass * omega[n]^2) > 0.

At a constant support `z > 0`, the model admits the equilibrium

    f = K*z / (1+K*C),
    q[n] = B[n]*f / (mass*omega[n]^2),  v[n] = 0,
    delta = z / (1+K*C) > 0,
    U_contact = K*delta^2/2 > 0.

These equations follow directly from the modal spring balance and the
existing unilateral contact potential. They show an admissible positive
storage equilibrium, not its reachability from any particular captured
initial state. Modal velocity dissipation and instantaneous actuator power
are zero there. The pickup's derivative can be silent while the mechanical
displacement and contact storage remain nonzero; audible silence does not
authorize resetting that field or contact.

The next continuous-state qualification must retain, rather than skip,
strong-to-soft strokes, both normal signs, several incoming phases and
intervals, and arrival during both press and withdrawal. `D=0` is a useful
controller edge case; it does not inherit a nonzero energy normalization or
stand in for a physically calibrated MIDI stroke. Actual physical retirement
still requires zero contact storage and the owned separating condition.
Unresolved positive storage remains explicit. There is no timeout reset,
automatic field erasure, arbitrary added lift distance, new gain, or claim
that the controller component alone qualifies natural repicks.

If the full ringing-state tests expose trapped or busy contact, the next
physical hypothesis must own a geometric return/lift or continuation law and
account for its work. Changing that law would require fresh mechanics
qualification; it cannot inherit the current source run's proof unchanged.
