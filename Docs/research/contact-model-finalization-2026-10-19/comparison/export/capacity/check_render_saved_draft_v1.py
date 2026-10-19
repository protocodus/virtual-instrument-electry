"""UNEXECUTED independent saved-render checker draft. No payload/import authority.

Future exact root/peer authorization must follow closed all96 mechanics/render.
This checker never launches a renderer, imports a producer, retaps or makes states.
"""
if not __debug__:raise RuntimeError('Optimized Python forbidden')
import argparse,datetime,hashlib,json,math,os,platform,struct,sys,traceback
from pathlib import Path
sys.dont_write_bytecode=True
P=Path(__file__).resolve().parent;R=P.parent
PIN=Path('/Users/vojta/Dev/electry-real-dry-20261018/build-real-dry-review/python312/bin/python')
SOURCE='build-passive-source-tone-candidate-capacity'
SM='08fc876f88fe519eb2a1a24d6126e22ef6526b19c2807c0212a30b6471485e2c'
SP='7d6c94db920cd607a0fccdfe05cb236cf65d9e8a9b63adf75d5a11a2fc138058'
CP='c35ba0501f022f93454f3d007cab4c5dd98d193832dc3ef704b9e0f90fb030f9'
MF=P/'saved-render-review-manifest-v1.json';PROTOCOL=P/'saved-render-review-protocol-draft-v1.json'
RESULT=P/'saved-render-review-results-v1.json'

def need(ok,message):
    if not ok:raise RuntimeError(message)
def read(p):return json.loads(Path(p).read_text())
def fact(p):
    with Path(p).open('rb')as f:return {'bytes':os.fstat(f.fileno()).st_size,'sha256':hashlib.file_digest(f,'sha256').hexdigest()}
def relative(name,base=R):
    p=Path(name);need(not p.is_absolute()and'..'not in p.parts and str(p)==name,'canonical relative path')
    return base/p
def bound(binding):
    p=relative(binding['path']);need(fact(p)['sha256']==binding['sha256'],'changed bound receipt '+binding['path']);return p,read(p)
def verify(entries,base):
    for name,expected in entries.items():
        p=base/name if base==Path('/')else relative(name,base)
        need(fact(p)==expected,'changed binding '+name)
def role_reviews(a,mh,ph,flag):
    for role in('root','peer'):
        _,v=bound(a[role+'_review'])
        need(v['role']==role and v['static_clear']is True and v[flag]is True,'exact review '+role)
        need(v['implementation_manifest_sha256']==mh and v['protocol_sha256']==ph,'review target '+role)

def gate(authpath,authsha):
    need(fact(authpath)['sha256']==authsha,'exact saved-render review authority')
    a=read(authpath);m=read(MF);mh=fact(MF)['sha256'];ph=fact(PROTOCOL)['sha256']
    need(m['schema']=='electry-passive-source-saved-render-review-implementation/v1','checker manifest schema')
    need(a['schema']=='electry-passive-source-saved-render-review-authorization/v1'
        and a['saved_render_review_authorized']is True and a['new_states_or_renderer_calls_authorized']is False,'saved-only authority')
    need(a['implementation_manifest_sha256']==mh and a['protocol_sha256']==ph==m['protocol_sha256'],'exact checker target')
    role_reviews(a,mh,ph,'saved_render_review_authorized')
    _,q=bound(a['mechanics_qualification'])
    need(q['PASS']is True and q['all96_mechanics_qualified']is True and q['independent_saved_audit_completed']is True,'all96 independent mechanics')
    need(q['source_root']==SOURCE and q['source_manifest_sha256']==SM and q['source_protocol_sha256']==SP,'selected source')
    _,closed=bound(q['closed_source_run']);_,audit=bound(q['independent_saved_audit_receipt'])
    need(closed['run_closed']is True and closed['completed']is True and closed['exit_code']==closed['process_exit_code']==0
        and closed['post_static_input_bindings_PASS']is True and closed['selected_root']==SOURCE
        and closed['implementation_manifest_sha256']==SM and closed['protocol_sha256']==SP,'closed source identity')
    need(audit['audit_complete']is True and audit['all_saved_arithmetic_consistent']is True
        and audit['all96_scientifically_qualified']is True and not audit['issues']and not audit['unavailable'],'closed independent mechanics audit')
    _,ea=bound(a['export_authorization']);em=read(P/'implementation-manifest-v1.json');emh=fact(P/'implementation-manifest-v1.json')['sha256']
    eph=fact(P/'protocol-draft-v2.json')['sha256']
    need(ea['passive_source_export_render_authorized']is True and ea['implementation_manifest_sha256']==emh
        and ea['protocol_sha256']==eph==em['protocol_sha256']and ea['mechanics_qualification']==a['mechanics_qualification'],'exact export selection')
    role_reviews(ea,emh,eph,'passive_source_export_render_authorized')
    rp,render=bound(a['render_receipt']);xp,export=bound(a['field_export_receipt']);_,done=bound(a['render_completion'])
    need(rp==P/'E-render-v1.json'and xp==P/'E-field-export-v1.json','selected receipt paths')
    need(render['schema']=='electry-passive-source-E-render/v1'and render['all96_rendered_after_blind_gate']is True
        and render['export_render_manifest_sha256']==emh and render['protocol_sha256']==eph
        and render['authorization_sha256']==a['export_authorization']['sha256'],'render selection/closure')
    need(render['mechanics_qualification']==a['mechanics_qualification']and export['mechanics_qualification']==a['mechanics_qualification']
        and render['source_root']==export['source_root']==SOURCE and render['source_manifest_sha256']==export['source_manifest_sha256']==SM
        and render['source_protocol_sha256']==export['source_protocol_sha256']==SP,'render/source prerequisite identity')
    need(done['complete']is True and done['E_render_sha256']==a['render_receipt']['sha256']
        and done['post_static_input_bindings_PASS']is True and done['all96_owned_renderer_groups_closed']is True
        and render['post_static_qualification_source_input_environment_bindings']['PASS']is True,'closed render required')
    need(export['all96_qualified_mechanics_exported_bit_identically']is True
        and render['field_export_receipt']['sha256']==a['field_export_receipt']['sha256'],'field export closure')
    rt=m['runtime']
    need(Path(sys.executable).resolve()==PIN.resolve()and sys.version_info[:3]==(3,12,8)
        and str(PIN.resolve())==rt['python_real_executable']==em['python_real_executable']and sys.version==rt['python_version']==em['python_version'],'fixed Python312')
    need(platform.system()=='Darwin'and platform.machine()=='arm64'and platform.release()==rt['platform_release']
        and sys.path==rt['direct_entry_sys_path'],'fixed host/import paths')
    for values in(rt['inherited_environment'],em['inherited_environment']):
        need(all(os.environ.get(k)==v for k,v in values.items()),'fixed inherited environment')
    for owner in(m,em):
        verify(owner['artifacts'],P);verify(owner['inputs'],R);verify(owner['environment_files'],Path('/'))
    need(m['numpy']==em['numpy'],'same fixed NumPy root/version')
    for name,h in em['Source24'].items():need(fact(R/name)['sha256']==h,'unchanged production Source')
    planpath=R/'build-inherited-field-C/protocol.json';need(fact(planpath)['sha256']==CP,'original C protocol');plan=read(planpath)
    ids=[c['id']for c in plan['all96_cases']]
    need(len(ids)==len(set(ids))==96 and ids==m['all96_case_ids']==q['all96_case_ids']==[v['id']for v in render['rows']]
        ==[v['id']for v in export['rows']],'all96 ordered unique contexts')
    need(len(render['owned_renderer_attempts'])==96,'all96 child attempts')
    samples=set()
    for c in plan['all96_cases']:
        samples.add(SOURCE+'/run-v1/'+c['id'].replace('/','--')+'/model96/raw-mechanics-physical-tap.npz')
        samples.add(str((P/'fields'/c['scope']/(c['old_key']+'.f64')).relative_to(R)))
        samples.add(str((P/'E'/c['scope']/(c['old_key']+'.f32')).relative_to(R)))
    need(set(m['sample_bindings'])==samples and len(samples)==288,'exact288 tap/field/PCM payload bindings')
    # First candidate tap/field/PCM hash point: all prerequisites above passed.
    verify(m['sample_bindings'],R)
    return a,m,q,render,export,plan

def review(authpath,authsha):
    need(not RESULT.exists(),'exclusive saved-render review output')
    a,m,q,render,export,plan=gate(authpath,authsha)
    import importlib.util
    spec=importlib.util.find_spec('numpy');need(spec is not None and str(Path(spec.origin).resolve().parent)==m['numpy']['root'],'fixed NumPy preimport root')
    import numpy as np
    need(np.__version__==m['numpy']['version']=='1.26.4'and str(Path(np.__file__).resolve().parent)==m['numpy']['root'],'fixed loaded NumPy')
    prepath=R/SOURCE/'run-v1/study/preflight-receipt-v1.json';aggpath=R/SOURCE/'run-v1/study/aggregate-v1.json'
    for p in(prepath,aggpath):need(fact(p)==q['bindings'][str(p.relative_to(R))],'qualified original summary')
    pre=read(prepath);aggregate=read(aggpath);ids=m['all96_case_ids']
    need(pre['all96_inputs_PASS']is True and aggregate['all96_completed']is True and aggregate['all96_mechanics_selfreported_PASS']is True
        and [v['id']for v in pre['rows']]==[v['id']for v in aggregate['rows']]==ids,'all96 original summaries')
    rows=[];failures=[]
    for c,p,science,x,r,attempt in zip(plan['all96_cases'],pre['rows'],aggregate['rows'],export['rows'],render['rows'],render['owned_renderer_attempts']):
        cid=c['id']
        try:
            need(p['id']==science['id']==x['id']==r['id']==cid and x['scope']==r['scope']==c['scope']
                and x['old_key']==r['old_key']==c['old_key']and science['PASS']is True,'case identity/admission')
            tapname=SOURCE+'/run-v1/'+cid.replace('/','--')+'/model96/raw-mechanics-physical-tap.npz';tp=R/tapname
            need(m['sample_bindings'][tapname]==q['bindings'][tapname]==fact(tp),'sealed raw tap identity')
            need(x['source_tap']=={'path':tapname,**fact(tp)},'export source identity')
            with np.load(tp,allow_pickle=False)as z:
                need(set(z.files)=={'frames','tap'},'raw tap schema');frames=z['frames'];tap=z['tap']
            need(tap.dtype==np.dtype('<f8')and tap.shape==(p['calls96'],)and np.isfinite(tap).all()
                and frames.dtype.kind in('i','u')and np.array_equal(frames,np.arange(p['calls96'])),'complete finite callback tap')
            fieldname='fields/'+c['scope']+'/'+c['old_key']+'.f64';field=P/fieldname
            need(x['field_path']==r['field_path']==fieldname and x['field_bytes']==8*p['calls96']
                and x['field_sha256']==r['field_sha256']==fact(field)['sha256'],'field path/hash/count')
            need(m['sample_bindings'][str(field.relative_to(R))]==fact(field)and field.read_bytes()==tap.tobytes(order='C'),'independent exact tap-to-field byte copy')
            out=P/'E'/c['scope']/c['old_key'];pcm=out.with_suffix('.f32')
            metadata=out.with_suffix('.json')if c['scope']=='neck'else out.parent/(c['old_key']+'-voice.json')
            need(r['PCM_path']==str(pcm.relative_to(P))and r['metadata_path']==str(metadata.relative_to(P))
                and fact(pcm)==m['sample_bindings'][str(pcm.relative_to(R))]=={'bytes':384000,'sha256':r['PCM_sha256']}
                and fact(metadata)['sha256']==r['metadata_sha256'],'raw PCM/metadata identities')
            count=sum(1 for(v,)in struct.iter_unpack('<f',pcm.read_bytes())if math.isfinite(v))
            need(count==96000,'all96000 mono48k samples finite')
            oldfact=c['artifacts']['old_geometry_metadata'];oldpath=relative(oldfact['path'])
            need(fact(oldpath)=={k:oldfact[k]for k in('bytes','sha256')},'original geometry metadata')
            old=read(oldpath);actual=read(metadata)
            need({k:v for k,v in actual.items()if k not in('rms','peak')}=={k:v for k,v in old.items()if k not in('rms','peak')},'complete controls/phase timeline identity')
            for target in(x,r):
                need(target['calls']==p['calls96']==old['calls']and target['first_clock']==p['first_clock']==old['first_clock']
                    and target['first_contact_index']==p['contact_frame96']==old['first_contact_index']
                    and target['first_release_index']==p['release_frame96']==old['first_release_index'],'original callback/phase clocks')
            need(x['callback_index0_maps_to_engine_clock']==p['first_clock']and x['physical_Contact_engine_clock']==p['first_clock']+p['contact_frame96'],'callback origin mapping')
            renderer=R/('build-damped-field-neck-body/hook/geometry-renderer'if c['scope']=='neck'else'build-damped-field-bridge-upper/hook/geometry-renderer')
            args=[str(c['profile']['pickPosition']),str(c['note']['string_index']),str(c['note']['sounding_midi']),'1',str(field)]if c['scope']=='neck'else[str(c['profile']['pick_position']),str(c['profile']['pickup_type']),'1',str(field)]
            need(attempt['argv']==[str(renderer),str(out),*args]and attempt['renderer_sha256']==r['renderer_sha256']==fact(renderer)['sha256'],'immutable exact renderer argv/binary')
            log=read(out.parent/(c['old_key']+'-render-log.json'));start=read(out.parent/(c['old_key']+'-render-started.json'));initial=read(out.parent/(c['old_key']+'-render-attempt.json'))
            need(log==attempt and attempt['spawn_attempted']is True and type(attempt['pid'])is int and attempt['pid']>0
                and attempt['pgid']==attempt['pid']and attempt['returncode']==0 and attempt['owned_group_closed']is True
                and attempt['PASS']is True and attempt['failure']is None and not attempt['cleanup_errors']and r['owned_renderer_group_closed']is True,'saved owned-child closure')
            need(initial['argv']==attempt['argv']and initial['spawn_attempted']is False and initial['pid']is None
                and start['argv']==attempt['argv']and start['pid']==start['pgid']==attempt['pid']and start['spawn_attempted']is True,'saved attempt/start PID ownership')
            for label in('stdout','stderr'):
                logfile=out.parent/(c['old_key']+'-render-'+label+'.txt')
                need(attempt[label]=={'path':str(logfile.relative_to(P)),**fact(logfile)},'closed '+label+' log')
            need(r['actual_E_physical_Fq_N']==science['model96']['actual_physical_Fq_N']
                and r['actual_E_physical_stroke_D_m']==science['model96']['actual_physical_stroke_D_m']
                and r['actual_E_force_work_provenance']==a['mechanics_qualification']
                and r['legacy_renderer_contact_force_metadata_is_NOT_actual_E_force']is True,'separate E physical versus legacy force provenance')
            rows.append({'id':cid,'PASS':True,'field_bytes':x['field_bytes'],'PCM_bytes':384000,'finite_samples':count,
                'stderr_bytes':attempt['stderr']['bytes'],'saved_group_closed':True})
        except Exception as error:
            rows.append({'id':cid,'PASS':False});failures.append({'id':cid,'error':repr(error),'traceback':traceback.format_exc()})
    gate(authpath,authsha) # Recheck all selected input/sample bytes before a result.
    result={'schema':'electry-passive-source-independent-saved-render-review/v1','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'rows':rows,'failures':failures,'all96_render_qualified':len(rows)==96 and all(x['PASS']for x in rows)and not failures,
        'post_input_and_sample_bindings_PASS':True,'render_receipt':a['render_receipt'],
        'no_renderer_retap_new_state_force_metric_or_tone_improvement_claim':True,
        'saved_process_closure_is_not_a_live_process_probe':True}
    with RESULT.open('x')as f:json.dump(result,f,indent=2,allow_nan=False);f.write('\n')
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--authorization',type=Path,required=True);parser.add_argument('--authorization-sha256',required=True)
    args=parser.parse_args();review(args.authorization,args.authorization_sha256)
