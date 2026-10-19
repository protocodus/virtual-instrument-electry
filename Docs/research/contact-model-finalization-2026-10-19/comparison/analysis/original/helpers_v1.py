"""UNEXECUTED bound-function loading; no old module globals or audio on import."""
if not __debug__:raise RuntimeError('Optimized Python is forbidden')
from pathlib import Path
import ast,hashlib,json,math,wave
R=Path(__file__).resolve().parent.parent

RATIOS=('H1_share123','H2_H1_db','H3_H1_db')
POWERS=('absolute_H1','absolute_H2','absolute_H3','body80_250_power','lowmid250_1000_power')
METRICS=RATIOS+POWERS
LETTERS=('A','C','D','E')

def sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb')as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''):h.update(chunk)
    return h.hexdigest()

def neck_functions(np,blackmanharris):
    """Extract the SAME frozen functions; never execute source-loading globals."""
    ns={'np':np,'math':math,'wave':wave,'blackmanharris':blackmanharris,
        'BANDS':{'body80_250':(80,250),'lowmid250_1000':(250,1000)},'ALL':METRICS}
    for source,names in((R/'build-p2-neck-source-intake/bound-observers/evaluate_tone_envelopes.py',('db','spectrum')),
                         (R/'build-p2-neck-late-body/qualify_source.py',('observation',)),
                         (R/'build-damped-field-neck-body/analyze.py',('dist','summary','pcm24'))):
        tree=ast.parse(source.read_text());selected=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name in names]
        if {x.name for x in selected}!=set(names):raise RuntimeError('frozen helper function schema')
        exec(compile(ast.Module(body=selected,type_ignores=[]),str(source),'exec'),ns)
    return ns


def load_f32(path,expected,np):
    if sha(path)!=expected:raise RuntimeError('PCM binding changed '+str(path))
    y=np.fromfile(path,dtype='<f4').astype(float)
    if len(y)!=96000 or not np.isfinite(y).all():raise RuntimeError('finite unchanged2s PCM required')
    return y


def candidate_receipt(repo,contract):
    """Future entry first validates exact auth, every binding and all96 gates."""
    path=repo/contract['candidate_render_receipt']['path']
    if sha(path)!=contract['candidate_render_receipt']['sha256']:raise RuntimeError('E render receipt changed')
    render=json.loads(path.read_text())
    if render['all96_rendered_after_blind_gate']is not True or len(render['rows'])!=96:raise RuntimeError('all96 blind+render gate required')
    rows={x['id']:x for x in render['rows']}
    if len(rows)!=96 or set(rows)!=set(contract['all96_case_ids']):raise RuntimeError('exact96 case coverage')
    return rows

if __name__=='__main__':raise SystemExit('DRAFT ONLY; no analysis authorization exists')
