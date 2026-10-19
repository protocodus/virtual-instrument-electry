"""DRAFT evidence serialization only; no scientific modules or computations.

Nonfinite values remain explicit raw-bit facts. Failure handling is best effort
under filesystem/hardware failure and must never replace the original exception.
"""
if not __debug__:
    raise SystemExit('Optimized Python forbidden')
import base64
import hashlib
import json
import math
import struct
import traceback
from pathlib import Path

MARKER_BYTES = 4096


def json_safe(value, arrays=None):
    """Return JSON-safe facts; optionally extract full ndarray bytes to NPZ.

    `arrays` is an already owned output dictionary, never an array constructor.
    NumPy is not imported here. Finite binary floats round-trip as JSON numbers;
    nonfinite scalar payloads retain their original bytes/dtype.
    """
    if value is None or isinstance(value, (str, bool, int)):
        return value
    if hasattr(value, 'dtype') and hasattr(value, 'tobytes'):
        if getattr(value, 'ndim', 0) != 0:
            if arrays is not None:
                key = f'raw_array_{len(arrays):05d}'
                arrays[key] = value
                return {'raw_array_npz_member': key, 'dtype': str(value.dtype),
                        'shape': list(value.shape)}
            return {'dtype': str(value.dtype), 'shape': list(value.shape),
                    'values': json_safe(value.tolist())}
        item = value.item()
        if isinstance(item, (float, complex)) and (
                not math.isfinite(item.real) or not math.isfinite(item.imag)):
            return {'nonfinite_scalar': repr(item), 'dtype': str(value.dtype),
                    'native_bytes_hex': value.tobytes().hex()}
        return json_safe(item, arrays)
    if isinstance(value, float):
        if math.isfinite(value):
            return value
        return {'nonfinite_binary64': repr(value),
                'ieee754_big_endian_hex': struct.pack('>d', value).hex()}
    if isinstance(value, complex):
        return {'complex': [json_safe(value.real), json_safe(value.imag)]}
    if isinstance(value, dict):
        if not all(isinstance(key, str) for key in value):
            return {'mapping_items': [[json_safe(k, arrays), json_safe(v, arrays)]
                                      for k, v in value.items()]}
        return {key: json_safe(item, arrays) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [json_safe(item, arrays) for item in value]
    if isinstance(value, Path):
        return str(value)
    if isinstance(value, bytes):
        return {'raw_bytes_base64': base64.b64encode(value).decode('ascii')}
    raise TypeError(f'Unsupported evidence value: {type(value).__name__}')


def json_bytes(value, arrays=None, compact=False):
    return (json.dumps(json_safe(value, arrays), allow_nan=False,
                       separators=(',', ':') if compact else None,
                       indent=None if compact else 2) + '\n').encode('utf-8')


def exception_facts(error):
    try:
        description = repr(error)
    except BaseException:
        description = '<exception repr failed>'
    try:
        trace = ''.join(traceback.format_exception(type(error), error, error.__traceback__))
    except BaseException:
        trace = '<traceback formatting failed>'
    return {'type': f'{type(error).__module__}.{type(error).__qualname__}',
            'repr': description, 'traceback': trace,
            'original_exception_not_replaced': True}


def marker_payload(case_id, original_error=None, evidence_errors=()):
    """Bounded fixed-block marker; detailed evidence is stored separately."""
    full = json_bytes(original_error or {})
    record = {'case_id': str(case_id)[:256],
              'role': 'dormant administrative failure marker; not case completion or scientific admission',
              'complete': False,
              'failure_recorded': original_error is not None,
              'all_histories_complete': False,
              'original_error_sha256': hashlib.sha256(full).hexdigest(),
              'error_utf8_prefix_base64': base64.b64encode(full[:768]).decode('ascii'),
              'error_prefix_truncated': len(full) > 768,
              'emergency_evidence_error_count': len(evidence_errors),
              'emergency_evidence_errors_prefix': [str(e)[:192] for e in evidence_errors[:4]],
              'no_physical_truncation_or_rescue': True}
    raw = json_bytes(record, compact=True)
    if len(raw) > MARKER_BYTES:
        raise RuntimeError('Bounded administrative marker exceeded allocation')
    return raw + b' ' * (MARKER_BYTES - len(raw))


def rewrite_marker_noexcept(file, case_id, original_error, errors):
    """Overwrite already allocated blocks, never masking the caller's error."""
    try:
        file.seek(0)
        file.write(marker_payload(case_id, original_error, errors))
        file.flush()
        return True
    except BaseException:
        return False


if __name__ == '__main__':
    raise SystemExit('DRAFT only; no evidence or numerical execution authorized')
