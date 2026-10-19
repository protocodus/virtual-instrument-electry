"""UNEXECUTED analysis-only gate; no numerical imports or source/PCM scans."""
if not __debug__:raise RuntimeError('Optimized Python is forbidden')
from pathlib import Path
import hashlib,json,os,platform,subprocess,sys
P=Path(__file__).resolve().parent;R=P.parent

def sha(p):
    h=hashlib.sha256()
    with Path(p).open('rb')as s:
        for block in iter(lambda:s.read(1024*1024),b''):h.update(block)
    return h.hexdigest()

def require(ok,why):
    if not ok:raise RuntimeError(why)

def require_review(authpath,expected):
    require(__debug__,'Optimized Python forbidden')
    require(sha(authpath)==expected,'analysis authorization changed')
    auth=json.loads(Path(authpath).read_text());mp=P/'implementation-manifest-v1.json';m=json.loads(mp.read_text());mh=sha(mp);ph=sha(P/'protocol-draft-v1.json')
    require(m['protocol_sha256']==ph,'selected protocol changed')
    require(auth['passive_source_tone_analysis_authorized']is True and auth['implementation_manifest_sha256']==mh and auth['protocol_sha256']==ph,'fresh analysis-only authorization')
    for role in('root','peer'):
        f=auth[role+'_review'];q=R/f['path'];require(sha(q)==f['sha256'],'review changed');review=json.loads(q.read_text())
        require(review['role']==role and review['static_clear']is True and review['passive_source_tone_analysis_authorized']is True and review['implementation_manifest_sha256']==mh and review['protocol_sha256']==ph,'exact root/peer analysis review')
    # This future root seal is supplied only after independent all96 source
    # mechanics and immutable-pickup renders. No field/state is read here.
    f=auth['candidate_qualification'];q=R/f['path'];require(sha(q)==f['sha256'],'E prerequisite changed');qr=json.loads(q.read_text())
    require(qr['PASS']is True and qr['all96_mechanics_and_render_qualified']is True,'all96 E prerequisites unavailable')
    require(qr['render_receipt']==auth['candidate_render_receipt'],'E prerequisite/render identity')
    for name in m['exclusive_output_paths']:require(not(P/name).exists(),'existing analysis output '+name)
    for section,base in(('artifacts',P),('inputs',R),('sample_bindings',R),('environment_files',Path('/'))):
        for name,fact in m[section].items():require((base/name).stat().st_size==fact['bytes']and sha(base/name)==fact['sha256'],'binding '+name)
    require(str(Path(sys.executable).resolve())==m['python_real_executable']and platform.system()=='Darwin'and platform.machine()=='arm64','fixed interpreter/platform')
    for name,value in m['inherited_environment'].items():require(os.environ.get(name)==value,'environment '+name)
    for name,h in m['Source24_sha256'].items():
        require(sha(R/name)==h,'current Source changed');require(hashlib.sha256(subprocess.check_output(['git','show',m['immutable_source_commit']+':'+name],cwd=R)).hexdigest()==h,'published Source changed')
    require(not subprocess.check_output(['git','diff',m['immutable_source_commit'],'--','Source','Tests','CMakeLists.txt'],cwd=R),'Source/Tests/CMake changed')
    return m,auth
