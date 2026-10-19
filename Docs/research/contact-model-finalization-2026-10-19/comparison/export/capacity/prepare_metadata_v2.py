"""Metadata-only preparation after an actual, independently qualified source run.

No numerical imports, physical arrays, fields, PCM, compiler or renderer calls.
Existing reviewed facts are copied; selected static metadata/code are hashed.
"""
if not __debug__:
    raise RuntimeError('Optimized Python forbidden')
from pathlib import Path
import argparse, datetime, hashlib, json, sys

P=Path(__file__).resolve().parent
R=P.parent
PIN=Path('/Users/vojta/Dev/electry-real-dry-20261018/build-real-dry-review/python312/bin/python')
SOURCE='build-passive-source-tone-candidate-capacity'
SOURCE_MF='08fc876f88fe519eb2a1a24d6126e22ef6526b19c2807c0212a30b6471485e2c'
SOURCE_PROTOCOL='7d6c94db920cd607a0fccdfe05cb236cf65d9e8a9b63adf75d5a11a2fc138058'


def need(ok,message):
    if not ok:raise RuntimeError(message)


def fact(path):
    with path.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
    return {'bytes':path.stat().st_size,'sha256':digest}


def load(path):return json.loads(path.read_text())


def relative(name):
    path=Path(name)
    need(not path.is_absolute() and '..' not in path.parts and str(path)==name,'canonical relative path')
    return R/path


def main(seal,seal_sha):
    need(sys.version_info[:3]==(3,12,8) and Path(sys.executable).resolve()==PIN.resolve(),'fixed Python312')
    path=relative(seal)
    need(fact(path)['sha256']==seal_sha,'actual root mechanical seal hash')
    qualification=load(path)
    need(qualification['PASS'] is True and qualification['all96_mechanics_qualified'] is True
         and qualification['independent_saved_audit_completed'] is True,'all96 independent mechanics required')
    need(qualification['source_root']==SOURCE and qualification['source_manifest_sha256']==SOURCE_MF
         and qualification['source_protocol_sha256']==SOURCE_PROTOCOL,'selected source identity')
    for key in ('closed_source_run','independent_saved_audit_receipt'):
        b=qualification[key];need(fact(relative(b['path']))['sha256']==b['sha256'],'closed prerequisite '+key)
    closed=load(relative(qualification['closed_source_run']['path']))
    need(closed['run_closed'] is True and closed['completed'] is True and closed['exit_code']==closed['process_exit_code']==0
         and closed['post_static_input_bindings_PASS'] is True,'closed successful source')
    need(closed['selected_root']==SOURCE and closed['implementation_manifest_sha256']==SOURCE_MF
         and closed['protocol_sha256']==SOURCE_PROTOCOL,'closed source identity')
    audited=load(relative(qualification['independent_saved_audit_receipt']['path']))
    need(audited['audit_complete'] is True and audited['all_saved_arithmetic_consistent'] is True
         and audited['all96_scientifically_qualified'] is True and not audited['issues'] and not audited['unavailable'],
         'completed independent all96 audit')
    sm=R/SOURCE/'implementation-manifest-v1.json'
    need(fact(sm)['sha256']==SOURCE_MF and fact(R/SOURCE/'protocol-selected-v1.json')['sha256']==SOURCE_PROTOCOL,'source manifests')
    source=load(sm)
    cp=R/'build-inherited-field-C/protocol.json'
    need(fact(cp)['sha256']=='c35ba0501f022f93454f3d007cab4c5dd98d193832dc3ef704b9e0f90fb030f9','original C protocol')
    plan=load(cp)
    ids=[x['id'] for x in plan['all96_cases']]
    need(len(ids)==len(set(ids))==96 and ids==qualification['all96_case_ids']==source['all96_case_ids'],'all96 ordered contexts')
    inputs={}

    def expected(name,binding):
        p=relative(name);entry={k:binding[k] for k in ('bytes','sha256')}
        need(p.is_file() and p.stat().st_size==entry['bytes'],'static input present '+name)
        need(name not in inputs or inputs[name]==entry,'conflicting static binding '+name)
        inputs[name]=entry

    def actual(name):expected(name,fact(relative(name)))

    for name,binding in plan['inputs'].items():expected(name,binding)
    for case in plan['all96_cases']:
        item=case['artifacts']['old_geometry_metadata'];expected(item['path'],item)
    for name in (SOURCE+'/implementation-manifest-v1.json',SOURCE+'/protocol-selected-v1.json',
                 'build-inherited-field-C/protocol.json','build-inherited-field-C/identity-receipt.json',
                 'build-inherited-field-C/C-render-receipt.json','build-damped-field-neck-body/DriverGeometry.cpp',
                 'build-damped-field-neck-body/hook/geometry-renderer','build-damped-field-bridge-upper/GeometryDriver.cpp',
                 'build-damped-field-bridge-upper/hook/geometry-renderer',
                 'build-physical-source-pickup-bridge/hook/source/Source/DSP/ElectryEngine.cpp',seal):actual(name)
    artifacts={name:fact(P/name) for name in ('export_render_draft_v2.py','protocol-draft-v2.json','prepare_metadata_v2.py',
                                             'export_render_draft_v1.py','protocol-draft-v1.json','routing-delta-receipt-v1.json')}
    manifest={'schema':'electry-passive-source-export-render-implementation/v2',
        'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'protocol_sha256':artifacts['protocol-draft-v2.json']['sha256'],'artifacts':artifacts,'inputs':inputs,
        'environment_files':source['environment_files'],'Source24':source['Source24'],
        'python_real_executable':source['python_real_executable'],'python_version':source['python_version'],
        'direct_entry_sys_path':[str(P),*source['sys_path'][1:]],'inherited_environment':source['inherited_environment'],
        'numpy':source['packages']['numpy'],'all96_case_ids':ids,
        'mechanics_qualification':{'path':seal,**fact(path)},
        'metadata_preparation_only':True,'numeric_imports_payload_or_renderer_calls':False}
    target=P/'implementation-manifest-v1.json'
    with target.open('x') as stream:stream.write(json.dumps(manifest,indent=2,allow_nan=False)+'\n')
    receipt={'manifest':fact(target),'selected_protocol':artifacts['protocol-draft-v2.json'],
             'qualification':{'path':seal,**fact(path)},'static_inputs':len(inputs),'artifacts':len(artifacts),
             'execution_authorized':False,'numerical_imports_or_payload_access':False}
    with (P/'preparation-receipt-v2.json').open('x') as stream:stream.write(json.dumps(receipt,indent=2,allow_nan=False)+'\n')
    print(json.dumps(receipt))


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--seal',required=True);parser.add_argument('--seal-sha256',required=True)
    args=parser.parse_args();main(args.seal,args.seal_sha256)
