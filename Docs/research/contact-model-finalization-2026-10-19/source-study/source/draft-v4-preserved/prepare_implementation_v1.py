"""Metadata/AST-only preparation; NEVER imports source math or opens arrays.

Run only after draft comments close and the independent damped unit is sealed.
This preparation creates no state authority; both exact role reviews are fresh.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
from fractions import Fraction
import ast,argparse,hashlib,json,os,sys,datetime
P=Path(__file__).resolve().parent;R=P.parent
BASE='6c83ff453ff7ac346598e8e73367ced128a90838'
PYTHON=Path('/Users/vojta/Dev/electry-real-dry-20261018/build-real-dry-review/python312/bin/python')
def require(ok,msg):
    if not ok:raise RuntimeError(msg)
def sha(q):return hashlib.sha256(Path(q).read_bytes()).hexdigest()
def read(q):return json.loads(Path(q).read_text())
def fact(q):return {'bytes':Path(q).stat().st_size,'sha256':sha(q)}
def write_new(q,obj):
    with Path(q).open('x')as f:json.dump(obj,f,indent=2,allow_nan=False);f.write('\n')
def main():
    a=argparse.ArgumentParser();a.add_argument('--unit-qualification',required=True);args=a.parse_args()
    require(not(P/'implementation-manifest-v1.json').exists(),'source implementation already frozen')
    cm_path=R/'build-damped-collocated-source-port-coefficients-py312/implementation-manifest-v1.json';cm=read(cm_path);env=cm['environment']
    require(str(Path(sys.executable).resolve())==str(PYTHON.resolve())==env['interpreter_path'],'established Python312 only')
    require(sys.version==env['python_version'],'Python312 exact version')
    for k,v in env['inherited_environment'].items():require(os.environ.get(k)==v,'pinned inherited environment '+k)
    unit=R/args.unit_qualification;uv=read(unit);require(uv.get('PASS')is True,'independent damped unit qualification pending/failed')
    protocol=read(P/'protocol-selected-v1.json');selected=protocol['selected_files']
    artifacts={}
    for name in selected+['protocol-selected-v1.json','README-selected-v1.md']:
        q=P/name
        if q.suffix=='.py':ast.parse(q.read_text(),filename=str(q))
        artifacts[name]=fact(q)
    Cpath='build-inherited-field-C/protocol.json';prepath='build-inherited-field-C/alpha-preflight-receipt.json'
    C=read(R/Cpath);pre=read(R/prepath)
    require([c['id']for c in C['all96_cases']]==[x['id']for x in pre['rows']]and len(pre['rows'])==96,'exact96 metadata-only joins')
    inputs={};samples={}
    # Previous immutable hash facts are copied, not freshly computed for any
    # parameter member/NPZ or coefficient archive. Only stat is checked here.
    for name,old in cm['inputs'].items():
        if name.endswith('.npz'):
            require((R/name).stat().st_size==old['bytes'],'recorded parameter size '+name);samples[name]=old
        else:inputs[name]=fact(R/name)
    closed_path='build-damped-collocated-source-port-coefficients-root-review/execution-receipt-py312-v1.json';closed=read(R/closed_path)
    rows_path='build-damped-collocated-source-port-coefficients-py312/qualification-v1/all-coefficients.jsonl.gz'
    rows_fact=closed['result_files']['qualification-v1/all-coefficients.jsonl.gz']
    require((R/rows_path).stat().st_size==rows_fact['bytes'],'recorded qualified table size');samples[rows_path]=rows_fact
    for c in C['all96_cases']:
        d=c['artifacts']['old_geometry_metadata'];require(fact(R/d['path'])=={'bytes':d['bytes'],'sha256':d['sha256']},'old geometry identity');inputs[d['path']]=fact(R/d['path'])
    required=[str(cm_path.relative_to(R)),Cpath,prepath,closed_path,args.unit_qualification,
        'build-next-tone-mechanism-synthesis/conditional-candidate-v2.json',
        'build-damped-collocated-source-port-coefficients-root-review/qualification-receipt-v1.json',
        'build-damped-collocated-source-port-premium-review/saved-coefficient-audit/completed-receipt-v1.json',
        'build-damped-collocated-source-port-premium-review/saved-coefficient-audit/final-manifest-v1.json',
        'build-certified-contact-rt-replay/exact_check_v2.py',
        'build-certified-contact-rt-replay/driver',
        'build-damped-native-unit-fixture/native_solver_v1.py',
        'build-damped-native-unit-fixture/implementation-manifest-v1.json',
        'build-damped-native-unit-fixture/protocol.json',
        'build-damped-field-neck-body/DriverGeometry.cpp','build-damped-field-bridge-upper/GeometryDriver.cpp',
        'build-physical-source-pickup-bridge/hook/source/Source/DSP/ElectryEngine.cpp',
        'build-damped-collocated-source-port-coefficients-py312/coefficients_draft_v1.py']
    # Resolve driver filename from the already-qualified damped unit manifest,
    # avoiding guessed binaries or any compile/rebuild/execute operation.
    unit_manifest=read(R/'build-damped-native-unit-fixture/implementation-manifest-v1.json')
    native=unit_manifest['native_driver'];required.remove('build-certified-contact-rt-replay/driver');required.append(native['path'])
    require(fact(R/native['path'])['sha256']==native['sha256'],'immutable native binary identity')
    for rel in required:inputs[rel]=fact(R/rel)
    def dep(path,fields):return {'path':path,'sha256':sha(R/path),'required_fields':fields}
    cf='build-damped-collocated-source-port-coefficients-root-review/qualification-receipt-v1.json'
    ia='build-damped-collocated-source-port-premium-review/saved-coefficient-audit/completed-receipt-v1.json'
    require(read(R/cf).get('PASS')is True and read(R/ia).get('all_independent_coefficients_qualified')is True,'qualified coefficient dependencies')
    physical=Fraction(171136785840078848,3141016124975315625)
    m={'schema':'electry-passive-source-implementation/v1','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'status':'Immutable source mechanics only; NO state authority until exact root+peer fresh authorization',
        'protocol_sha256':sha(P/'protocol-selected-v1.json'),'artifacts':artifacts,'inputs':inputs,'sample_bindings':samples,
        'environment_files':env['files'],'python_real_executable':env['interpreter_path'],'python_version':env['python_version'],
        'sys_path':[str(P)]+env['sys_path'][1:],'inherited_environment':env['inherited_environment'],'packages':env['packages'],
        'immutable_source_commit':BASE,'Source24':cm['Source24_sha256'],'all96_case_ids':[x['id']for x in pre['rows']],
        'C_protocol_path':Cpath,'C_preflight_path':prepath,'coefficient_rows_path':rows_path,'coefficient_rows_sha256':rows_fact['sha256'],
        'coefficient_qualification':dep(cf,{'PASS':True}),
        'independent_coefficient_audit':dep(ia,{'audit_consistency_PASS':True,'all_independent_coefficients_qualified':True,'rows':70544,'actual_C_hooks':68240,'unit_responses':423264,'discrepancies':0,'unavailable':0,'unqualified':0}),
        'damped_unit_qualification':dep(args.unit_qualification,{'PASS':True}),
        'native_driver':native,'fixed_native_physical_lambda_pair':[physical.numerator,physical.denominator],
        'qualified_analytic_api':{'path':'build-damped-collocated-source-port-coefficients-py312/coefficients_draft_v1.py','sha256':sha(R/'build-damped-collocated-source-port-coefficients-py312/coefficients_draft_v1.py')},
        'preparation_array_contents_hashes_or_members_read':False,'numeric_imports_coefficients_native_forces_states_fields_PCM_called':False}
    write_new(P/'implementation-manifest-v1.json',m)
    write_new(P/'preparation-receipt-v1.json',{'schema':'electry-passive-source-preparation/v1','manifest_sha256':sha(P/'implementation-manifest-v1.json'),
        'metadata_AST_only':True,'parameter_NPZ_and_coefficient_archive_STAT_only':True,'counts':{'artifacts':len(artifacts),'inputs':len(inputs),'recorded_samples':len(samples),'environment':len(env['files']),'Source':len(m['Source24']),'contexts':96},'states_authorized':False})
if __name__=='__main__':main()
