"""UNEXECUTED v2 analytic-tail evidence architecture. Compression/chunks do not alter physics."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
import gzip,hashlib,json,shutil
from source_inputs_v1 import require

# Administrative evidence chunk, reusing the established4096-sample witness
# length. It is never a force, release, contact or audio smoothing parameter.
CHUNK=4096
BUDGET_BYTES=12*(1024**3)

class StudyBudget:
    """Shared across all96 cases; purely administrative storage accounting."""
    def __init__(self):self.used=0
    def reserve(self,count,root):
        require(self.used+count<=BUDGET_BYTES,'whole-study evidence budget exhausted')
        require(shutil.disk_usage(root).free>count,'insufficient disk for next complete evidence write')
        self.used+=count

class CaseArchive:
    def __init__(self,root,case,bindings,np,budget):
        self.root=Path(root);self.root.mkdir(exist_ok=False);self.np=np
        self.case=case;self.bindings=bindings;self.buffers=[];self.bytes=0;self.chunk=0;self.budget=budget
        self.cert=(self.root/'all-active-native-certificates.jsonl.gz').open('xb')
        self.gz=gzip.GzipFile(fileobj=self.cert,mode='wb',mtime=0)
        self._write_json('input-provenance.json',bindings)

    def _capacity(self,upper_bytes):
        # A resource failure preserves the completed prefix and marks the case
        # incomplete. It never shortens contact or drops states/certificates.
        self.budget.reserve(upper_bytes,self.root)

    def _write_json(self,name,obj):
        raw=(json.dumps(obj,indent=2,allow_nan=False)+'\n').encode();self._capacity(len(raw))
        with (self.root/name).open('xb')as f:f.write(raw)
        self.bytes+=len(raw)

    def active_sample(self,frame,q,v,record):
        """Keep full active q/v and the raw equation/trace, including failures."""
        self.buffers.append((frame,q.copy(),v.copy()))
        raw=(json.dumps({'frame':frame,**record},separators=(',',':'),allow_nan=False)+'\n').encode()
        self._capacity(len(raw));self.gz.write(raw);self.bytes+=len(raw)
        if len(self.buffers)==CHUNK:self.flush_active()

    def flush_active(self):
        if not self.buffers:return
        np=self.np;frames=np.asarray([r[0]for r in self.buffers]);q=np.asarray([r[1]for r in self.buffers]);v=np.asarray([r[2]for r in self.buffers])
        self._capacity(frames.nbytes+q.nbytes+v.nbytes)
        path=self.root/f'contact-qv-{self.chunk:05d}.npz'
        with path.open('xb')as f:np.savez_compressed(f,frames=frames,q=q,v=v)
        self.bytes+=path.stat().st_size;self.chunk+=1;self.buffers=[]

    def tail_checkpoint(self,frame,q,v,work,D,retirement):
        self.flush_active();np=self.np
        path=self.root/'post-retirement-initial-state.npz'
        self._capacity(q.nbytes+v.nbytes+4096)
        with path.open('xb')as f:np.savez_compressed(f,frame=frame,q=q,v=v,work=work,D=D)
        self.bytes+=path.stat().st_size
        self._write_json('post-retirement-provenance.json',{
            'retirement':retirement,'coefficient_provenance':self.bindings,
            'free_tail_algorithm':'analytic_tail_v2, qualified stable analytic API from own canonical-frame checkpoint; integer elapsed-frame/rate; no recurrence bitidentity claim',
            'checkpoint_inclusive_frame':frame,'full_q_v_reconstructible':True,
            'qv_checkpoint_sha256':hashlib.sha256(q.tobytes()+v.tobytes()).hexdigest(),
            'stream_entire_tail':['frame','physical_tap','string_energy','work','cumulative_D_Gramian_16_32','ledger_numerator','ordered_qv_chunk_hash','chunk_end_qv'],
            'omitted_modal_tail_samples_reconstruct_from_exact_checkpoint_API_env_and_integer_clock':True})

    def finish(self,complete,summary):
        self.flush_active();self.gz.close();self.cert.close()
        self._write_json('completion.json',{'complete':complete,'summary':summary,
            'all_active_contact_qv_and_certificates_retained':bool(complete),
            'no_force_or_tail_rescue':True})

if __name__=='__main__':raise SystemExit('DRAFT only; no evidence/state execution authorized')

class TailStream:
    """Every tail frame keeps scalar work/D/state evidence; q/v reconstructs.

    Each chunk hashes every ordered q/v sample and stores its final q/v. These
    are scientific raw evidence, not fitted summaries or a hidden tail filter.
    """
    def __init__(self,archive):
        self.archive=archive;self.rows=[];self.digest=hashlib.sha256();self.index=0
        self.last_q=None;self.last_v=None
    def append(self,frame,q,v,tap,E,W,D,ledger_numerator):
        self.rows.append([frame,tap,E,W,*map(float,D),ledger_numerator])
        self.digest.update(q.tobytes());self.digest.update(v.tobytes())
        self.last_q=q.copy();self.last_v=v.copy()
        if len(self.rows)==CHUNK:self.flush()
    def flush(self):
        if not self.rows:return
        a=self.archive;np=a.np;rows=np.asarray(self.rows)
        a._capacity(rows.nbytes+self.last_q.nbytes+self.last_v.nbytes+4096)
        path=a.root/f'free-tail-{self.index:05d}.npz'
        with path.open('xb')as f:np.savez_compressed(f,
            diagnostic_rows=rows,last_q=self.last_q,last_v=self.last_v,
            ordered_all_qv_sha256=self.digest.hexdigest())
        a.bytes+=path.stat().st_size;self.index+=1;self.rows=[];self.digest=hashlib.sha256()
