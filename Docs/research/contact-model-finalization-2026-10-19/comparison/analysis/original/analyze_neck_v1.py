"""UNEXECUTED: all frozen138 Neck windows x2profiles x A/C/D/E, no refit."""
if not __debug__:raise RuntimeError('Optimized Python is forbidden')
from pathlib import Path
import json,math,wave
from helpers_v1 import R,LETTERS,METRICS,POWERS,neck_functions,sha
from background_v1 import candidate_background
from summary_v1 import neck_summary,comparison,paired_harmonic_changes
P=Path(__file__).resolve().parent


def model_admissions(observer):
    good=[p['quality']['guard_pass']and not p['peak_at_search_boundary']for p in observer['partials']]
    return {'absolute_H1':good[0],'absolute_H2':good[1],'absolute_H3':good[2],
            'H2_H1_db':good[0]and good[1],'H3_H1_db':good[0]and good[2],
            'H1_share123':all(good),'body80_250_power':observer['bands']['body80_250']['guard_pass'],
            'lowmid250_1000_power':observer['bands']['lowmid250_1000']['guard_pass']}


def run(context,np,blackmanharris):
    ns=neck_functions(np,blackmanharris);observe,db,dist,oldsummary=[ns[k]for k in('observation','db','dist','summary')]
    oldplan=json.loads((R/'build-damped-field-neck-body/protocol.json').read_text())
    cp=json.loads((R/'build-inherited-field-C/protocol.json').read_text())
    mask=json.loads((R/'build-p2-neck-late-body/source-mask-frozen.json').read_text())
    notes=cp['controls_and_events']['neck']['all_notes'];profiles=cp['controls_and_events']['neck']['exact_profiles']
    reference={}
    with wave.open(str(R/oldplan['source_audio']['path']),'rb')as source:
        if(source.getframerate(),source.getnchannels(),source.getsampwidth())!=(48000,2,3):raise RuntimeError('original Neck encoding')
        for note in notes:
            source.setpos(note['MIDI_on_source_frame']);raw=source.readframes(96000)
            if len(raw)!=576000:raise RuntimeError('original2s source crop incomplete')
            by=np.frombuffer(raw,np.uint8).reshape(-1,2,3);q=by[:,0,0].astype(np.int32)|(by[:,0,1].astype(np.int32)<<8)|(by[:,0,2].astype(np.int32)<<16)
            reference[note['slot']]=((q^0x800000)-0x800000).astype(float)/8388608
    clips={};tracks={'Reference':np.concatenate([reference[n['slot']]for n in notes])};backgrounds={};background_facts=[]
    for profile in profiles:
        for label in LETTERS:
            key=profile['id']+'-'+label;clips[key]={}
            for note in notes:
                caseid=f"neck/{profile['id']}-slot{note['slot']:03d}"
                y=context['audio'][caseid][label];clips[key][note['slot']]=y
                if label=='E':
                    prefix,facts=candidate_background(y,context['metadata'][caseid]['E'],np)
                    backgrounds[caseid]=prefix;background_facts.append({'id':caseid,**facts})
            tracks[key]=np.concatenate([clips[key][n['slot']]for n in notes])
    if len(tracks)!=9 or any(len(y)!=4416000 for y in tracks.values()):raise RuntimeError('unchanged46x2s gain cohorts')
    rms={k:float(np.sqrt(np.mean(y*y)))for k,y in tracks.items()};peaks={k:float(max(abs(y)))for k,y in tracks.items()}
    if not all(math.isfinite(v)and v>0 for v in rms.values()):raise RuntimeError('whole-track RMS unavailable, no gain floor')
    base={k:10**(-24/20)/v for k,v in rms.items()};cap=min(1.,.94/max(peaks[k]*base[k]for k in tracks));gain={k:v*cap for k,v in base.items()}
    prior=context['D_neck'];oldchecks=[]
    for key in base:
        if not key.endswith('-E')and(base[key]!=prior['matching']['base_gains'][key]or rms[key]!=prior['matching']['rms_raw'][key]):raise RuntimeError('old whole92s raw/base gain identity')
    results=[];failures=[]
    for key,slots in clips.items():
        label=key.rsplit('-',1)[1];profile=key.rsplit('-',1)[0];rows=[]
        for frozen in mask['rows']:
            note=next(n for n in notes if n['slot']==frozen['slot']);a,b=[f-note['MIDI_on_source_frame']for f in frozen['source_frames']]
            sourcevalues=dict(frozen['source_metrics']['metrics'])
            for metric in POWERS:sourcevalues[metric]=db(sourcevalues[metric]*gain['Reference']**2)
            row={'id':frozen['id'],'slot':frozen['slot'],'string_index':frozen['string_index'],'fret':frozen['fret'],'offset_s':frozen['center_offset_s'],
                 'local_frames':[a,b],'original_source_frames':frozen['source_frames'],'source_admitted':frozen['metric_admitted'],
                 'original_source_guard_facts':frozen,'source':sourcevalues,'model':{m:None for m in METRICS},'errors':{m:None for m in METRICS},'numeric_observation_available':False}
            try:
                caseid=f"neck/{profile}-slot{frozen['slot']:03d}";bg=backgrounds[caseid]if label=='E'else None
                obs=observe(slots[frozen['slot']][a:b],48000,440*2**((frozen['sounding_midi']-69)/12),bg)
                values=dict(obs['metrics'])
                for metric in POWERS:values[metric]=db(values[metric]*gain[key]**2)
                if not all(math.isfinite(x)for x in values.values()):raise ValueError('nonfinite metric; no epsilon/floor change')
                row.update({'model':values,'errors':{m:values[m]-sourcevalues[m]for m in METRICS},'model_observer_descriptive_no_remask':obs,'numeric_observation_available':True})
                # Paired same-window harmonic levels, never median subtraction.
                row['paired_level_ratio_error_closure_db']={f'H{n}_H1':row['errors'][f'absolute_H{n}']-row['errors']['absolute_H1']-row['errors'][f'H{n}_H1_db']for n in(2,3)}
                if label=='E':
                    own=model_admissions(obs);row['candidate_only_model_admitted']=own
                    row['candidate_source_and_model_admitted']={m:bool(own[m]and frozen['metric_admitted'][m])for m in METRICS}
                else:
                    previous=next(r for p in prior['profiles_letters']if p['key']==key for r in p['rows']if r['id']==frozen['id'])
                    sameobs=obs==previous['model_observer_descriptive_no_remask'];delta=max(abs(row['errors'][m]-previous['errors'][m])for m in METRICS)
                    oldchecks.append({'key':key,'id':frozen['id'],'old_observer_and_unknown_background_identity':sameobs,'error_delta_max':delta,'PASS':sameobs and delta<1e-10})
            except Exception as error:
                row['failure']=repr(error);failures.append({'key':key,'id':row['id'],'failure':repr(error),'numeric_observation_available':False})
                if label=='E':row.update({'candidate_only_model_admitted':{m:False for m in METRICS},'candidate_source_and_model_admitted':{m:False for m in METRICS}})
            rows.append(row)
        summaries={'all':neck_summary(rows,oldsummary),'per_string':{str(s):neck_summary([r for r in rows if r['string_index']==s],oldsummary)for s in(2,3)},
                   'per_fret':{str(f):neck_summary([r for r in rows if r['fret']==f],oldsummary)for f in range(23)},
                   'per_offset':{str(t):neck_summary([r for r in rows if r['offset_s']==t],oldsummary)for t in(.5,.8,1.1)},
                   'per_note':{str(n):neck_summary([r for r in rows if r['slot']==n],oldsummary)for n in range(1,47)}}
        results.append({'key':key,'rows':rows,'summary':summaries})
    comparisons={}
    for profile in profiles:
        candidate=next(x for x in results if x['key']==profile['id']+'-E');comparisons[profile['id']]={}
        for label in('A','C','D'):
            baseline=next(x for x in results if x['key']==profile['id']+'-'+label)
            comparisons[profile['id']]['E_vs_'+label]=comparison(baseline['rows'],candidate['rows'],dist)
            comparisons[profile['id']]['E_vs_'+label+'_paired_harmonic_levels']=paired_harmonic_changes(baseline['rows'],candidate['rows'],dist)
    return {'schema':'electry-passive-source-Neck-analysis/draft-v1','letters':list(LETTERS),'archived_oldB_context':'original C/D studies remain unchanged; B not in new gain cohort',
            'source_denominators':mask['summary']['metric_denominators'],'source_mask_sha256':sha(R/'build-p2-neck-late-body/source-mask-frozen.json'),
            'matching':{'frames':4416000,'seconds':92,'rms_raw':rms,'peaks_raw':peaks,'base_gains':base,'common_peakcap':cap,'final_gains':gain,'nine_whole_concatenation_gains':True},
            'profiles_letters':results,'all_E_vs_A_C_D':comparisons,'candidate_preContact_backgrounds':background_facts,'all828_old_outcome_checks':oldchecks,
            'all_old_outcomes_reproduced':len(oldchecks)==828 and all(x['PASS']for x in oldchecks),'failures':failures,'all1104_rows_retained':sum(len(x['rows'])for x in results)==1104,
            'primary_statistics_keep_original_source_masks':'Candidate-only model guard status is supplementary; no source denominator/selection changed.',
            'qualification':'Isolated two-second loading/held-note numerical-source diagnostic, no production/repick/attack/DIcalibration or95percent qualification.'}

if __name__=='__main__':raise SystemExit('DRAFT ONLY; no PCM/metrics authorization exists')
