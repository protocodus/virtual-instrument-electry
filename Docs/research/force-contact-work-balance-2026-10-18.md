# Force, contact and string work balance — 2026-10-18

An ideal string can exchange calibrated force with a pick contact while
preserving its existing ringing state and accounting for every change in
energy. This diagnostic establishes that smaller physical model. It does
not qualify a production source, identify a pick trajectory or demonstrate
a realism improvement. The [numeric record](force-contact-work-balance-2026-10-18.json)
contains every case and the frozen evidence hashes.

## Current waves and a physical force port

Use a nondispersive string with tension T, mass per length μ, speed
`c=sqrt(T/μ)` and wave impedance `Z=μc=T/c`. Each directed delay edge spans
`dx=c*h`, where h is the timestep. Its rightgoing and leftgoing velocity
waves are r and l. Physical cell velocity is r+l; slope is `(l−r)/c`.
Consequently the complete string storage is

```
E_string = h*Z*sum(r²+l²).
```

This already includes kinetic and strain energy: no additional factor 1/2
belongs in the wave-energy expression. Displacement has an independent
check: `Y[j+1]−Y[j]=h*(l[j]−r[j])`, with fixed `Y[0]=0`.

At an interior point, read incoming waves a and b before either outgoing
write. For transverse force F applied to the string,

```
q = a+b
v = q+F/(2Z)
out_left = v−a;  out_right = v−b
Delta E_string = h*F*v.
```

The junction velocity is `a+out_left=b+out_right=v`. Summing the two
outgoing storage samples instead gives `2v−q`, a different quantity. These
relations use the standard [loaded waveguide junction](https://www.dsprelated.com/freebooks/pasp/Loaded_Waveguide_Junctions.html)
and [traveling-wave energy](https://www.dsprelated.com/freebooks/pasp/Digital_Waveguide_Theory.html)
bases; the simultaneous edge update and discrete work checks are derived
here. In that update, reconstructed displacement advances by exactly `h*v`.

## Static release and passive ringing contact

The existing quasistatic force anchor is `F0=4.8 N`. With distances L1 and L2
from the contact to the fixed ends, it determines a triangular peak A:

```
K_string = T*(1/L1+1/L2)
A = F0/K_string
E_string = F0*A/2.
```

A complete zero-velocity triangle has opposing nonzero waves
`r=−c*y_x/2`, `l=+c*y_x/2`. Holding force F0 keeps it stationary. Releasing
the constraint leaves those waves intact. The ideal kink then launches a
velocity front; its first interval velocity is `−F0/(2Z)`. The held initial
field has zero velocity, while the released kink needs a one-sided time
convention. An extra zero-velocity sample there would define another problem.
An arbitrary displacement-wave pulse therefore cannot be relabeled F:
velocity-wave forcing uses F/(2Z), and displacement-wave forcing requires
compatible time integration and initial state.

For a spring and parallel damper, let e be string displacement minus anchor
displacement, u the anchor velocity, k≥0 the stiffness and d≥0 the damping.
Trapezoidal spring integration gives

```
e_next = e+h*(v−u)
v = [2Z*q−k*e+(d+k*h/2)*u]/[2Z+d+k*h/2]
F = −k*(e+e_next)/2−d*(v−u)
Delta(E_string+k*e²/2) = h*F*u−h*d*(v−u)².
```

A fixed anchor is passive even on a ringing field. The diagnostic attaches
an unstrained spring at the current local displacement, without resetting
the waves. A moving anchor would supply the explicit external work hFu;
no such trajectory is invented here. Removing a loaded spring removes its
storage unless a separate transfer mechanism is defined. Holding the static
triangle through that spring requires additional storage `F0²/(2k)` and an
anchor at `A+F0/k`; the force anchor alone does not determine k or a strike.

## Checks and their limits

The material probe reads the unchanged fresh-light E2's L, T and Z. The ideal
grid uses `h=L/(M*c)` and omits stiffness and production feedback filters.
Stiffness ratios .1/1/10 and damping 0/2Z are declared oracle stress cases,
not proposed instrument parameters.

| Check | Cases | Result |
|---|---:|---|
| Static 4.8 N triangle hold | 12 | Complete displacement and zero velocity retained |
| Free release against independent d'Alembert fields | 12 | Maximum node error 4.42e−17 m; cell-velocity error 1.59e−13 m/s |
| Static spring preload and storage removal | 3 | Loading and removed energy explicitly accounted for |
| Passive contact on ringing fields | 24 integer + 24 fractional | Maximum ledger error 1.01e−14 relative to initial energy |
| Independent local force/power identities | 6,000 | Maximum normalized residual below 5.6e−15 |

For fixed fractional read weights g, deposit nodal force with the same
weights and gather the solved velocities: `v_p=q_p+||g||²*F/(2Z)`. The spring
solve then uses `2Z/||g||²`, preserving `Delta E=h*F*v_p`. This is an extended
discrete force distribution, not an exact fractional point. Moving weights
also require transport work through `dot(g)^T Y`; production interpolation,
moving frets and changing string length are not qualified by this proof.

In the lossless integer case, matching source and observation coordinates
also preserves complete state and current pickup output exactly during three
ringing-contact episodes. A controlled wrong-port counterexample instead
shows a nominally passive damper adding 75% string energy: its velocity read
does not correspond to its force deposit. That is an ideal algebraic witness,
not a measured gain or failure claim about the complete production engine.

## What is still missing from a playable source

Shipping has no calibrated transient Contact/load force signal.
[makeVelocityProfile](../../Source/DSP/ElectryEngine.cpp#L1553) supplies
dimensionless effort and a timing heuristic. During Contact, voiced
attenuation and stopping-hand restoration act on the ringing state. The
named load/slip smoothsteps occur afterward inside Release, driving modal
projection, Bessel/low-pass and separately voiced edge paths rather than a
force port with a work ledger. The 4.8 N IC/attack-energy anchors constrain
static displacement and energy; they do not identify F(t), damping, hand
motion or detachment. The existing representative 6 kN/m pick stiffness does
not supply those missing inputs either.

A passive fixed contact cannot supply a new strike. A replacement needs an
explicit actuator/contact law and its external work. This result does not
reinstate the previously rejected passive-spring or prescribed-moving-edge
experiments, or justify new source gains, signs or detunes.

The [string-field investigation](string-field-initialization-2026-10-18.md)
also remains binding: a ring-only coordinate rotation does not commute with
a fixed rational feedback bank, and raw delay R cannot silently replace the
declared physical period P. Coupled polarizations, distributed dispersion,
finite contact release, interpolation, refrets, realtime cost and dry-reference
behavior still require qualification. No DSP, defaults, tests, version or
audio artifacts changed in this documentation step.

The export preserves all numeric values, replacing only four absolute
checkout-prefixed strings with repository-relative paths. It binds frozen
receipt `ee83c9dcf0de13aaaa2af6bc2223e9f9f9e147bb6d805307681dfe1890151f38`
and aggregate `7ab368899e6db5faed5637dbeb837906838f4d429a07d189b27f97ee09d21740`.
All 17 bound artifacts and 24 production Source identities were verified;
the latter also match published `6c83ff453ff7ac346598e8e73367ced128a90838`.
