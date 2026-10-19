"""DRAFT serial evidence archive; no numerical/physical calls on import.

The shared budget and emergency reserve are administrative only. A resource
failure stops the experiment incomplete; it never shortens a physical scene.
All prior archive versions are preserved. Caller must use `abort(error, ...);
raise` on failure, including attachment and analytic-tail failures.
"""
if not __debug__:
    raise SystemExit('Optimized Python forbidden')
from pathlib import Path
import gzip
import hashlib
import os
import shutil
from failure_io_v1 import (MARKER_BYTES, exception_facts, json_bytes, json_safe,
                           marker_payload, rewrite_marker_noexcept)

CHUNK = 4096
BUDGET_BYTES = 12 * 1024**3
EMERGENCY_BYTES = 256 * 1024**2
MIN_NORMAL_FREE_BYTES = 64 * 1024**2


class StudyBudget:
    """One serial-study budget, initialized before any case state construction.

    A real (not sparse) reserve owns failure/stream-closing disk capacity. Write
    accounting is conservative: overwritten ZIP headers are charged again.
    After any abort this budget cannot admit another normal case/write.
    """
    def __init__(self, root):
        self.root = Path(root)
        if not self.root.is_dir():
            raise RuntimeError('Study output directory must exist before resource preflight')
        free = shutil.disk_usage(self.root).free
        if free <= EMERGENCY_BYTES + MIN_NORMAL_FREE_BYTES:
            raise RuntimeError('Insufficient actual disk for emergency reserve and normal evidence')
        self.reserve_path = self.root / 'administrative-emergency-reserve.bin'
        self.used = EMERGENCY_BYTES
        self.emergency_used = 0
        self.terminal = False
        self.emergency_active = False
        # Exclusive creation prevents mixing/retrying an existing study.
        with self.reserve_path.open('xb') as file:
            block = b'\0' * (1024**2)
            for _ in range(EMERGENCY_BYTES // len(block)):
                file.write(block)
            file.flush()
            os.fsync(file.fileno())

    def reserve(self, count, root, emergency=False):
        if not isinstance(count, int) or count < 0:
            raise RuntimeError('Invalid administrative evidence byte count')
        if emergency:
            if not self.emergency_active or self.emergency_used + count > EMERGENCY_BYTES:
                raise RuntimeError('Emergency evidence allocation exhausted')
            self.emergency_used += count
        else:
            if self.terminal or self.used + count > BUDGET_BYTES:
                raise RuntimeError('Whole-study evidence budget exhausted or terminal')
            self.used += count
        if shutil.disk_usage(root).free <= count:
            raise RuntimeError('Insufficient actual disk for complete evidence write')

    def activate_emergency(self):
        self.terminal = True
        if not self.emergency_active:
            # Only our preallocated administrative reserve may be removed.
            self.reserve_path.unlink()
            self.emergency_active = True

    def finish_success(self):
        """Call only after the entire study has closed successfully."""
        if self.terminal:
            raise RuntimeError('Incomplete study cannot release reserve as successful')
        self.reserve_path.unlink()
        self.terminal = True


class _BudgetedWriter:
    def __init__(self, file, archive):
        self.file, self.archive = file, archive
    def write(self, data):
        a = self.archive
        a.budget.reserve(len(data), a.root, emergency=a._aborting)
        written = self.file.write(data)
        if written != len(data):
            raise OSError('Short evidence write')
        return written
    def tell(self): return self.file.tell()
    # NumPy1.26.4 zipfile_factory recognizes file objects by this method.
    def read(self, *args): return self.file.read(*args)
    def seek(self, *args): return self.file.seek(*args)
    def seekable(self): return True
    def writable(self): return True
    def flush(self): return self.file.flush()
    def fileno(self): return self.file.fileno()
    @property
    def name(self): return self.file.name


class CaseArchive:
    def __init__(self, root, case, bindings, np, budget):
        self.root = Path(root)
        self.root.mkdir(exist_ok=False)
        self.case, self.bindings, self.np, self.budget = case, bindings, np, budget
        self.case_id = case.get('id', case.get('case_id', '<unknown>'))
        self.buffers, self.active_chunks = [], []
        self.bytes, self.chunk = 0, 0
        self._aborting, self._finished, self._aborted = False, False, False
        self._pending_records, self._current_record = [], None
        self._abort_result = None
        self.cert = self.gz = self.marker = None
        try:
            self.marker = (self.root / 'emergency-status.json').open('xb+')
            raw = marker_payload(self.case_id)
            self.budget.reserve(MARKER_BYTES, self.root)
            self.marker.write(raw)
            self.marker.flush()
            os.fsync(self.marker.fileno())
            self.cert = (self.root / 'all-active-native-certificates.jsonl.gz').open('xb')
            self.gz = gzip.GzipFile(fileobj=_BudgetedWriter(self.cert, self), mode='wb', mtime=0)
            self.save_record('input-provenance', bindings)
        except BaseException as error:
            self.abort(error, attempted={'stage': 'archive_initialization'})
            raise

    def _path(self, name, suffix):
        if not name or Path(name).name != name or name in ('.', '..'):
            raise RuntimeError('Evidence name must be a single explicit basename')
        return self.root / (name if name.endswith(suffix) else name + suffix)

    def save_arrays(self, name, **arrays):
        """Save caller-owned raw arrays only; no reconstruction or dtype repair."""
        path = self._path(name, '.npz')
        for value in arrays.values():
            if hasattr(value, 'dtype') and value.dtype.hasobject:
                raise RuntimeError('Object arrays forbidden in raw evidence')
        with path.open('xb') as file:
            self.np.savez_compressed(_BudgetedWriter(file, self), **arrays)
        self.bytes += path.stat().st_size
        return path

    def save_record(self, name, obj):
        path = self._path(name, '.json')
        arrays = {}
        raw = json_bytes(obj, arrays)
        if arrays:
            array_path = self.save_arrays(path.stem + '-raw-arrays', **arrays)
            raw = json_bytes({'record': json_safe(obj, {}),
                              'raw_array_file': array_path.name})
        with path.open('xb') as file:
            _BudgetedWriter(file, self).write(raw)
        self.bytes += path.stat().st_size
        return path

    def active_sample(self, frame, q, v, record):
        """Preserve exact supplied index/q/v plus its complete raw equation/cert.

        Grid samples keep integer frame coordinates. Use tail_checkpoint or
        save_arrays/save_record for an exact off-grid event/accepted ODE states.
        No certificate is fabricated for no-call steps.
        """
        if frame is None:
            raise RuntimeError('Off-grid event requires exact event checkpoint')
        self._current_record = record
        self.buffers.append((frame, q.copy(), v.copy()))
        raw = json_bytes({'frame': frame, **record}, compact=True)
        self._pending_records.append(raw)
        self.gz.write(raw)
        if len(self.buffers) >= CHUNK:
            self.flush_active()

    def flush_active(self):
        if not self.buffers:
            return None
        # Until a successful compressed-stream flush, raw lines stay available
        # for emergency persistence if a quota/disk failure interrupts gzip.
        self.gz.flush()
        self._pending_records = []
        np = self.np
        # Active grid readers retain original integer frames,q,v. Exact event
        # checkpoints use tail_checkpoint/save_arrays instead of a frame snap.
        if any(row[0] is None for row in self.buffers):
            raise RuntimeError('Off-grid events require exact event checkpoint, not active grid chunk')
        arrays = {'frames': np.asarray([r[0] for r in self.buffers]),
                  'q': np.asarray([r[1] for r in self.buffers]),
                  'v': np.asarray([r[2] for r in self.buffers])}
        path = self.save_arrays(f'contact-qv-{self.chunk:05d}', **arrays)
        self.active_chunks.append(path.name)
        self.chunk += 1
        self.buffers = []
        return path

    def tail_checkpoint(self, frame=None, q=None, v=None, work=None, D=None,
                        retirement=None, *, event_time_s=None):
        if frame is None and event_time_s is None:
            raise RuntimeError('Tail checkpoint needs actual grid frame or exact event time')
        self.flush_active()
        arrays = {'q': q, 'v': v, 'work': work, 'D': D}
        if frame is not None: arrays['frame'] = frame
        if event_time_s is not None: arrays['event_time_s'] = event_time_s
        path = self.save_arrays('post-retirement-initial-state', **arrays)
        self.save_record('post-retirement-provenance', {
            'retirement': retirement, 'coefficient_provenance': self.bindings,
            'checkpoint_inclusive_frame': frame, 'event_time_s': event_time_s,
            'no_event_time_frame_snap': True, 'full_q_v_reconstructible': True,
            'qv_checkpoint_sha256': hashlib.sha256(q.tobytes() + v.tobytes()).hexdigest(),
            'qv_checkpoint_file': path.name,
            'tail_method_and_clock_owned_by_bound_caller': True})
        return path

    def save_analytic_batch(self, name, arrays, provenance):
        """Generic D2/D3 batch; caller owns diagnostics, endq/v, hash and method."""
        path = self.save_arrays(name, **arrays)
        self.save_record(name + '-provenance', {'arrays_file': path.name, **provenance})
        return path

    def _close_streams(self):
        errors = []
        try:
            if self.gz is not None: self.gz.close()
        except BaseException as error:
            errors.append('gzip close: ' + repr(error))
        finally:
            try:
                if self.cert is not None: self.cert.close()
            except BaseException as error:
                errors.append('raw certificate close: ' + repr(error))
        return errors

    def finish(self, complete, summary):
        if not complete:
            raise RuntimeError('Use abort(original_error, attempted=...) on failure; never finish(False)')
        self.flush_active()
        errors = self._close_streams()
        if errors:
            raise OSError('; '.join(errors))
        self.save_record('completion', {'complete': True, 'summary': summary,
            'all_active_contact_qv_and_certificates_retained': True,
            'emergency_status_role': 'dormant administrative failure marker, not the case completion or scientific gate',
            'active_chunks': self.active_chunks, 'no_force_or_tail_rescue': True})
        self._finished = True
        if self.marker is not None: self.marker.close()

    def abort(self, error, attempted=None):
        """No-throw emergency persistence. Caller must re-raise ORIGINAL error.

        Pass current attempted q/v/equation/cert/chunk and accepted/root facts.
        Filesystem failure can prevent some evidence: the preallocated marker
        and every completion flag remain explicitly incomplete in that case.
        """
        if self._aborted:
            return self._abort_result
        self._aborted = True
        errors = []
        self._aborting = True
        original = exception_facts(error)
        try:
            self.budget.activate_emergency()
        except BaseException as failure:
            errors.append('release reserve: ' + repr(failure))
        try:
            if self.buffers:
                # A failed normal chunk may already exist. Use an exclusive
                # emergency name and retain buffers without clearing/repair.
                np = self.np
                self.save_arrays('emergency-buffered-active-qv',
                    frames=np.asarray([r[0] for r in self.buffers]),
                    q=np.asarray([r[1] for r in self.buffers]),
                    v=np.asarray([r[2] for r in self.buffers]))
        except BaseException as failure:
            errors.append('buffered active prefix: ' + repr(failure))
        try:
            if self._pending_records:
                path = self.root / 'emergency-pending-equation-certificates.jsonl'
                with path.open('xb') as file:
                    writer = _BudgetedWriter(file, self)
                    for raw in self._pending_records:
                        writer.write(raw)
                self.bytes += path.stat().st_size
        except BaseException as failure:
            errors.append('pending equation/certificate lines: ' + repr(failure))
        try:
            self.save_record('emergency-original-error-and-attempt', {
                'complete': False, 'all_histories_complete': False,
                'original_error': original, 'attempted': attempted,
                'current_active_sample_record': self._current_record,
                'source_evidence': getattr(error, 'source_evidence', None),
                'active_chunks_completed_before_failure': self.active_chunks,
                'no_retry_truncation_or_physical_rescue': True})
        except BaseException as failure:
            errors.append('original error/attempt: ' + repr(failure))
        finally:
            errors.extend(self._close_streams())
            if self.marker is not None:
                rewrite_marker_noexcept(self.marker, self.case_id, original, errors)
                try: self.marker.close()
                except BaseException: pass
        self._abort_result = {'complete': False, 'original_error_preserved': True,
                              'emergency_evidence_errors': errors}
        return self._abort_result


if __name__ == '__main__':
    raise SystemExit('DRAFT only; no storage/physical/force/reference execution authorized')
