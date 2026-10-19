"""UNEXECUTED E tap export + immutable pickup render DRAFT.

No NumPy import, source/output payload read, compiler or renderer on import.
There is no execution authority or selected manifest in this draft directory.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
import argparse,datetime,hashlib,importlib.util,json,os,platform,shutil,subprocess,sys,traceback
P=Path(__file__).resolve().parent;R=P.parent
SOURCE='build-passive-source-tone-candidate-iofix'
SOURCE_MANIFEST='5c7a7fdbdf894367e0dedf72cfb48f568defb3474b20f821a2c4de87dbddc97f'
SOURCE_PROTOCOL='7d6c94db920cd607a0fccdfe05cb236cf65d9e8a9b63adf75d5a11a2fc138058'
C_PROTOCOL='c35ba0501f022f93454f3d007cab4c5dd98d193832dc3ef704b9e0f90fb030f9'
BASE='6c83ff453ff7ac346598e8e73367ced128a90838'
METADATA_LOG_ALLOWANCE_BYTES=64*1024**2 # Administrative only; no reserve layer or physical parameter.

def require(ok,why):
    if not ok:raise RuntimeError(why)
def sha(path):
    with Path(path).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def fact(path):return {'bytes':Path(path).stat().st_size,'sha256':sha(path)}
def read(path):return json.loads(Path(path).read_text())
def exclusive_json(path,obj):
    with Path(path).open('x')as f:json.dump(obj,f,indent=2,allow_nan=False);f.write('\n')
def bound(binding):
    require(not Path(binding['path']).is_absolute()and'..'not in Path(binding['path']).parts,'repo-relative receipt path')
    path=R/binding['path'];require(sha(path)==binding['sha256'],'changed receipt '+binding['path']);return path,read(path)
def verify_table(table,base):
    for name,f in table.items():
        if base!=Path('/'):require(not Path(name).is_absolute()and'..'not in Path(name).parts,'relative binding base '+name)
        require(fact(base/name)==f,'bound file changed '+name)

def gate(authpath,expected_auth_sha):
    """All future seals are exact, mechanical-only; no circular render gate."""
    require(sha(authpath)==expected_auth_sha,'exact export/render authorization')
    auth=read(authpath);mp=P/'implementation-manifest-v1.json';manifest=read(mp)
    mh=sha(mp);ph=sha(P/'protocol-draft-v1.json')
    require(auth['passive_source_export_render_authorized']is True and auth['implementation_manifest_sha256']==mh
        and auth['protocol_sha256']==ph and manifest['protocol_sha256']==ph,'fresh exact export/render target')
    for role in('root','peer'):
        _,review=bound(auth[role+'_review'])
        require(review['role']==role and review['static_clear']is True and review['passive_source_export_render_authorized']is True
            and review['implementation_manifest_sha256']==mh and review['protocol_sha256']==ph,'exact export/render role '+role)
    _,qualification=bound(auth['mechanics_qualification'])
    require(qualification['PASS']is True and qualification['all96_mechanics_qualified']is True,'independent all96 mechanical prerequisite')
    require(qualification['source_root']==SOURCE and qualification['source_manifest_sha256']==SOURCE_MANIFEST
        and qualification['source_protocol_sha256']==SOURCE_PROTOCOL,'IO-successor mechanics only; original failed run forbidden')
    # A future root qualification flattening the CLOSED producer + independent
    # saved-audit manifests supplies repo-relative bindings. Rehash them all,
    # rather than trusting a current self-report or an unbound raw tap file.
    require(qualification['independent_saved_audit_completed']is True,'independent saved mechanics audit incomplete')
    bound(qualification['independent_saved_audit_receipt'])
    _,closed=bound(qualification['closed_source_run'])
    require(closed['run_closed']is True and closed['process_exit_code']==closed['exit_code']==0 and closed['completed']is True,
        'producer did not close a complete successful run')
    require(closed['selected_root']==SOURCE and closed['implementation_manifest_sha256']==SOURCE_MANIFEST
        and closed['protocol_sha256']==SOURCE_PROTOCOL and closed['post_static_input_bindings_PASS']is True,'closed source identity/bindings')
    require(sha(R/SOURCE/'implementation-manifest-v1.json')==SOURCE_MANIFEST
        and sha(R/SOURCE/'protocol-selected-v1.json')==SOURCE_PROTOCOL,'source implementation identity')
    critical_static=[SOURCE+'/implementation-manifest-v1.json',SOURCE+'/protocol-selected-v1.json',
        'build-inherited-field-C/protocol.json','build-inherited-field-C/identity-receipt.json',
        'build-inherited-field-C/C-render-receipt.json','build-damped-field-neck-body/DriverGeometry.cpp',
        'build-damped-field-neck-body/hook/geometry-renderer','build-damped-field-bridge-upper/GeometryDriver.cpp',
        'build-damped-field-bridge-upper/hook/geometry-renderer',
        'build-physical-source-pickup-bridge/hook/source/Source/DSP/ElectryEngine.cpp']
    require(all(name in manifest['inputs']for name in critical_static),'static source/driver/control identity proof omitted')
    for section,base in(('artifacts',P),('inputs',R),('environment_files',Path('/'))):verify_table(manifest[section],base)
    require(str(Path(sys.executable).resolve())==manifest['python_real_executable']and sys.version==manifest['python_version'],
        'established fixed Python312')
    require(platform.system()=='Darwin'and platform.machine()=='arm64'and sys.path==manifest['direct_entry_sys_path'],'fixed host/import paths')
    for key,value in manifest['inherited_environment'].items():require(os.environ.get(key)==value,'inherited environment '+key)
    for rel,h in manifest['Source24'].items():
        require(sha(R/rel)==h,'current Source '+rel)
        require(hashlib.sha256(subprocess.check_output(['git','show',BASE+':'+rel],cwd=R)).hexdigest()==h,'published Source '+rel)
    require(not subprocess.check_output(['git','diff',BASE,'--','Source','Tests','CMakeLists.txt'],cwd=R),'production/Test/CMake changes')
    # This is the FIRST prospective source-result hash/read point. It is
    # unreachable before exact new auth and independent all96 qualification.
    verify_table(qualification['bindings'],R)
    planpath=R/'build-inherited-field-C/protocol.json';require(sha(planpath)==C_PROTOCOL,'fixed C contexts')
    plan=read(planpath);require(plan['Source24_sha256']==manifest['Source24'],'same fixed production source')
    cases=plan['all96_cases'];ids=[c['id']for c in cases]
    require(len(ids)==len(set(ids))==96 and ids==qualification['all96_case_ids']==manifest['all96_case_ids'],'exact all96 context order')
    prepath=R/SOURCE/'run-v1/study/preflight-receipt-v1.json';aggpath=R/SOURCE/'run-v1/study/aggregate-v1.json'
    for path in(prepath,aggpath):require(str(path.relative_to(R))in qualification['bindings'],'unreviewed source summary '+str(path))
    pre=read(prepath);aggregate=read(aggpath)
    require(pre['all96_inputs_PASS']is True and aggregate['all96_completed']is True and aggregate['all96_mechanics_selfreported_PASS']is True,
        'complete all96 source evidence required, never a subset')
    require([r['id']for r in pre['rows']]==[r['id']for r in aggregate['rows']]==ids and all(r['PASS']for r in aggregate['rows']),
        'source context/qualification mismatch')
    for case in cases:
        rel=SOURCE+'/run-v1/'+case['id'].replace('/','--')+'/model96/raw-mechanics-physical-tap.npz'
        require(rel in qualification['bindings'],'all96 raw mechanical taps must be independently bound '+rel)
        for name in('raw-mechanics-physical-tap.npz','completion.json','trajectory.json','all-active-native-certificates.jsonl.gz'):
            require(SOURCE+'/run-v1/'+case['id'].replace('/','--')+'/model96/'+name in qualification['bindings'],
                'mechanical history provenance missing '+case['id']+'/'+name)
    require(not any((P/n).exists()for n in('fields','E','E-field-export-v1.json','E-render-v1.json','execution-attempt-v1.json','completion-v1.json','failure-v1.json')),
        'existing export/render attempt; no retry/overwrite')
    return auth,manifest,qualification,plan,pre,aggregate

def run(authpath,authsha):
    auth,manifest,qualification,plan,pre,aggregate=gate(authpath,authsha)
    # Exact required payload bytes plus a labelled administrative allowance.
    # This only checks existing space; it allocates no reserve and never
    # shortens a field, crop, callback stream or context if space is missing.
    field_bytes=sum(8*p['calls96']for p in pre['rows']);pcm_bytes=96*96000*4
    required=field_bytes+pcm_bytes+METADATA_LOG_ALLOWANCE_BYTES;available=shutil.disk_usage(P).free
    require(available>=required,'insufficient space for ALL96 exact fields/PCM plus metadata/log allowance')
    io_space={'exact_field_bytes':field_bytes,'exact_PCM_bytes':pcm_bytes,
        'administrative_metadata_log_allowance_bytes':METADATA_LOG_ALLOWANCE_BYTES,
        'total_required_bytes':required,'available_bytes_before_writes':available,'new_reserve_layer':False}
    # No source/coefficient/trajectory module is imported by this export.
    spec=importlib.util.find_spec('numpy')
    require(spec is not None and spec.origin is not None and str(Path(spec.origin).resolve().parent)==manifest['numpy']['root']
        and [str(Path(p).resolve())for p in spec.submodule_search_locations or []]==[manifest['numpy']['root']],
        'fixed NumPy root BEFORE import')
    import numpy as np
    require(np.__version__==manifest['numpy']['version']and str(Path(np.__file__).resolve().parent)==manifest['numpy']['root'],'fixed loaded NumPy')
    exclusive_json(P/'execution-attempt-v1.json',{'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'authorization_sha256':authsha,'closed':False,'complete':False,'no_compilation_or_new_physical_states':True,
        'administrative_IO_space_preflight':io_space})
    rows=[];exports=[];stage={'name':'validate ALL96 saved mechanical tap arrays before first field write'}
    try:
        prepared=[]
        for case,p,a in zip(plan['all96_cases'],pre['rows'],aggregate['rows']):
            rel=SOURCE+'/run-v1/'+case['id'].replace('/','--')+'/model96/raw-mechanics-physical-tap.npz';path=R/rel
            require(fact(path)==qualification['bindings'][rel],'qualified raw tap changed immediately before read')
            with np.load(path,allow_pickle=False)as z:
                require(set(z.files)=={'frames','tap'},'raw mechanical tap schema')
                frames=z['frames'].copy();tap=z['tap'].copy()
            require(tap.dtype==np.dtype('<f8')and tap.shape==(p['calls96'],)and frames.dtype.kind in('i','u')
                and np.array_equal(frames,np.arange(p['calls96']))and np.isfinite(tap).all(),'exact callback-indexed finite float64 tap')
            oldpath=R/case['artifacts']['old_geometry_metadata']['path'];require(fact(oldpath)=={k:case['artifacts']['old_geometry_metadata'][k]for k in('bytes','sha256')},'old geometry identity')
            old=read(oldpath)
            require(p['id']==a['id']==case['id']and p['calls96']==old['calls']and p['first_clock']==old['first_clock']
                and p['contact_frame96']==old['first_contact_index']and p['release_frame96']==old['first_release_index'],'unaltered callback/phase clock mapping')
            require(a['model96']['PASS']is True and a['model192']['PASS']is True and a['reference_gate_PASS']is True
                and a['all5_rate_rows_PASS']is True,'all fixed mechanics contexts admitted')
            prepared.append((case,p,a,path,tap,old))
        require(len(prepared)==96,'validate all96 before any field export')
        (P/'fields').mkdir()
        for case,p,a,path,tap,old in prepared:
            stage={'name':'copy qualified full tap bits','id':case['id']}
            folder=P/'fields'/case['scope'];folder.mkdir(exist_ok=True);field=folder/(case['old_key']+'.f64')
            # Native little-endian f64 callback stream, exactly as immutable C
            # drivers read it. No gain, fade, interpolation, tail regeneration,
            # phase alignment, Contact overwrite or field resampling occurs.
            bits=tap.tobytes(order='C')
            with field.open('xb')as file:require(file.write(bits)==len(bits),'short raw field write')
            require(field.stat().st_size==8*p['calls96']and field.read_bytes()==bits,'exported tap byte identity')
            exports.append({'id':case['id'],'scope':case['scope'],'old_key':case['old_key'],
                'source_tap':{'path':str(path.relative_to(R)),**fact(path)},'field_path':str(field.relative_to(P)),
                'field_sha256':sha(field),'field_bytes':field.stat().st_size,'tap_bits_unchanged':True,
                'calls':p['calls96'],'first_clock':p['first_clock'],'first_contact_index':p['contact_frame96'],
                'first_release_index':p['release_frame96'],'callback_index0_maps_to_engine_clock':p['first_clock'],
                'physical_Contact_engine_clock':p['first_clock']+p['contact_frame96'],
                'cropclockorigin':1,'no_field_interpolation_gain_fade_or_new_state':True})
        exclusive_json(P/'E-field-export-v1.json',{'schema':'electry-passive-source-E-field-export/v1','rows':exports,
            'all96_qualified_mechanics_exported_bit_identically':True,'mechanics_qualification':auth['mechanics_qualification'],
            'source_root':SOURCE,'source_manifest_sha256':SOURCE_MANIFEST,'source_protocol_sha256':SOURCE_PROTOCOL,
            'no_renderer_or_combined_qualification_yet':True})
        (P/'E').mkdir()
        for (case,p,a,path,tap,old),export in zip(prepared,exports):
            stage={'name':'immutable external-tap pickup renderer','id':case['id']}
            field=P/export['field_path'];require(sha(field)==export['field_sha256'],'export field changed before render')
            folder=P/'E'/case['scope'];folder.mkdir(exist_ok=True);out=folder/case['old_key']
            if case['scope']=='neck':
                renderer=R/'build-damped-field-neck-body/hook/geometry-renderer'
                args=[str(case['profile']['pickPosition']),str(case['note']['string_index']),str(case['note']['sounding_midi']),'1',str(field)]
                metadata=out.with_suffix('.json')
            else:
                renderer=R/'build-damped-field-bridge-upper/hook/geometry-renderer'
                args=[str(case['profile']['pick_position']),str(case['profile']['pickup_type']),'1',str(field)]
                metadata=folder/(case['old_key']+'-voice.json')
            rel=str(renderer.relative_to(R));require(fact(renderer)=={k:plan['inputs'][rel][k]for k in('bytes','sha256')},'frozen C driver identity')
            result=subprocess.run([str(renderer),str(out),*args],capture_output=True,text=True)
            exclusive_json(folder/(case['old_key']+'-render-log.json'),{'argv':[str(renderer),str(out),*args],
                'renderer_sha256':sha(renderer),'returncode':result.returncode,'stdout':result.stdout,'stderr':result.stderr})
            require(result.returncode==0,'immutable renderer failed; partial files retained')
            m=read(metadata)
            require({k:v for k,v in m.items()if k not in('rms','peak')}=={k:v for k,v in old.items()if k not in('rms','peak')},
                'actual controls/geometry/callbacks/phase timeline changed')
            pcm=out.with_suffix('.f32');audio=np.fromfile(pcm,dtype='<f4')
            require(pcm.stat().st_size==384000 and audio.shape==(96000,)and np.isfinite(audio).all(),'unchanged two-second mono native48k finite PCM')
            rows.append({'id':case['id'],'scope':case['scope'],'old_key':case['old_key'],
                'PCM_path':str(pcm.relative_to(P)),'PCM_sha256':sha(pcm),'metadata_path':str(metadata.relative_to(P)),
                'metadata_sha256':sha(metadata),'field_path':export['field_path'],'field_sha256':export['field_sha256'],
                'renderer_sha256':sha(renderer),'all_metadata_timeline_equal_except_RMS_peak':True,
                'calls':m['calls'],'first_clock':m['first_clock'],'first_contact_index':m['first_contact_index'],
                'first_release_index':m['first_release_index'],'cropclockorigin':1,
                'actual_E_physical_Fq_N':a['model96']['actual_physical_Fq_N'],
                'actual_E_physical_stroke_D_m':a['model96']['actual_physical_stroke_D_m'],
                'actual_E_force_work_provenance':auth['mechanics_qualification'],
                'legacy_renderer_contact_force_metadata_is_NOT_actual_E_force':True,
                'raw_rms':float(np.sqrt(np.mean(audio.astype(float)**2))),'raw_peak':float(np.max(abs(audio)))})
        require(len(rows)==96 and [r['id']for r in rows]==manifest['all96_case_ids'],'all96 complete renders required')
        receipt={'schema':'electry-passive-source-E-render/v1','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'all96_rendered_after_blind_gate':True,'rows':rows,'mechanics_qualification':auth['mechanics_qualification'],
            'source_root':SOURCE,'source_manifest_sha256':SOURCE_MANIFEST,'source_protocol_sha256':SOURCE_PROTOCOL,
            'field_export_receipt':{'path':str((P/'E-field-export-v1.json').relative_to(R)),**fact(P/'E-field-export-v1.json')},
            'export_render_manifest_sha256':sha(P/'implementation-manifest-v1.json'),'protocol_sha256':sha(P/'protocol-draft-v1.json'),
            'authorization_sha256':authsha,'combined_independent_mechanics_and_render_qualification_pending':True,
            'analysis_PCM_background_or_tone_metrics_authorized':False,
            'limits':'Isolated source/native pickup diagnostic; no continuous performance/attack/DI-calibration/RT/95percent or production claim.'}
        exclusive_json(P/'E-render-v1.json',receipt)
        exclusive_json(P/'completion-v1.json',{'complete':True,'all96_rendered_after_blind_gate':True,
            'E_render_sha256':sha(P/'E-render-v1.json'),'independent_render_validation_and_analysis_authorization_pending':True})
        return receipt
    except BaseException as error:
        # No retry, regeneration, level correction, subset qualification or
        # successful E-render receipt is manufactured after an incomplete run.
        try:
            exclusive_json(P/'failure-v1.json',{'complete':False,'stage':stage,'completed_export_ids':[r['id']for r in exports],
                'completed_render_ids':[r['id']for r in rows],'original_error':repr(error),'traceback':traceback.format_exc(),
                'partial_outputs_preserved':True,'no_retry_or_source_parameter_change':True})
        except BaseException:pass # Never replace the original I/O/renderer error.
        raise

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--authorization',type=Path,required=True);parser.add_argument('--authorization-sha256',required=True)
    args=parser.parse_args();run(args.authorization,args.authorization_sha256)
