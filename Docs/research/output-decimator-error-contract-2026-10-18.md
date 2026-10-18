# Output-decimator error-contract prototype — 2026-10-18

This research prototype separates output-stage contamination from modeled
high-frequency signal. It changes the measured contract: the old guard caps
all sampled output above 12 kHz; the prototype measures error relative to an
independent finite-window baseband channel. Keeping the same numeric rails
does not preserve the old guard's meaning. No production test, threshold,
default or source pin changes.

The [original failure and diagnosis](ultrasonic-floor-diagnosis-2026-10-18.md)
remain unchanged. The separate `95e5c1b` upper-loss ablation is
[rejected on its absolute-tone tradeoff](upper-loss-ablation-2026-10-18.md).
Passing this error prototype does not approve that candidate.

## Method

The prospective protocol was frozen before these measurements and the tone
result. The primary case retains the original eight-open-note chord, seed,
controls, host 48 kHz/block 512, .2-second window start and 32768-sample
symmetric Hann. Its 854-frequency grid starts at 40 Hz, multiplies by `1.0075`
and stops below 23500 Hz. Normalization remains `4/(N-1)`; the relative
denominator remains the maximum native full-output spectrum on that entire
grid. The strict rule remains error **more than 150 dB below that peak OR
below −180 dBFS**. The original >12 kHz subset and added full-grid coverage
are reported separately.

The reference convolves the actual float32 pre-decimator input with the exact
float32 production FIR taps using double sums. Integer and half-integer
host-time Hann sums split its finite-window spectrum into baseband `B` and
folding image `I`. With native spectrum `Y` and double-filtered host spectrum
`F`, the aggregate is computed before magnitude:

`E = Y - B = I + (Y - F)`.

Thus folding and native FIR arithmetic stay combined; passing them separately
cannot replace the aggregate. No sinus fitting, source subtraction or gain
adjustment occurs. These finite-window channels can include window leakage;
they are not a unique physical decomposition of an evolving instrument.

## Results and controls

All nine applicable frozen engine traces pass both aggregate subsets. The
eleven original traces include two native 192 kHz cases with no output
decimator, explicitly marked not applicable. The 96 kHz host diagnostics retain
the same physical window duration. The [compact numeric record](output-decimator-error-contract-2026-10-18.json)
preserves every case, component maximum, original guard result and input hash.

| Case | Native >12 kHz maximum | Aggregate >12 kHz | Aggregate full grid |
| --- | ---: | ---: | ---: |
| Original flag off, 48 kHz | −186.413 dBFS | −207.678 dBFS | −206.860 dBFS |
| Ablation flag on, 48 kHz | −168.872 dBFS | −207.341 dBFS | −206.748 dBFS |
| Clean 1.002 kHz baseband tone | −197.559 dBFS | −197.936 dBFS | −194.534 dBFS |
| Clean 18.052 kHz baseband tone | −19.999 dBFS | −193.978 dBFS | −193.978 dBFS |
| 31.983 kHz internal tone, stock FIR | −100.205 dBFS | −100.205 dBFS | −100.205 dBFS |
| Same internal tone, FIR bypass | −20.000 dBFS | −20.000 dBFS | −20.000 dBFS |

The original flag-on total-signal cap still fails. The legitimate 18 kHz
baseband witness passes the error rule despite failing that old cap. The known
above-host-Nyquist tone folds to 16.017 kHz and fails the unchanged error rails,
including through the correct stock FIR. Mixed low/high and delayed-bypass
controls also fail. All synthetic tones use prospectively fixed amplitude `.1`;
the bypass retains 31 internal samples of nominal latency.

These controls expose a practical limit: the stock 63-tap FIR supplies roughly
80 dB rejection at that high-tone frequency, consistent with its stated roughly
75 dB stopband design. The 150 dB/−180 dBFS rails can constrain the fixed
engine fixture and input envelope; they are not an input-independent theorem
for arbitrary out-of-band inputs. No replacement filter tolerance is fitted.

## Capture and verification

An ignored witness calls the actual public `process(...,1)` and reads its two
newest decimator history entries through the existing friend accessor. At
48/96 kHz, one-sample versus 512-sample blocks produce byte-identical PCM;
history-captured internal samples also match the original frozen traces.
This covers 144000 host and 288000 internal samples without copying the
feedback, FIFO or render scheduler into a test.

Capture requires active linked 2:1 state, finite inputs, post-call not-idle and
exactly two history writes per host sample. An initial pre-call idle check was
overstrict immediately after reset/note-on; both rejected attempts remain
bound. A frozen or disabled rendered sample cannot silently reuse history.

An independent full half-step sum agrees with the producing polyphase formula
within `1.44e-13` complex amplitude over 95 probes. Native DFT magnitudes agree
with the original C++ recurrence within `1.31e-14`. All 120 earlier artifacts
remain hash-identical. The 62-file prototype manifest SHA256 is
`2cfc5dac955af124769294211769a3a3b4f81eee47d73c899c83d690b867d6fc`.
The numeric record binds the frozen tools, inputs and complex per-frequency
exports without duplicating their arrays. The 24 production source files
remain identical to `6c83ff453ff7ac346598e8e73367ced128a90838`.

This isolates only the output stage. Internal baseband can still contain
upstream alias, numerical content or modeled noise. Existing pickup and
upstream/nonlinear alias guards remain necessary and untouched; neither
physical source correctness nor tone, note-transition or realtime readiness
follows from passing this prototype.
