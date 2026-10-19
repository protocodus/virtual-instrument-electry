"""UNEXECUTED: frozen A/C/D+reference inputs and future E render contract.

Only JSON/source metadata are read during preparation. load() is called only
AFTER separate exact analysis authorization; it may then read bound old/E PCM.
"""
if not __debug__:raise RuntimeError('Optimized Python is forbidden')
from pathlib import Path
import importlib.util,json,sys
from helpers_v1 import R,sha,load_f32,LETTERS
P=Path(__file__).resolve().parent


def load(np,authorization):
    dp=R/'build-instant-release-field-D';cp=R/'build-inherited-field-C'
    plan=json.loads((dp/'protocol-v1.json').read_text())
    oldD=json.loads((dp/'D-render-v1.json').read_text());oldC=json.loads((cp/'C-render-receipt.json').read_text())
    fact=authorization['candidate_render_receipt'];ep=R/fact['path']
    if sha(ep)!=fact['sha256']:raise RuntimeError('candidate render receipt changed')
    er=json.loads(ep.read_text())
    if er['all96_rendered_after_blind_gate']is not True or len(er['rows'])!=96:raise RuntimeError('all96 independent mechanics+render gate required')
    by={letter:{x['id']:x for x in receipt['rows']}for letter,receipt in(('C',oldC),('D',oldD),('E',er))}
    expected={x['id']for x in plan['all96_cases']}
    if any(set(v)!=expected for v in by.values()):raise RuntimeError('exact96 C/D/E case coverage')
    roots={'C':cp,'D':dp,'E':ep.parent};audio={};metadata={};records=[]
    for case in plan['all96_cases']:
        key=case['id'];audio[key]={};metadata[key]={}
        aa=case['artifacts']['A_hook'];audio[key]['A']=load_f32(R/aa['path'],aa['sha256'],np)
        oldfact=case['artifacts']['old_geometry_metadata'];oldpath=R/oldfact['path']
        if sha(oldpath)!=oldfact['sha256']:raise RuntimeError('old geometry metadata changed')
        oldmeta=json.loads(oldpath.read_text())
        for label in('C','D','E'):
            rec=by[label][key]
            if rec['scope']!=case['scope']or rec['old_key']!=case['old_key']:raise RuntimeError('case/profile output join')
            root=roots[label];pcm=root/rec['PCM_path'];mp=root/rec['metadata_path']
            audio[key][label]=load_f32(pcm,rec['PCM_sha256'],np)
            if sha(mp)!=rec['metadata_sha256']:raise RuntimeError('model metadata changed')
            m=json.loads(mp.read_text());metadata[key][label]=m
            if {k:v for k,v in m.items()if k not in('rms','peak')}!={k:v for k,v in oldmeta.items()if k not in('rms','peak')}:raise RuntimeError('actual driver geometry/timeline changed')
            records.append({'id':key,'letter':label,'PCM_path':str(pcm.relative_to(R)),'PCM_sha256':rec['PCM_sha256'],'metadata_path':str(mp.relative_to(R)),'metadata_sha256':rec['metadata_sha256']})
    return {'plan':plan,'audio':audio,'metadata':metadata,'records':records,'E_receipt':fact,
            'D_neck':json.loads((dp/'neck-analysis-v1.json').read_text()),
            'D_bridge':json.loads((dp/'bridge-analysis-v1.json').read_text())}


def bridge_common():
    # These exact same immutable modules expose the frozen observer functions.
    # Importing is authorized only by the NEW runner before this call.
    folder=R/'build-inherited-C-bridge-analysis'
    sys.dont_write_bytecode=True
    sys.path.insert(0,str(folder))
    spec=importlib.util.spec_from_file_location('passive_analysis_frozen_bridge_common',folder/'common.py')
    module=importlib.util.module_from_spec(spec);sys.modules[spec.name]=module;spec.loader.exec_module(module)
    return module

if __name__=='__main__':raise SystemExit('DRAFT ONLY; no PCM/metrics authorization exists')
