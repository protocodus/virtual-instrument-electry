"""UNEXECUTED control-only checker. Entry is the separately reviewed wrapper.

No package import, path construction/evaluation or payload access at import.
All binary64 evidence is lossless: old arrays plus uint64 XOR arrays, never a
numerical difference substituted for the recorded words. No allclose/tolerance.
"""
if not __debug__:
    raise SystemExit('Optimized Python forbidden')
import ast
import copy
from dataclasses import replace
from fractions import Fraction
import hashlib
import math
from pathlib import Path
import struct

RATE = 96000
ORIGINS = (0, 2**53, 2**63)
FIXTURES = (
    ('primary', '0x0p+0', '0x0p+0', '0x1p-10'),
    ('nonzero_approach', '0x1p-11', '0x1p-6', '0x1p-10'),
    ('opposite_phase', '-0x1p-11', '-0x1p-6', '0x1p-10'),
    ('zero_stroke_edge', '0x1p-11', '0x1p-6', '0x0p+0'),
)


def require(value, message):
    if not value:
        raise RuntimeError(message)


def raw(x):
    return struct.pack('>d', float(x)).hex()


def number(x):
    x = float(x)
    return x if math.isfinite(x) else {'nonfinite_binary64': raw(x)}


def safe(value):
    if isinstance(value, float):
        return number(value)
    if isinstance(value, dict):
        return {str(k): safe(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [safe(v) for v in value]
    return value


def original_class(path):
    """Execute only the exact bound SupportPath AST, not its source imports.

    This is called only after the wrapper's exact authority and imports gate.
    The original class body is neither copied nor rewritten.
    """
    tree = ast.parse(Path(path).read_text())
    selected = [x for x in tree.body if isinstance(x, ast.ClassDef) and x.name == 'SupportPath']
    require(len(selected) == 1, 'Exactly one original SupportPath')
    ns = {'math': math, 'PHYSICAL_RATE': RATE, 'require': require}
    exec(compile(ast.Module(body=selected, type_ignores=[]), str(path), 'exec'), ns)
    return ns['SupportPath']


def new_instance(api, case, pulse, fixture, origin):
    name, yh, vh, dh = fixture
    y, v, D = map(float.fromhex, (yh, vh, dh))
    p = case['release_frame96'] - case['contact_frame96']
    recipe = api.ContactRecipe(case['id'] + '/' + name, 'controller-only-fixed-geometry',
        p, tuple(float(x) for x in pulse),
        float.fromhex(case['release_coefficient_binary64_hex']), D,
        'Original C pulse/pole/clocks; manufactured descriptor, no force calibration')
    arrival = api.Arrival(recipe.recipe_id, origin, None, 0)
    snap = api.PortSnapshot(origin, recipe.geometry_key, y, v, 'opaque-qv', 'opaque-ledger')
    return api.ContactInstance(recipe, arrival, snap)


def tuple_value(instance, tick, side):
    x = instance.value(tick, side)
    return (math.nan if x.z_m is None else x.z_m,
            math.nan if x.velocity_m_per_s is None else x.velocity_m_per_s,
            math.nan if x.acceleration_m_per_s2 is None else x.acceleration_m_per_s2)


def translated_probe(instance, local_tick, side):
    """Explicit control-cursor clones, NOT a reset/reattach of a physical port.

    The clone has exactly the current three-knot/state history. Only arrival
    origin and the matching opaque attachment boundary are translated. This
    tests the actual local-clock adapter at fixed sentinels. It does not claim
    three independently replayed full dynamic prefixes at the large origins.
    """
    out = []
    for origin in ORIGINS:
        clone = copy.copy(instance)
        clone.withdrawal = copy.copy(instance.withdrawal)
        clone.withdrawal.cache = copy.copy(instance.withdrawal.cache)
        clone.arrival = replace(instance.arrival, contact_boundary_tick96=origin)
        clone.attachment = replace(instance.attachment, boundary_tick96=origin)
        out.append(tuple_value(clone, origin + local_tick, side))
    return out


def original_case(row, pulse, fixture, zero_origin=False):
    _, yh, vh, dh = fixture
    c = row['contact_frame96']
    return {'contact_frame96': 0 if zero_origin else c,
            'release_frame96': row['release_frame96'] - (c if zero_origin else 0),
            'calls96': row['calls96'] - (c if zero_origin else 0),
            'release_coefficient': float.fromhex(row['release_coefficient_binary64_hex']),
            'pulse': pulse, 'D_m': float.fromhex(dh)}, float.fromhex(yh), float.fromhex(vh)


def comparison(np, old, new):
    old, new = np.asarray(old, dtype=np.float64), np.asarray(new, dtype=np.float64)
    require(old.shape == new.shape, 'Comparison shape')
    xo = np.bitwise_xor(old.view(np.uint64), new.view(np.uint64))
    finite = np.isfinite(old) & np.isfinite(new)
    diff = new[finite] - old[finite]
    return {'shape': list(old.shape), 'raw_word_equal_count': int(np.count_nonzero(xo == 0)),
        'word_count': int(old.size), 'raw_bit_identity': bool(np.all(xo == 0)),
        'finite_pattern_equal': bool(np.array_equal(np.isfinite(old), np.isfinite(new))),
        'old_nonfinite_count': int(np.count_nonzero(~np.isfinite(old))),
        'new_nonfinite_count': int(np.count_nonzero(~np.isfinite(new))),
        'max_absolute_finite_difference': number(np.max(np.abs(diff))) if diff.size else None,
        'min_signed_finite_difference': number(np.min(diff)) if diff.size else None,
        'max_signed_finite_difference': number(np.max(diff)) if diff.size else None}


def xor(np, baseline, actual):
    return np.bitwise_xor(np.asarray(baseline, dtype=np.float64).view(np.uint64),
                          np.asarray(actual, dtype=np.float64).view(np.uint64))


def probe_records(np, api, Ref, row, pulse, fixture, out):
    """Complete manufactured press/side probes, fixed withdrawal sentinels/tail.

    These are descriptor observations, not mechanics/lifetime/retirement tests.
    Original terminal-right remains explicitly undefined; tail has no old ref.
    """
    p = row['release_frame96']-row['contact_frame96']
    n, k = row['calls96']-row['release_frame96'], len(pulse)
    queries = {(Fraction(j, 2), 'right', 'press_native_and_half') for j in range(2*p + 1)}
    queries |= {(Fraction(0), 'left', 'attachment_left'), (Fraction(p), 'left', 'Release_left')}
    for j in (0, k-1, n-1):
        for side in ('left', 'right'):
            queries.add((Fraction(p+j), side, 'withdrawal_sentinel'))
    for j in (n, n-1+k, 2*n-1):
        queries.add((Fraction(p+j), 'right', 'postcrop_NEW_SCOPE'))
    ordered = sorted(queries, key=lambda q: (q[0], q[1], q[2]))
    # Declare every query before evaluation: a later exception cannot silently
    # remove the unevaluated remainder or previously completed observations.
    out.extend({'fixture': fixture[0], 'local_tick96_num_den': [t.numerator, t.denominator],
                'side': side, 'label': label, 'completed': False,
                'unavailable': 'Not evaluated before a retained fixture failure'}
               for t, side, label in ordered)
    rc, y, v = original_case(row, pulse, fixture)
    zc, _, _ = original_case(row, pulse, fixture, True)
    ref, zero = Ref(rc, y, v, np, _SIGNAL), Ref(zc, y, v, np, _SIGNAL)
    new = new_instance(api, row, pulse, fixture, 0)
    for record, (tick, side, label) in zip(out, ordered):
        old_available = tick <= p+n-1
        terminal_right = tick == p+n-1 and side == 'right'
        if old_available:
            if tick >= p and tick.denominator == 1:
                j = int(tick-p)
                ta, tz = float(ref.knots[j]), float(zero.knots[j])
            else:
                ta, tz = (row['contact_frame96'] + float(tick))/RATE, float(tick)/RATE
            a, z = ref.value(ta, side), zero.value(tz, side)
        else:
            ta = tz = None
            a = z = None
        b = tuple_value(new, tick, side)
        # After the base query its cached j-1/j/j+1 supports the same sentinel.
        translated = translated_probe(new, tick, side)
        record.update({'completed': True, 'unavailable': None,
            'old_absolute_time_binary64': raw(ta) if ta is not None else None,
            'old_zero_time_binary64': raw(tz) if tz is not None else None,
            'old_absolute_values_words': [raw(x) for x in a] if a is not None else None,
            'old_zero_values_words': [raw(x) for x in z] if z is not None else None,
            'new_local_values_words': [raw(x) for x in b],
            'large_origin_cursor_clone_values_words': [[raw(x) for x in q] for q in translated],
            'old_status': 'UNDEFINED_terminal_right_velocity' if terminal_right else
                          ('available' if old_available else 'UNAVAILABLE_postcrop'),
            'absolute_vs_local': comparison(np, a, b) if a is not None and not terminal_right else None,
            'zero_vs_local': comparison(np, z, b) if z is not None and not terminal_right else None,
            'large_origin_clone_bit_identity': all([raw(x) for x in q] == [raw(x) for x in b] for q in translated),
            'no_physical_retirement_or_ledger_claim': True})


def lifecycle_records(api, row, pulse, out):
    """Manufactured branch descriptors only; no q/v/storage calculation."""
    origin = 100
    fixture = FIXTURES[0]
    initial = new_instance(api, row, pulse, fixture, origin)
    service = api.SameGeometryContactService(initial.recipe.geometry_key)
    service.current = initial
    for label, endpoint, U in (('positive_U', origin+1, 1.0),
                               ('zero_U_unretired', origin+2, 0.0)):
        snap = api.PortSnapshot(endpoint, initial.recipe.geometry_key, 0.0, 0.0, 'opaque-qv', 'opaque-ledger')
        initial.observe_endpoint(snap, U)
        before = (id(service.current), id(initial.withdrawal), initial.withdrawal.last_generated,
                  initial.contact_U_J, initial.last_qv_identity, initial.last_ledger_identity)
        result = service.request_attach(initial.recipe, api.Arrival(label, endpoint, None, endpoint), snap)
        after = (id(service.current), id(initial.withdrawal), initial.withdrawal.last_generated,
                 initial.contact_U_J, initial.last_qv_identity, initial.last_ledger_identity)
        out.append({'branch': label, 'result': result, 'old_owner_controller_unchanged': before == after})
    endpoint = origin+3
    snap = api.PortSnapshot(endpoint, initial.recipe.geometry_key, 0.0, 0.0, 'opaque-qv', 'opaque-ledger')
    result = service.request_attach(initial.recipe, api.Arrival('stale', endpoint, None, endpoint), snap)
    out.append({'branch': 'stale_endpoint', 'result': result})
    for label, recipe, arrival, snapshot in (
        ('duplicate_arrival', initial.recipe, api.Arrival('duplicate', endpoint, None, endpoint), snap),
        ('geometry_mismatch', replace(initial.recipe, geometry_key='unsupported-other-geometry'),
         api.Arrival('geometry', endpoint+1, None, endpoint+1),
         api.PortSnapshot(endpoint+1, 'unsupported-other-geometry', 0., 0., 'opaque-qv', 'opaque-ledger'))):
        try:
            result = service.request_attach(recipe, arrival, snapshot)
            out.append({'branch': label, 'unexpected_return': result, 'expected_explicit_rejection': False})
        except RuntimeError as e:
            out.append({'branch': label, 'expected_explicit_rejection': True, 'exception': repr(e)})


_SIGNAL = None  # injected by authorized entry; no import/package call here


def run_case(np, signal, api, Ref, row, pulse, arrays, facts):
    """Caller owns a per-case failure sink and writes all initialized evidence.

    Array allocation is initialized to NaN, not uninitialized memory. Completed
    prefix counts explicitly delimit partial evidence on exceptions.
    """
    global _SIGNAL
    _SIGNAL = signal
    p = row['release_frame96'] - row['contact_frame96']
    n = row['calls96'] - row['release_frame96']
    require(p > 0 and n >= len(pulse) >= 1, 'Owned Contact/crop/pulse lengths')
    rc, y, v = original_case(row, pulse, FIXTURES[0])
    zc, _, _ = original_case(row, pulse, FIXTURES[0], True)
    ref, zero = Ref(rc, y, v, np, signal), Ref(zc, y, v, np, signal)
    new = new_instance(api, row, pulse, FIXTURES[0], 0)
    arrays.update(old_rate=ref.rate, old_R=ref.R, old_knots_absolute=ref.knots,
                  old_knots_zero=zero.knots)
    for key, shape in (('new_rate', (n,)), ('new_R', (n,)),
                       ('old_native', (n, 2, 3)), ('zero_native', (n, 2, 3)),
                       ('new_native', (n, 2, 3)), ('old_midpoint_times_absolute', (n-1,)),
                       ('old_midpoint_times_zero', (n-1,)), ('old_midpoint', (n-1, 3)),
                       ('zero_midpoint', (n-1, 3)), ('new_midpoint', (n-1, 3))):
        arrays[key] = np.full(shape, np.nan, dtype=np.float64)
    facts.update(full_native_knots_completed=0, full_midpoint_intervals_completed=0,
                 old_terminal_right_status='z available; velocity undefined; no old right interval',
                 fixtures=[], lifecycle=[])
    for j in range(n):
        tick = Fraction(p+j)
        for side_index, side in enumerate(('left', 'right')):
            arrays['old_native'][j, side_index] = ref.value(float(ref.knots[j]), side)
            arrays['zero_native'][j, side_index] = zero.value(float(zero.knots[j]), side)
            arrays['new_native'][j, side_index] = tuple_value(new, tick, side)
        knot = new.withdrawal.knot(j)
        arrays['new_rate'][j], arrays['new_R'][j] = knot.filtered_mass, knot.cumulative_mass
        facts['full_native_knots_completed'] = j+1
        if j < n-1:
            ta = (float(ref.knots[j])+float(ref.knots[j+1]))/2
            tz = (float(zero.knots[j])+float(zero.knots[j+1]))/2
            arrays['old_midpoint_times_absolute'][j] = ta
            arrays['old_midpoint_times_zero'][j] = tz
            arrays['old_midpoint'][j] = ref.value(ta, 'right')
            arrays['zero_midpoint'][j] = zero.value(tz, 'right')
            arrays['new_midpoint'][j] = tuple_value(new, tick+Fraction(1, 2), 'right')
            facts['full_midpoint_intervals_completed'] = j+1
    # The old last-right NaN is expected, retained, and excluded from an
    # equivalence claim about an interval the old finite crop never defined.
    native_domain = np.ones(arrays['old_native'].shape, dtype=bool)
    native_domain[-1, 1, 1] = False
    facts['comparisons'] = {
        'rate': comparison(np, ref.rate, arrays['new_rate']),
        'R': comparison(np, ref.R, arrays['new_R']),
        'owned_absolute_native_defined_components': comparison(np, arrays['old_native'][native_domain], arrays['new_native'][native_domain]),
        'zero_origin_native_defined_components': comparison(np, arrays['zero_native'][native_domain], arrays['new_native'][native_domain]),
        'owned_absolute_midpoints': comparison(np, arrays['old_midpoint'], arrays['new_midpoint']),
        'zero_origin_midpoints': comparison(np, arrays['zero_midpoint'], arrays['new_midpoint']),
    }
    facts['defined_prefix_finite'] = bool(
        np.isfinite(arrays['old_native'][native_domain]).all()
        and np.isfinite(arrays['zero_native'][native_domain]).all()
        and np.isfinite(arrays['new_native']).all()
        and all(np.isfinite(arrays[k]).all() for k in
                ('old_rate', 'old_R', 'new_rate', 'new_R',
                 'old_midpoint', 'zero_midpoint', 'new_midpoint')))
    for fixture in FIXTURES:
        fixture_record = {'fixture': fixture[0], 'completed': False}
        fixture_record['records'] = []
        facts['fixtures'].append(fixture_record)
        try:
            probe_records(np, api, Ref, row, pulse, fixture, fixture_record['records'])
            fixture_record['completed'] = True
        except Exception as e:
            fixture_record['exception'] = repr(e)
            if isinstance(e, MemoryError):
                raise
    lifecycle_records(api, row, pulse, facts['lifecycle'])


def encoded_arrays(np, arrays):
    """Declared NPZ representation: baseline + uint64 XOR, complete shapes.

    Each actual array = view_float64(view_uint64(baseline) XOR xor). Bytes for
    NaN/signedzero/failed partial prefixes are retained without semantic repair.
    Single copies of rate/R are shared across all descriptor/clock probes.
    """
    out = dict(arrays)
    for key, baseline in (('new_rate', 'old_rate'), ('new_R', 'old_R'),
                          ('new_native', 'old_native'), ('zero_native', 'old_native'),
                          ('new_midpoint', 'old_midpoint'), ('zero_midpoint', 'old_midpoint')):
        if key in arrays and baseline in arrays:
            out[key+'_xor_'+baseline] = xor(np, arrays[baseline], arrays[key])
            del out[key]
    return out


if __name__ == '__main__':
    raise SystemExit('DRAFT core only; no current execution authority')
