"""UNEXECUTED once-only stored-equation replay; no compile or physical scheduler.

Source preparation may inspect this file but must not import or run it. A future
exact manifest plus root/peer execution authority is required by main(). The
reviewed oracle/cases are imported only after all old logical byte proofs close.
"""
if not __debug__:
    raise SystemExit('Optimized Python is forbidden')
import sys
sys.dont_write_bytecode = True
import argparse, contextlib, copy, datetime, gzip, hashlib, importlib.util, json
import os, platform, shutil, signal, stat, struct, subprocess, tarfile, traceback, zipfile
from pathlib import Path

P=Path(__file__).resolve().parent;R=P.parent
PYTHON=Path('/Users/vojta/Dev/electry-real-dry-20261018/build-real-dry-review/python312/bin/python')
BASELINE='6c83ff453ff7ac346598e8e73367ced128a90838'
M=P/'execution-implementation-manifest-v1.json'
PROTOCOL=P/'wrapper-protocol-draft-v1.json'
ROUTES=P/'old-regression-routes-draft-v1.json'
COMPILE_ROOT=R/'build-multicontact-passive-kernel'
COMPILE_M=COMPILE_ROOT/'implementation-manifest-v2.json'
COMPILE_P=COMPILE_ROOT/'compile-protocol-draft-v2.json'
COMPILE_RECEIPT=COMPILE_ROOT/'compile-v2/execution-receipt-v2.json'
OUT=P/'results-v1'
CHUNK=1024*1024;OUTPUT_CAP=2*1024**3;ADMIN_RESERVE=8*1024**2;OWN_FLOOR=2*1024**3

def need(ok,message):
    if not ok:raise RuntimeError(message)
def utc():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def load(p):return json.loads(Path(p).read_text())
def rel(p):return str(Path(p).relative_to(R))
def canonical(name):
    p=Path(name);need(not p.is_absolute() and '..' not in p.parts,'canonical repository-relative path')
    need(str(p)==name,'canonical path spelling')
    return R/p
def metadata(p):
    s=os.fstat(p.fileno())
    return dict(bytes=s.st_size,mtime_ns=s.st_mtime_ns,device=s.st_dev,inode=s.st_ino,
                mode=stat.S_IMODE(s.st_mode),nlink=s.st_nlink)
def open_regular(path):
    path=Path(path);need(path.is_absolute(),'absolute held input')
    for p in reversed(path.parents):need(not p.is_symlink(),'input ancestor symlink '+str(p))
    fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW)
    try:
        need(stat.S_ISREG(os.fstat(fd).st_mode),'regular input '+str(path))
        return os.fdopen(fd,'rb')
    except BaseException:os.close(fd);raise
def digest_stream(f,guard=None):
    h=hashlib.sha256();size=lines=0
    while True:
        if guard:guard.check()
        b=f.read(CHUNK)
        if not b:break
        h.update(b);size+=len(b);lines+=b.count(b'\n')
    return dict(bytes=size,sha256=h.hexdigest()),lines
def fact(path):
    with Path(path).open('rb') as f:return digest_stream(f)[0]
def bound(path,expected):need(fact(path)==expected,'changed binding '+str(path))
def write_json(path,value):
    raw=(json.dumps(value,indent=2,allow_nan=False)+'\n').encode()
    with Path(path).open('xb') as f:need(f.write(raw)==len(raw),'short metadata write');f.flush();os.fsync(f.fileno())
def failure(error):return dict(type=type(error).__name__,message=str(error),traceback=traceback.format_exc())
def import_path(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec)
    sys.modules[name]=module;spec.loader.exec_module(module);return module

def static(m):
    need(Path(sys.executable).absolute()==PYTHON and sys.version_info[:3]==(3,12,8),'fixed Python312 entry')
    need(platform.system()=='Darwin' and platform.machine()=='arm64','fixed strict host')
    need(m['schema']=='electry-capacity5-replay-execution-implementation/v1','manifest schema')
    need(sys.path==m['sys_path'],'direct entry import paths')
    rt=m['runtime'];need(str(PYTHON.resolve())==rt['python_real_executable'],'interpreter route')
    need(platform.release()==rt['platform_release'],'platform release')
    need({k:os.environ.get(k) for k in rt['inherited_environment']}==rt['inherited_environment'],'inherited environment')
    for section,base in (('artifacts',P),('inputs',R),('environment_files',Path('/'))):
        for name,b in m[section].items():bound(base/name,b)
    need(m['protocol_sha256']==fact(PROTOCOL)['sha256'] and m['production_commit']==BASELINE,'selected protocol/Source baseline')
    for name,b in m['Source24'].items():
        bound(R/name,b)
        published=subprocess.run(['git','show',BASELINE+':'+name],cwd=R,capture_output=True)
        need(published.returncode==0 and len(published.stdout)==b['bytes'] and
             hashlib.sha256(published.stdout).hexdigest()==b['sha256'],'published Source '+name)
    return m

def reviews(auth,manifest_sha,protocol_sha,flag):
    for role in ('root','peer'):
        item=auth['reviews'][role];p=canonical(item['path']);need(fact(p)['sha256']==item['sha256'],'review hash '+role)
        x=load(p);need(x['role']==role and x['static_clear'] is True and x[flag] is True,'review scope '+role)
        need(x['implementation_manifest_sha256']==manifest_sha and x['protocol_sha256']==protocol_sha,'review target '+role)

def authorize(path,expected):
    need(fact(path)['sha256']==expected,'root execution authority hash')
    a=load(path);need(a['schema']=='electry-capacity5-stored-equation-replay-authorization/v1','execution authority schema')
    need(a['capacity5_stored_equation_replay_authorized'] is True and
         a['physical_state_work_audio_or_E_access_authorized'] is False,'execution scope')
    mh=fact(M)['sha256'];ph=fact(PROTOCOL)['sha256']
    need(a['implementation_manifest_sha256']==mh and a['protocol_sha256']==ph,'execution exact targets')
    reviews(a,mh,ph,'capacity5_stored_equation_replay_authorized')
    need(a['live_source_capacity_admitted'] is True and type(a['source_reserved_free_bytes']) is int and
         a['source_reserved_free_bytes']>=0,'fresh root live-source capacity admission')
    need(a['compile_receipt']['path']==rel(COMPILE_RECEIPT) and
         a['compile_receipt']['sha256']==fact(COMPILE_RECEIPT)['sha256'],'actual successful compile receipt authority')
    return a

def compiled(a):
    """No future binary fact is guessed: exact closed receipt and root auth own it."""
    c=load(COMPILE_RECEIPT);cm=load(COMPILE_M);cp=load(COMPILE_P)
    need(c['schema']=='electry-multicontact-compile-only-execution/v2' and c['compile_link_PASS'] is True and
         c['post_input_bindings']['PASS'] is True and c['compiled_binary_or_environment_control_executed'] is False,'closed compile-only qualification')
    need(c['manifest']['path']==str(COMPILE_M) and {k:c['manifest'][k] for k in ('bytes','sha256')}==fact(COMPILE_M),'compile manifest receipt target')
    need(c['protocol']==fact(COMPILE_P) and cm['protocol_sha256']==fact(COMPILE_P)['sha256'],'compile protocol receipt target')
    need(len(c['attempts'])==3 and all(x['PASS'] is True and x['owned_group_closed'] is True and x['returncode']==0 for x in c['attempts']),'all strict compiler children closed successfully')
    need([x['argv'] for x in c['attempts']]==cp['allowed_compiler_processes'],'exact strict compile argv')
    ca=c['authorization'];capath=Path(ca['path']);capath=capath if capath.is_absolute() else canonical(ca['path'])
    bound(capath,{k:ca[k] for k in ('bytes','sha256')});old=load(capath)
    need(old['schema']=='electry-multicontact-compile-only-authorization/v2' and old['multicontact_compile_only_authorized'] is True and
         old['compiled_binary_execution_authorized'] is False,'compile authority scope')
    need(old['implementation_manifest_sha256']==fact(COMPILE_M)['sha256'] and
         old['protocol_sha256']==fact(COMPILE_P)['sha256'],'compile authority exact targets')
    reviews(old,fact(COMPILE_M)['sha256'],fact(COMPILE_P)['sha256'],'multicontact_compile_only_authorized')
    for section,base in (('artifacts',COMPILE_ROOT),('inputs',R),('Source24',R),('Tests',R)):
        for name,b in cm[section].items():bound(base/name,b)
    bound(R/'CMakeLists.txt',cm['CMakeLists'])
    binaries=[]
    for label,name in (('binary','compile-v2/multicontact-stream-v1'),('environment_binary','compile-v2/multicontact-environment-negatives-v1')):
        p=COMPILE_ROOT/name;b=c['outputs'][name];bound(p,b)
        need(a[label]=={'path':rel(p),**b},'exact postcompile runtime binary '+label);binaries.append(p)
    return binaries

class Budget:
    """Administrative bounded own writes; no physics adaptation or hard OS reservation."""
    def __init__(self,authority):
        self.floor=max(OWN_FLOOR,authority['source_reserved_free_bytes']);self.reserve=OUT/'administrative-reserve.bin'
        self.check()
        with self.reserve.open('xb') as f:
            for _ in range(ADMIN_RESERVE//CHUNK):need(f.write(b'\0'*CHUNK)==CHUNK,'short admin reserve write')
            f.flush();os.fsync(f.fileno())
    def used(self):return sum(p.stat().st_size for p in OUT.iterdir() if p.is_file())
    def check(self,extra=0):
        need(shutil.disk_usage(P).free>=self.floor,'administrative free floor, live-source reservation retained')
        # used() includes the physically written reserve until data work ends.
        need(self.used()+extra<=OUTPUT_CAP,'administrative total cap; no cohort truncation rescue')
    def release(self):
        with self.reserve.open('wb') as f:f.flush();os.fsync(f.fileno())
    def writer(self,path):return BudgetWriter(path,self)

class BudgetWriter:
    def __init__(self,path,budget):self.f=Path(path).open('xb');self.budget=budget
    def write(self,data):
        self.budget.check(len(data));n=self.f.write(data);need(n==len(data),'short compressed evidence write');return n
    def flush(self):self.f.flush()
    def tell(self):return self.f.tell()
    def close(self):self.f.close()

class Evidence:
    def __init__(self,path,budget):
        self.raw=budget.writer(path);self.gz=gzip.GzipFile(filename='',mode='wb',compresslevel=6,fileobj=self.raw,mtime=0)
    def write(self,row):
        b=(json.dumps(row,separators=(',',':'),allow_nan=False)+'\n').encode();need(self.gz.write(b)==len(b),'short logical evidence write')
    def flush(self):self.gz.flush();self.raw.flush()
    def close(self):
        try:self.gz.close()
        finally:self.raw.close()

def held_payloads(stack,m,routes,budget):
    """Byte proof pass, not semantic decoding; all complete before first force call."""
    held={};saved={};proofs=[]
    for name,b in m['payload_container_bindings'].items():
        p=Path(name);f=stack.enter_context(open_regular(p));before=metadata(f);got,_=digest_stream(f,budget)
        need(got==b and metadata(f)==before,'held whole archive changed '+name);f.seek(0);held[name]=f;saved[name]=before
        proofs.append(dict(kind='whole-container',path=name,**got))
    for item in routes['files']:
        x=item['npz']
        if x['route']=='direct-preserved':
            name=x['original_path'];f=stack.enter_context(open_regular(Path(name)))
            need(metadata(f)==x['stat'],'direct registered metadata '+name)
            got,_=digest_stream(f,budget);need(got==x['owned_byte_fact'] and metadata(f)==x['stat'],'direct owned NPZ byte proof '+name)
            f.seek(0);held[name]=f;saved[name]=x['stat'];proofs.append(dict(kind='direct-NPZ',path=name,**got))
    certs={x['certificate']['member']:x for x in routes['files']}
    tarname=routes['files'][0]['certificate']['archive'];seen=[]
    for item,stream in selected_tar(held[tarname],certs):
        got,lines=digest_stream(stream,budget);need(got==item['certificate']['owned_byte_fact'] and lines==item['expected_records'],'all old certificate bytes/count '+item['id'])
        seen.append(item['id']);proofs.append(dict(kind='tar-certificate',id=item['id'],records=lines,**got))
    need(len(seen)==66 and len(set(seen))==66,'all66 old logical certificate byte proofs')
    for item in routes['files']:
        x=item['npz']
        if x['route']=='object-zip':
            f=held[x['archive']];f.seek(0)
            with zipfile.ZipFile(f) as z:
                need(sum(i.filename==x['member'] for i in z.infolist())==1,'unique owned NPZ object')
                with z.open(x['member']) as s:got,_=digest_stream(s,budget)
                need(got==x['owned_byte_fact'],'owned NPZ object full SHA/size/CRC '+item['id'])
            proofs.append(dict(kind='object-NPZ',id=item['id'],**got))
    for name,f in held.items():need(metadata(f)==saved[name],'held preflight identity '+name)
    return held,saved,proofs,seen

def selected_tar(file,selected):
    """Linear archive-order generator. Never extract or decode unselected payloads."""
    file.seek(0);seen=set()
    with gzip.GzipFile(fileobj=file,mode='rb') as gz:
        with tarfile.open(fileobj=gz,mode='r|') as tar:
            for member in tar:
                if member.name in selected:
                    need(member.name not in seen and member.isfile(),'unique regular selected tar member')
                    seen.add(member.name);item=selected[member.name]
                    need(member.size==item['certificate']['owned_byte_fact']['bytes'],'logical member size')
                    with tar.extractfile(member) as stream:yield item,stream
        # Drive gzip through its true EOF/trailer; tar may stop at zero blocks.
        for _ in iter(lambda:gz.read(CHUNK),b''):pass
    need(seen==set(selected),'selected tar cohort complete')

def numeric_modules(m):
    import numpy as np
    import scipy
    rt=m['runtime']
    for label,module in (('numpy',np),('scipy',scipy)):
        need(module.__version__==rt[label+'_version'] and str(Path(module.__file__).resolve())==rt[label+'_file'],'actual numeric package '+label)
    # Importing definitions is allowed here; no make_basis, state, scipy root or
    # original solve is ever called. Only this original callback is used later.
    original=import_path('capacity5_bound_original_dg',R/'build-live-field-certified-contraction/mechanics_v2.py')
    need(original.K==6000.,'original numerical fixture K')
    import oracle_draft_v1 as oracle
    import cases_draft_v1 as cases
    return np,original.dg_force_and_derivative,oracle,cases

def raw_request(req):
    return (str(req['id'])+' '+str(req['j'])+' '+' '.join([w for row in req['A'] for w in row]+req['d0']+req['dfree'])+'\n').encode('ascii')

class Driver:
    def __init__(self,binary,raw,checks,budget,np,dg,oracle):
        self.binary=binary;self.raw=raw;self.checks=checks;self.budget=budget;self.np=np;self.dg=dg;self.oracle=oracle
        self.child=None;self.stderr=None;self.sequence=0;self.failed=0;self.groups={};self.cleanup=[];self.closed=False;self.pending=None
    def start(self):
        self.stderr=(OUT/'driver-stderr.txt').open('xb')
        self.child=subprocess.Popen([str(self.binary)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=self.stderr,start_new_session=True)
    def consume(self,req,identity):
        need(req['id']==self.sequence,'fixed global request sequence')
        self.budget.check();line=raw_request(req);response=b'';row=dict(sequence=self.sequence,identity=identity,checks_PASS=False)
        self.pending=dict(request=req,identity=identity,request_ascii=line.decode('ascii'),raw_response_hex='',check_row=row)
        transport_owned=False
        try:
            # Retain attempted request before any write/read/parse failure.
            self.raw.write(dict(stage='request',sequence=self.sequence,identity=identity,request_ascii=line.decode('ascii')))
            need(self.child.stdin.write(line)==len(line),'short driver stdin');self.child.stdin.flush()
            response=self.child.stdout.readline()
            self.pending['raw_response_hex']=response.hex()
            self.raw.write(dict(stage='response',sequence=self.sequence,response_hex=response.hex()))
            need(response.endswith(b'\n'),'closed native response line')
            value=json.loads(response);self.oracle.schema_check(req,value);transport_owned=True
            row['native_status']=value['status'];row['native_status_name']=value['status_name']
            row['native_partial_claims']={k:value.get(k) for k in ('exact_stored_residual_certified','posterior_error_bound_valid','cpp_legacy_diagnostic_pass')}
            diagnostic=None
            if req.get('expected','accept')=='accept' and value.get('status') in (0,1):
                # Schema/input ownership first, before NumPy decodes any return.
                diagnostic=self.oracle.actual_numpy_residual(self.np,self.dg,req,value);row['actual_original_numpy']=diagnostic
            row['exact_checks']=self.oracle.check_saved_record(req,value,diagnostic)
            row['checks_PASS']=row['exact_checks']['expected_behavior_PASS'] is True
        except Exception as error:
            row['failure']=failure(error)
            # A trusted immutable driver transport failure terminates the run;
            # no malformed record is retried or replaced with another equation.
            if not transport_owned or isinstance(error,MemoryError):
                try:self.checks.write(row)
                except BaseException as write_error:row['secondary_evidence_failure']=failure(write_error)
                self.failed+=1;raise
            value=None
        self.checks.write(row);self.pending=None;group=identity['group'];g=self.groups.setdefault(group,dict(records=0,failed=0))
        g['records']+=1;g['failed']+=not row['checks_PASS'];self.failed+=not row['checks_PASS'];self.sequence+=1
        if self.sequence%2048==0:
            self.raw.flush();self.checks.flush();print('REPLAY',self.sequence,'records',self.failed,'failed',flush=True)
        return row,value,diagnostic
    def close(self,aborted):
        if self.child is None:
            if self.stderr:self.stderr.close()
            return
        try:
            if aborted and self.child.poll() is None:
                try:os.killpg(self.child.pid,signal.SIGINT)
                except ProcessLookupError:pass
                except BaseException as e:self.cleanup.append(failure(e))
            try:self.child.stdin.close()
            except BaseException as e:self.cleanup.append(failure(e))
            try:
                tail=self.child.stdout.read()
                if tail:self.raw.write(dict(stage='unconsumed-stdout',response_hex=tail.hex()))
            except BaseException as e:self.cleanup.append(failure(e));tail=None
            code=self.child.wait();self.closed=True
            need(not aborted and code==0 and tail==b'','native closed cleanly with no unconsumed responses')
        finally:
            if self.stderr:self.stderr.close()

def old_arrays(np,item,held):
    """Only three original equation members. No q/v/work or old-force member."""
    x=item['npz']
    with contextlib.ExitStack() as stack:
        if x['route']=='direct-preserved':f=held[x['original_path']];f.seek(0)
        else:
            need(x['route']=='object-zip','registered NPZ route only')
            archive=held[x['archive']];archive.seek(0);z=stack.enter_context(zipfile.ZipFile(archive));f=stack.enter_context(z.open(x['member']))
        with np.load(f,allow_pickle=False) as z:
            d0=z['solve_d0_m'];df=z['solve_dfree_m'];A=z['solve_Asolve_m_per_N']
        need(d0.dtype==df.dtype==A.dtype==np.dtype('float64'),'original stored binary64 equations')
        need(d0.ndim==2 and df.shape==d0.shape and A.shape==(len(d0),d0.shape[1],d0.shape[1]),'original equation array shape')
        return A,d0,df

def old_request(np,oracle,sequence,item,certificate,arrays,previous):
    A,d0,df=arrays;frame=certificate['frame'];idx=certificate['active_ports']
    need(type(frame) is int and previous<frame<len(d0),'old certificate frame ownership')
    need(type(idx) is list and 1<=len(idx)<=2 and len(set(idx))==len(idx) and
         all(type(i) is int and 0<=i<d0.shape[1] for i in idx),'old active-port order ownership')
    a=A[frame][np.ix_(idx,idx)];x=d0[frame,idx];y=df[frame,idx]
    need(np.isfinite(a).all() and np.isfinite(x).all() and np.isfinite(y).all(),'active original equation finiteness')
    word=lambda v:struct.pack('>d',float(v)).hex();j=len(idx)
    req=dict(id=sequence,j=j,A=[['0000000000000000']*5 for _ in range(5)],d0=['0000000000000000']*5,dfree=['0000000000000000']*5,expected='accept')
    for i in range(j):
        req['d0'][i]=word(x[i]);req['dfree'][i]=word(y[i])
        for k in range(j):req['A'][i][k]=word(a[i,k])
    return req,dict(case_id=item['id'],group=item['group'],frame=frame,active_ports=idx),frame

def saved_negatives(oracle,req,native,diagnostic,evidence):
    """Ten predeclared saved-evidence corruptions; never an additional force call."""
    names=('wrong-id','truncated-trace','wrong-update','raw-lower-word','reversed-interval',
           'false-zero-certificate','false-cap-count','false-cap-endpoint','inactive-force','altered-equation')
    rows=[]
    for name in names:
        q=copy.deepcopy(req);r=copy.deepcopy(native);row=dict(name=name,rejection_verified=False)
        try:
            if name=='wrong-id':r['id']+=1
            elif name=='truncated-trace':r['trace'].pop()
            elif name=='wrong-update':r['trace'][0]['updates']+=1
            elif name=='raw-lower-word':r['trace'][1]['force'][0]=f"{int(r['trace'][1]['force'][0],16)^1:016x}"
            elif name=='reversed-interval':r['trace'][-1]['mapped'][0].reverse()
            elif name=='false-zero-certificate':r['trace'][-1]['certificate_residual']='0000000000000000'
            elif name=='false-cap-count':r['budgets']['cap_updates']+=1
            elif name=='false-cap-endpoint':r['budgets']['geometric_upper_at_cap']=f"{int(r['budgets']['geometric_upper_at_cap'],16)^1:016x}"
            elif name=='inactive-force':r['force'][3]='0000000000000001'
            elif name=='altered-equation':q['A'][0][0]=f"{int(q['A'][0][0],16)^1:016x}"
            try:oracle.check_saved_record(q,r,diagnostic)
            except (ValueError,RuntimeError,KeyError,IndexError) as e:row.update(rejection_verified=True,reason=str(e))
            row['modified_request']=q;row['modified_response']=r
        except Exception as e:row['unavailable']=failure(e)
        evidence.write(dict(stage='saved-record-negative',**row));rows.append({k:v for k,v in row.items() if k not in ('modified_request','modified_response')})
    return rows

def environment_controls(binary,raw,budget):
    rows=[]
    for mode in ('FE_DOWNWARD','FTZ_BIT24'):
        budget.check();row=dict(mode=mode,PASS=False);child=None
        try:
            # These child guards call no force map on their correct branch.
            raw.write(dict(stage='environment-request',mode=mode,argv=[str(binary),mode]))
            child=subprocess.Popen([str(binary),mode],stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
            stdout,stderr=child.communicate();row.update(returncode=child.returncode,stdout_hex=stdout.hex(),stderr_hex=stderr.hex())
            raw.write(dict(stage='environment-response',**row))
            x=json.loads(stdout);need(child.returncode==0 and x['schema']=='electry-multicontact-environment-negative/v1' and x['mode']==mode and x['PASS'] is True,'environment child status')
            need(x['status']==3 and x['map_evaluations']==x['primitive_operations']==x['trace_rows']==0,'zero-map environment guard')
            need(x['before_fpcr']==x['after_fpcr']!=x['saved_fpcr'] and x['entry_fpcr']==x['before_fpcr'] and
                 x['restored_fpcr']==x['saved_fpcr']==0 and x['before_round']==x['after_round'] and x['saved_round']==x['restored_round'],'caller environment unchanged and restored by child')
            need(all(x[k] is False for k in ('exact_stored_residual_certified','posterior_error_bound_valid','cpp_legacy_diagnostic_pass','analytic_zero_solution','cap_bound_certified')),'environment rejection clears all claims')
            row['PASS']=True
        except Exception as e:row['failure']=failure(e)
        finally:
            if child is not None and child.poll() is None:
                try:os.killpg(child.pid,signal.SIGINT)
                finally:child.wait()
        rows.append(row)
    return rows

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--authorization',required=True);parser.add_argument('--authorization-sha256',required=True);args=parser.parse_args()
    authpath=canonical(args.authorization);auth=authorize(authpath,args.authorization_sha256);m=static(load(M))
    binaries=compiled(auth);routes=load(ROUTES)
    entry=dict(implementation_manifest=fact(M),protocol=fact(PROTOCOL),authorization={'path':args.authorization,**fact(authpath)},compile_receipt=fact(COMPILE_RECEIPT))
    need(not OUT.exists(),'once-only results destination collision');OUT.mkdir()
    budget=None;raw=checks=driver=None;errors=[];cases=[];env=[];corruptions=[];permutations=[];proofs=[];complete=False;post=False;bytepost=False
    started=utc();sequence_at_close=0;groups={};failed=0;input_preflight=False;case_ids=[]
    try:
        budget=Budget(auth);write_json(OUT/'attempt-v1.json',dict(schema='electry-capacity5-stored-replay-attempt/v1',utc=started,pid=os.getpid(),ppid=os.getppid(),argv=sys.argv,authorization={'path':args.authorization,**fact(authpath)},implementation_manifest=fact(M),protocol=fact(PROTOCOL),physical_or_E_access=False))
        raw=Evidence(OUT/'raw-native-and-negative-evidence-v1.jsonl.gz',budget);checks=Evidence(OUT/'checks-v1.jsonl.gz',budget)
        need(len(routes['files'])==66 and routes['expected_nonempty_records']==152579,'fixed original scope')
        with contextlib.ExitStack() as stack:
            held,held_stats,proofs,ordered_old=held_payloads(stack,m,routes,budget);input_preflight=True
            write_json(OUT/'logical-input-byte-preflight-v1.json',dict(all_input_bytes_proven_before_first_force=True,proofs=proofs,tar_ordered_case_ids=ordered_old))
            np,dg,oracle,recipes=numeric_modules(m)
            new=recipes.registered_cases();need(len(new)==230,'all fixed new requests');case_ids=[x['case_id'] for x in new]
            driver=Driver(binaries[0],raw,checks,budget,np,dg,oracle);base={}
            try:
                driver.start()
                for req in new:
                    row,native,diagnostic=driver.consume(req,dict(case_id=req['case_id'],group='registered-new'))
                    if req['case_id'].endswith('/coupled-dense-spd') and row['checks_PASS']:base[req['j']]=(req,native,diagnostic)
                    if 'permutation' in req:
                        extra=dict(stage='permutation',case_id=req['case_id'],PASS=False)
                        try:
                            need(row['checks_PASS'] and req['j'] in base,'both permutation endpoints globally qualified')
                            extra.update(facts=oracle.permutation_posterior_check(*base[req['j']][:2],req,native),PASS=True)
                        except Exception as e:extra['failure']=failure(e)
                        checks.write(extra);permutations.append(extra)
                if 3 in base:corruptions=saved_negatives(oracle,*base[3],raw)
                else:corruptions=[dict(name='all-ten',rejection_verified=False,unavailable='registered J3 dense-SPD base not qualified')]
                del base,new
                selected={x['certificate']['member']:x for x in routes['files']};tarname=routes['files'][0]['certificate']['archive']
                for item,stream in selected_tar(held[tarname],selected):
                    c=dict(id=item['id'],group=item['group'],expected_records=item['expected_records'],records=0,failed=0,complete=False);cases.append(c)
                    arrays=old_arrays(np,item,held) if item['expected_records'] else None;previous=-1;h=hashlib.sha256();size=lines=0
                    for line in stream:
                        budget.check();h.update(line);size+=len(line);lines+=line.count(b'\n');certificate=json.loads(line)
                        req,identity,previous=old_request(np,oracle,driver.sequence,item,certificate,arrays,previous)
                        row,_,_=driver.consume(req,identity);c['records']+=1;c['failed']+=not row['checks_PASS']
                    need(dict(bytes=size,sha256=h.hexdigest())==item['certificate']['owned_byte_fact'] and
                         c['records']==lines==item['expected_records'],'old complete logical replay identity/count')
                    c['complete']=True;raw.flush();checks.flush();print('CASE',c['id'],c['records'],'records',c['failed'],'failed',flush=True)
                    del arrays
                need(driver.sequence==230+152579 and len(cases)==66 and all(x['complete'] for x in cases),'complete original/new cohort')
                need(driver.groups['main']['records']==106679 and driver.groups['controls_permutation']['records']==45900,'all original groups')
                driver.close(False)
                env=environment_controls(binaries[1],raw,budget)
                for name,f in held.items():
                    need(metadata(f)==held_stats[name],'post held input metadata');f.seek(0);got,_=digest_stream(f,budget)
                    expected=m['payload_container_bindings'].get(name)
                    if expected is None:expected=next(x['npz']['owned_byte_fact'] for x in routes['files'] if x['npz']['original_path']==name)
                    need(got==expected,'post logical/container byte identity '+name)
                bytepost=True;complete=True
            finally:
                if driver is not None and not driver.closed:
                    try:driver.close(True)
                    except BaseException as e:errors.append(dict(stage='owned-driver-close',**failure(e)))
    except BaseException as e:errors.append(dict(stage='replay',**failure(e)))
    finally:
        if budget is not None:
            try:budget.release()
            except BaseException as e:errors.append(dict(stage='release-own-admin-reserve',**failure(e)))
        if driver is not None and driver.pending is not None:
            # Full current request/response is best-effort reserved evidence,
            # separate from any partial gzip; never hide the original exception.
            try:
                emergency=driver.pending;raw_emergency=(json.dumps(emergency,allow_nan=False)+'\n').encode()
                need(len(raw_emergency)<=ADMIN_RESERVE-CHUNK,'current record exceeds reserved administrative evidence capacity')
                with (OUT/'emergency-current-record-v1.json').open('xb') as f:
                    need(f.write(raw_emergency)==len(raw_emergency),'short emergency record write');f.flush();os.fsync(f.fileno())
            except BaseException as e:errors.append(dict(stage='emergency-current-record',full_current_record_retention_unavailable=True,**failure(e)))
        for label,evidence in (('raw',raw),('checks',checks)):
            if evidence is not None:
                try:evidence.close()
                except BaseException as e:errors.append(dict(stage='close-'+label,partial_gzip_readability_not_claimed=True,**failure(e)))
        try:static(m);post=True
        except BaseException as e:errors.append(dict(stage='post-static-bindings',**failure(e)))
    if driver is not None:sequence_at_close=driver.sequence;groups=driver.groups;failed=driver.failed
    all_checks=(complete and post and bytepost and not errors and failed==0 and len(env)==2 and all(x['PASS'] for x in env) and
                len(permutations)==150 and all(x['PASS'] for x in permutations) and len(corruptions)==10 and all(x['rejection_verified'] for x in corruptions))
    summary=dict(schema='electry-capacity5-stored-equation-replay-closure/v1',start_utc=started,closed_utc=utc(),run_closed=driver is None or driver.closed,
        complete=complete and not errors,all_stored_equation_checks_PASS=all_checks,logical_input_byte_preflight_PASS=input_preflight,post_static_bindings_PASS=post,post_payload_bindings_PASS=bytepost,
        **entry,attempted_complete_records=sequence_at_close,failed_records=failed,groups=groups,old_cases=cases,expected_old_cases=[x['id'] for x in routes['files']],
        expected_new_request_count=230,expected_old_nonempty_count=152579,native_cleanup_errors=[] if driver is None else driver.cleanup,
        completed_new_case_ids=case_ids[:min(sequence_at_close,230)],expected_new_case_ids=case_ids,environment_controls=env,permutation_comparisons=permutations,saved_evidence_negative_controls=corruptions,failures=errors,
        physical_state_work_audio_RT_calibration_or_E_qualified=False,old_force_or_numpy_bit_identity_claim=False,
        evidence_policy='Full lossless raw request and response bytes before parse; all failed/partial rows retained. No retry, pruning, physical rescue or skipped input fallback. Own closure cannot guarantee readable interrupted gzip.')
    try:
        write_json(OUT/'summary-v1.json',summary)
        print(json.dumps({'complete':summary['complete'],'all_stored_equation_checks_PASS':all_checks,'summary':fact(OUT/'summary-v1.json')}),flush=True)
    except BaseException as e:
        print(json.dumps({'complete':False,'closure_failure':failure(e),'records':sequence_at_close,'previous_failures':errors},allow_nan=False),flush=True);return 1
    return 0 if all_checks else 1

if __name__=='__main__':raise SystemExit(main())
