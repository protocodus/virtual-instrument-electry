"""UNEXECUTED administrative parent for the unchanged eight-file analysis.

No scientific imports, PCM/field decoding, render, model or analysis call occurs
on import. Future exact manifests, reviews and execution authority are absent.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
import sys
sys.dont_write_bytecode=True
import argparse,datetime,hashlib,json,os,platform,shutil,signal,stat,subprocess,time,traceback
from pathlib import Path

P=Path(__file__).resolve().parent;R=P.parent
PYTHON=Path('/Users/vojta/Dev/electry-real-dry-20261018/build-real-dry-review/python312/bin/python')
BASELINE='6c83ff453ff7ac346598e8e73367ced128a90838'
M=P/'outer-implementation-manifest-v1.json';PROTOCOL=P/'outer-execution-protocol-draft-v3.json'
AM=P/'implementation-manifest-v1.json';AP=P/'protocol-draft-v1.json';OUT=P/'outer-run-v1'
OUTPUT_CAP=256*1024**2;OWN_FREE_FLOOR=1024**3;POLL_SECONDS=.25
RESULT_NAMES=('neck-analysis-v1.json','bridge-analysis-v1.json','incomplete-analysis-v1.json')
EIGHT={
 'analyze_bridge_v1.py':'c1f6c3dc46b48c45092860179d68cf56c550247a8fc388ac89ead4114282778d',
 'analyze_neck_v1.py':'f1df1bfdbcce08864020ac5f575d2dfd34501b57ab0b9aa809721324310a1487',
 'background_v1.py':'053efd5a752c33c3c6633c18c96b510f4e79a66f13452e9fca54d1beff1df103',
 'context_v1.py':'48d9f7a5d7b7d4860d4023b4f96b7517dfbe4939486b23eefea118caa2c7ab87',
 'gate_v1.py':'903e340ba07c530b8dfe13a2f815c1bb975edf3dbd938ef886d2b2ff86bad968',
 'helpers_v1.py':'95e9bf526476bcb303e22bf94db7c006b45a4efc419edd45bc7786025b1312b4',
 'run_after_review_v1.py':'dd747720208331fdf9dbce3585e790461ff069117b5aee6271d5504f4661fbc4',
 'summary_v1.py':'da8506aa8a687c89dc9890dba069dc84fadcdb1be4c1558c450564ff268604f7'}

def need(ok,message):
    if not ok:raise RuntimeError(message)
def utc():return datetime.datetime.now(datetime.timezone.utc).isoformat()
def load(path):return json.loads(Path(path).read_text())
def fact(path):
    p=Path(path)
    with p.open('rb') as f:return {'bytes':os.fstat(f.fileno()).st_size,'sha256':hashlib.file_digest(f,'sha256').hexdigest()}
def canonical(name):
    p=Path(name);need(not p.is_absolute() and '..' not in p.parts and str(p)==name,'canonical repository-relative path')
    return R/p
def bound(path,expected):need(fact(path)==expected,'changed binding '+str(path))
def record_error(stage,error):return {'stage':stage,'type':type(error).__name__,'message':str(error),'traceback':traceback.format_exc()}
def write_json(path,value):
    raw=(json.dumps(value,indent=2,allow_nan=False)+'\n').encode()
    with Path(path).open('xb') as f:need(f.write(raw)==len(raw),'short closure metadata write');f.flush();os.fsync(f.fileno())
def table(entries,base):
    for name,expected in entries.items():
        path=base/name if base==Path('/') else canonical(str((base/name).relative_to(R)))
        bound(path,expected)
def exact_review(authority,role,manifest_sha,protocol_sha,flag):
    item=authority[role+'_review'];p=canonical(item['path']);need(fact(p)['sha256']==item['sha256'],'changed '+role+' review')
    review=load(p)
    need(review['role']==role and review['static_clear'] is True and review[flag] is True,'exact '+role+' review scope')
    need(review['implementation_manifest_sha256']==manifest_sha and review['protocol_sha256']==protocol_sha,'exact '+role+' review target')

def static(m,am):
    """Code, metadata, runtime and production bytes only; never sample bytes."""
    need(m['schema']=='electry-passive-tone-analysis-outer-implementation/v1','outer manifest schema')
    need(m['protocol_sha256']==fact(PROTOCOL)['sha256'],'outer protocol binding')
    need(m['analysis_manifest']=={'path':str(AM.relative_to(R)),**fact(AM)},'selected unchanged analysis manifest')
    need(m['analysis_protocol']=={'path':str(AP.relative_to(R)),**fact(AP)},'selected unchanged analysis protocol')
    need(am['protocol_sha256']==fact(AP)['sha256'] and am['immutable_source_commit']==BASELINE,'analysis protocol/production baseline')
    for name,h in EIGHT.items():need(fact(P/name)['sha256']==h and am['artifacts'][name]['sha256']==h,'unchanged eight-file science '+name)
    for owner,base in ((m,P),(am,P)):
        for section,root in (('artifacts',base),('inputs',R),('environment_files',Path('/'))):
            # Numerical payloads belong in the separate sample_bindings table.
            if section=='inputs':need(all(Path(n).suffix.lower() not in ('.npz','.wav','.f32','.f64') for n in owner[section]),'metadata/source inputs only')
            table(owner[section],root)
    rt=m['runtime']
    need(Path(sys.executable).absolute()==PYTHON and sys.version_info[:3]==(3,12,8),'fixed Python312 entry')
    need(str(PYTHON.resolve())==rt['python_real_executable']==am['python_real_executable'] and sys.version==rt['python_version'],'fixed actual Python bytes/version')
    need(platform.system()=='Darwin' and platform.machine()=='arm64' and platform.release()==rt['platform_release'],'fixed host')
    need(sys.path==rt['direct_entry_sys_path'],'fixed direct-entry import paths')
    for values in (rt['inherited_environment'],am['inherited_environment']):
        need({k:os.environ.get(k) for k in values}==values,'unchanged inherited runtime environment')
    need(am['numpy_version']=='1.26.4' and am['scipy_version']=='1.14.1','fixed numeric package declarations')
    need(m['production_commit']==BASELINE and len(m['Source24'])==24 and len(m['Tests'])==50,'complete production binding scope')
    need({n:f['sha256'] for n,f in m['Source24'].items()}==am['Source24_sha256'],'same original production Source hashes')
    table(m['Source24'],R);table(m['Tests'],R);bound(R/'CMakeLists.txt',m['CMakeLists'])
    # Read-only published production checks are administrative Git children,
    # independent of the one scientific child and bounded at30seconds each.
    for name,expected in m['Source24'].items():
        published=subprocess.run(['/usr/bin/git','--no-pager','show',BASELINE+':'+name],cwd=R,capture_output=True,timeout=30)
        need(published.returncode==0 and len(published.stdout)==expected['bytes'] and hashlib.sha256(published.stdout).hexdigest()==expected['sha256'],'published original production Source '+name)
    difference=subprocess.run(['/usr/bin/git','--no-pager','diff','--no-ext-diff','--no-textconv','--exit-code',BASELINE,'--','Source','Tests','CMakeLists.txt'],cwd=R,capture_output=True,timeout=30)
    need(difference.returncode==0 and not difference.stdout,'production Source/Tests/CMake diff from immutable baseline')

def authorize(path,expected,*,permit_existing_outputs=False):
    need(fact(path)['sha256']==expected,'exact outer execution authority hash')
    a=load(path);m=load(M);am=load(AM);mh=fact(M)['sha256'];ph=fact(PROTOCOL)['sha256']
    need(a['schema']=='electry-passive-tone-analysis-outer-authorization/v1' and a['one_shot_existing_analysis_authorized'] is True,'outer authorization scope')
    need(a['new_physics_render_retune_or_production_authorized'] is False,'no new mechanism/render/production scope')
    need(a['implementation_manifest_sha256']==mh and a['protocol_sha256']==ph,'exact outer authority target')
    for role in ('root','peer'):exact_review(a,role,mh,ph,'one_shot_existing_analysis_authorized')
    entry=a['analysis_authorization'];ap=canonical(entry['path']);bound(ap,{k:entry[k] for k in ('bytes','sha256')});aa=load(ap)
    need(aa['passive_source_tone_analysis_authorized'] is True and aa['implementation_manifest_sha256']==fact(AM)['sha256'] and aa['protocol_sha256']==fact(AP)['sha256'],'separate exact scientific analysis authority')
    for role in ('root','peer'):exact_review(aa,role,fact(AM)['sha256'],fact(AP)['sha256'],'passive_source_tone_analysis_authorized')
    q=aa['candidate_qualification'];qp=canonical(q['path']);need(fact(qp)['sha256']==q['sha256'],'combined all96 prerequisite binding');qr=load(qp)
    need(qr['PASS'] is True and qr['all96_mechanics_and_render_qualified'] is True and qr['render_receipt']==aa['candidate_render_receipt'],'independent full96 mechanics/render prerequisite')
    rr=aa['candidate_render_receipt'];rp=canonical(rr['path']);need(fact(rp)['sha256']==rr['sha256'],'render receipt identity');render=load(rp)
    ids=m['all96_case_ids'];need(len(ids)==len(set(ids))==96 and render['all96_rendered_after_blind_gate'] is True and [x['id'] for x in render['rows']]==ids,'complete ordered original96 render cohort')
    need(a['source_capacity_admitted'] is True and type(a['source_reserved_free_bytes']) is int and a['source_reserved_free_bytes']>=0,'fresh root capacity admission')
    static(m,am)
    need(am['exclusive_output_paths']==list(RESULT_NAMES),'only original three exclusive analysis outputs')
    if not permit_existing_outputs:need(all(not (P/n).exists() for n in RESULT_NAMES),'one-shot analysis output collision')
    return a,m,am,aa,ap

def output_inventory():
    result={}
    for p in list(OUT.iterdir())+[P/n for n in RESULT_NAMES]:
        if not p.exists():continue
        s=p.lstat();need(stat.S_ISREG(s.st_mode),'owned regular output '+str(p))
        result[str(p.relative_to(P))]=s.st_size
    return result
def resource_check(floor):
    need(shutil.disk_usage(P).free>=floor,'sampled administrative free floor/source capacity')
    need(sum(output_inventory().values())<=OUTPUT_CAP,'sampled administrative output cap')

def close_child(child,row):
    """Bounded owned-group cleanup; final reap is attempted after every stage."""
    row['cleanup_steps']=[];row['group_cleanup_steps']=[]
    row['reaped']=False;row['owned_group_closed']=False
    try:
        for signum in (None,signal.SIGINT,signal.SIGTERM,signal.SIGKILL):
            step={'signal':None if signum is None else int(signum)};row['cleanup_steps'].append(step)
            try:running=child.poll() is None
            except BaseException as error:step['poll_failure']=record_error('owned-child-poll',error);running=True
            if not running:break
            if signum is not None:
                try:os.killpg(child.pid,signum);step['signal_sent']=True
                except ProcessLookupError:step['signal_target_absent']=True
                except BaseException as error:step['signal_failure']=record_error('owned-child-signal',error)
            try:child.wait(timeout=.25 if signum is None else 5.);step['wait_reaped']=True
            except subprocess.TimeoutExpired:step['wait_timeout']=True
            except BaseException as error:step['wait_failure']=record_error('owned-child-wait',error)
    finally:
        # This attempt cannot be skipped by poll/signal/wait stage errors.
        try:row['returncode']=child.wait(timeout=.25);row['reaped']=True
        except BaseException as error:row['final_wait_failure']=record_error('owned-child-final-wait',error)
    # The direct child may have exited while descendants still own its PGID.
    # Probe and clean that same owned group even when its leader was reaped.
    for signum in (None,signal.SIGINT,signal.SIGTERM,signal.SIGKILL):
        step={'signal':None if signum is None else int(signum)};row['group_cleanup_steps'].append(step)
        try:os.killpg(child.pid,0)
        except ProcessLookupError:row['owned_group_closed']=True;break
        except BaseException as error:step['probe_failure']=record_error('owned-group-probe',error)
        if signum is None:continue
        try:os.killpg(child.pid,signum);step['signal_sent']=True
        except ProcessLookupError:row['owned_group_closed']=True;break
        except BaseException as error:step['signal_failure']=record_error('owned-group-signal',error)
        deadline=time.monotonic()+5.
        while time.monotonic()<deadline:
            try:os.killpg(child.pid,0)
            except ProcessLookupError:row['owned_group_closed']=True;break
            except BaseException as error:step['probe_failure']=record_error('owned-group-wait-probe',error);break
            time.sleep(.05)
        if row['owned_group_closed']:break
    row['group_still_present']=not row['owned_group_closed']

def declared_results():
    """Summarize existing flags/counts; never recompute metrics or improvement."""
    rows={name:{'present':(P/name).exists()} for name in RESULT_NAMES};parsed={}
    for name in RESULT_NAMES:
        if rows[name]['present']:rows[name].update(fact(P/name));parsed[name]=load(P/name)
    need('neck-analysis-v1.json' in parsed and 'bridge-analysis-v1.json' in parsed,'both planned scientific outputs required')
    n=parsed['neck-analysis-v1.json'];b=parsed['bridge-analysis-v1.json']
    need(n['letters']==b['letters']==['A','C','D','E'],'unchanged comparison letters')
    coverage=(n['all1104_rows_retained'] is True and len(n['profiles_letters'])==8 and sum(len(x['rows']) for x in n['profiles_letters'])==1104 and len(b['all64_body_absolute_windows'])==64 and len(b['all16_original_upper_prefix_cases'])==16)
    old=(n['all_old_outcomes_reproduced'] is True and len(n['all828_old_outcome_checks'])==828 and all(x['PASS'] is True for x in n['all828_old_outcome_checks']) and b['all_old_outcomes_reproduced'] is True and len(b['all192_old_outcome_checks'])==192 and all(x['PASS'] is True for x in b['all192_old_outcome_checks']))
    return {'outputs':rows,'declared_1104_neck_64_bridge_16_prefix_coverage':coverage,'original_A_C_D_reproduction_PASS':old,'neck_numeric_observation_failures':n['failures'],'bridge_numeric_observation_failures':b['failures'],'incomplete_analysis_present':'incomplete-analysis-v1.json' in parsed,'candidate_guards_and_tone_errors_remain_in_original_outputs':True,'candidate_improvement_or_production_qualification_claimed':False}

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--authorization',required=True);parser.add_argument('--authorization-sha256',required=True);args=parser.parse_args()
    need(not OUT.exists(),'once-only outer attempt collision');OUT.mkdir()
    started=utc();errors=[];child=None;handles=[];a=m=am=aa=ap=None;post=False;sample_post=False;result=None
    process={'spawn_attempted':False,'pid':None,'pgid':None,'reaped':False,'owned_group_closed':False};stage='entry'
    try:
        write_json(OUT/'attempt-v1.json',{'schema':'electry-passive-tone-analysis-outer-attempt/v1','utc':started,'pid':os.getpid(),'ppid':os.getppid(),'argv':sys.argv,'scientific_execution_started':False,'no_new_reserve':True})
        a,m,am,aa,ap=authorize(canonical(args.authorization),args.authorization_sha256)
        floor=max(OWN_FREE_FLOOR,a['source_reserved_free_bytes']);resource_check(floor)
        argv=[str(PYTHON),'-B',str(P/'run_after_review_v1.py'),'--authorization',str(ap),'--authorization-sha256',a['analysis_authorization']['sha256']]
        process['argv']=argv;stage='prepare exclusive scientific child logs'
        stdout=(OUT/'stdout-v1.txt').open('xb');handles.append(stdout);stderr=(OUT/'stderr-v1.txt').open('xb');handles.append(stderr)
        process['spawn_attempted']=True;stage='one unchanged metrics-only child'
        child=subprocess.Popen(argv,cwd=R,stdout=stdout,stderr=stderr,start_new_session=True)
        process.update(pid=child.pid,pgid=child.pid);write_json(OUT/'child-start-v1.json',process)
        while child.poll() is None:
            resource_check(floor)
            try:child.wait(timeout=POLL_SECONDS)
            except subprocess.TimeoutExpired:pass
        process['returncode']=child.returncode;resource_check(floor)
    except BaseException as error:errors.append(record_error(stage,error))
    finally:
        if child is not None:
            try:close_child(child,process)
            except BaseException as error:errors.append(record_error('owned-child-cleanup',error))
        for handle in handles:
            try:handle.flush();os.fsync(handle.fileno())
            except BaseException as error:errors.append(record_error('log-flush',error))
            finally:
                try:handle.close()
                except BaseException as error:errors.append(record_error('log-close',error))
        if m is not None and am is not None:
            try:authorize(canonical(args.authorization),args.authorization_sha256,permit_existing_outputs=True);post=True
            except BaseException as error:errors.append(record_error('post-static-bindings',error))
        if child is not None and am is not None:
            # Whole-byte sample identity only after authorized child execution;
            # the parent never decodes PCM, fields, arrays or any observation.
            try:table(am['sample_bindings'],R);sample_post=True
            except BaseException as error:errors.append(record_error('post-whole-byte-sample-bindings',error))
            try:result=declared_results()
            except BaseException as error:errors.append(record_error('scientific-output-flags',error))
    cleanup_errors=[x for x in process.get('cleanup_steps',[])+process.get('group_cleanup_steps',[]) if any(k.endswith('failure') for k in x)]
    cleanup_errors.extend([{k:v} for k,v in process.items() if k.endswith('failure')])
    complete=(child is not None and process.get('returncode')==0 and process['reaped'] and process['owned_group_closed'] and not cleanup_errors and post and sample_post and not errors and result is not None and result['declared_1104_neck_64_bridge_16_prefix_coverage'] and result['original_A_C_D_reproduction_PASS'] and not result['neck_numeric_observation_failures'] and not result['bridge_numeric_observation_failures'] and not result['incomplete_analysis_present'])
    closure={'schema':'electry-passive-tone-analysis-outer-closure/v1','start_utc':started,'closed_utc':utc(),'run_closed':child is None or (process['reaped'] and process['owned_group_closed']),'execution_and_fixed_comparison_complete':complete,'process':process,'native_cleanup_errors':cleanup_errors,'post_static_bindings_PASS':post,'post_whole_byte_sample_bindings_PASS':sample_post,'scientific_output_facts':result,'failures':errors,'new_physics_render_retune_RT_tone_improvement_or_production_qualified':False,'no_retry_or_science_changes':True}
    if m is not None:closure['outer_manifest']=fact(M);closure['outer_protocol']=fact(PROTOCOL)
    try:
        closure['owned_output_inventory_bytes']=output_inventory();write_json(OUT/'closure-v1.json',closure)
        print(json.dumps({'execution_and_fixed_comparison_complete':complete,'closure':fact(OUT/'closure-v1.json')},allow_nan=False),flush=True)
    except BaseException as error:
        print(json.dumps({'execution_and_fixed_comparison_complete':False,'closure_failure':record_error('closure-write',error),'previous_failures':errors},allow_nan=False),flush=True);return 1
    return 0 if complete else 1

if __name__=='__main__':raise SystemExit(main())
