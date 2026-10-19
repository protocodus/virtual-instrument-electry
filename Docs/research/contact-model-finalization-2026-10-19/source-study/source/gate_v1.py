"""Unexecuted source-only exacthash gate; stdlib only before authorization."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
import hashlib,json,os,platform,subprocess,sys
P=Path(__file__).resolve().parent;R=P.parent

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb')as f:
        for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
    return h.hexdigest()
def require(ok,message):
    if not ok:raise RuntimeError(message)
def read(path):return json.loads(Path(path).read_text())
def verify_bindings(manifest):
    for section,base in [('artifacts',P),('inputs',R),('sample_bindings',R),('environment_files',Path('/'))]:
        for rel,fact in manifest[section].items():
            q=base/rel;require(q.stat().st_size==fact['bytes']and sha(q)==fact['sha256'],'bound source input '+rel)
    for rel,digest in manifest['Source24'].items():
        require(sha(R/rel)==digest,'current Source '+rel)
        raw=subprocess.check_output(['git','show',manifest['immutable_source_commit']+':'+rel],cwd=R)
        require(hashlib.sha256(raw).hexdigest()==digest,'published Source '+rel)
    require(not subprocess.check_output(['git','diff',manifest['immutable_source_commit'],'--','Source','Tests','CMakeLists.txt'],cwd=R),'production/Test/CMake changes')

def require_review():
    require(__debug__,'optimized Python forbidden')
    m=read(P/'implementation-manifest-v1.json');mh=sha(P/'implementation-manifest-v1.json');ph=sha(P/'protocol-selected-v1.json')
    require(m['protocol_sha256']==ph,'selected protocol identity')
    pointer=read(P/'review-pointer-v1.json');require(pointer['source_states_authorized']is True,'source state pointer')
    ap=R/pointer['authorization_path'];require(sha(ap)==pointer['authorization_sha256'],'source authorization identity')
    auth=read(ap)
    require(auth['passive_source_states_authorized']is True and auth['implementation_manifest_sha256']==mh and auth['protocol_sha256']==ph,'source authorization target')
    for role in ('root','peer'):
        b=auth[role+'_review'];p=R/b['path'];require(sha(p)==b['sha256'],'source review identity')
        v=read(p);require(v['role']==role and v['static_clear']is True and v['passive_source_states_authorized']is True and v['implementation_manifest_sha256']==mh and v['protocol_sha256']==ph,'exact source role review')
    verify_bindings(m)
    require(str(Path(sys.executable).resolve())==m['python_real_executable']and sys.version==m['python_version'],'fixed Python312')
    require([str(Path(p).resolve())for p in sys.path]==m['sys_path'],'fixed package lookup path')
    require(platform.system()=='Darwin'and platform.machine()=='arm64','fixed strict native host')
    for key,value in m['inherited_environment'].items():require(os.environ.get(key)==value,'inherited environment '+key)
    for dep in ('coefficient_qualification','independent_coefficient_audit','damped_unit_qualification'):
        binding=m[dep];p=R/binding['path'];require(sha(p)==binding['sha256'],'required qualified dependency '+dep)
        v=read(p)
        for key,value in binding['required_fields'].items():require(v.get(key)==value,'required dependency fact '+dep+'/'+key)
    require(m['independent_coefficient_audit']['required_fields'].get('all_independent_coefficients_qualified')is True,'independent coefficient gate omitted')
    require(m['coefficient_qualification']['required_fields'].get('PASS')is True and m['damped_unit_qualification']['required_fields'].get('PASS')is True,'prior qualification omitted')
    C=read(R/m['C_protocol_path']);pre=read(R/m['C_preflight_path'])
    require(len(C['all96_cases'])==len(pre['rows'])==96 and [x['id']for x in C['all96_cases']]==[x['id']for x in pre['rows']]==m['all96_case_ids'],'exact all96 source context order')
    return m,pointer

if __name__=='__main__':raise SystemExit('DRAFT gate; execute only the later jointly-authorized source entry')
