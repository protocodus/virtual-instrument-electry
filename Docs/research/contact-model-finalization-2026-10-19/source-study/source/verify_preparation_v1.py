"""Hash-only source preparation verifier, arrays/archive STAT ONLY."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
import hashlib,json,sys
P=Path(__file__).resolve().parent;R=P.parent
def sha(q):return hashlib.sha256(Path(q).read_bytes()).hexdigest()
def require(ok,msg):
    if not ok:raise RuntimeError(msg)
def verify():
    m=json.loads((P/'implementation-manifest-v1.json').read_text())
    for section,base in [('artifacts',P),('inputs',R),('environment_files',Path('/'))]:
        for rel,f in m[section].items():
            q=base/rel;require(q.stat().st_size==f['bytes']and sha(q)==f['sha256'],'preparation identity '+rel)
    for rel,f in m['sample_bindings'].items():require((R/rel).stat().st_size==f['bytes'],'STAT-only recorded sample '+rel)
    for rel,h in m['Source24'].items():require(sha(R/rel)==h,'Source24 identity '+rel)
    require(len(m['all96_case_ids'])==96 and len(set(m['all96_case_ids']))==96,'all96 case identities')
    require(sha(P/'protocol-selected-v1.json')==m['protocol_sha256'],'selected protocol')
    require(str(Path(sys.executable).resolve())==m['python_real_executable']and sys.version==m['python_version'],'pinned312 metadata verifier')
    print('PASS',len(m['artifacts']),len(m['inputs']),len(m['sample_bindings']),'STAT-only',len(m['environment_files']),'env',len(m['Source24']),'Source',96,'contexts; NO STATE AUTHORITY')
    return m
if __name__=='__main__':verify()
