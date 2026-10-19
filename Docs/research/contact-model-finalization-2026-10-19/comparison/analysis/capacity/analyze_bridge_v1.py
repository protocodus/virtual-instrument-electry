"""UNEXECUTED all64 Bridge windows/all16 ORIGINAL complete prefixes A/C/D/E."""
if not __debug__:raise RuntimeError('Optimized Python is forbidden')
from pathlib import Path
import math
from helpers_v1 import R,LETTERS,sha
from context_v1 import bridge_common
from background_v1 import candidate_background,unavailable_upper
P=Path(__file__).resolve().parent


def finite_record(x):
    if isinstance(x,dict):return all(finite_record(v)for v in x.values())
    if isinstance(x,(list,tuple)):return all(finite_record(v)for v in x)
    return not isinstance(x,float)or math.isfinite(x)


def baseline_primary_summary(rows,cm,np):
    """Same original cohorts/statistics, with unavailable members retained.

    Mirrors the frozen helper's arithmetic on complete cohorts. A missing
    member suppresses that letter/metric and dependent pairwise statistic;
    it never selects a smaller cohort or discards the assembled rows.
    """
    summary=[]
    for pid in(1,2):
        for key in cm.KEYS:
            chosen=[r for r in rows if r['player']==pid and r['profile']==key and r['primary_body']and not r['source_censored_hold_reasons']]
            metrics={}
            for metric in chosen[0]['letters']['A']['errors']:
                result={}
                for label in('A','C','D'):
                    values=[r['letters'][label]['errors'][metric]for r in chosen]
                    missing=[r['window_id']for r,v in zip(chosen,values)if not isinstance(v,(int,float))or not math.isfinite(v)]
                    result.update({label+'_signed_errors':values,label+'_original_primary_n':len(chosen),label+'_unavailable_ids':missing,label+'_statistics_available':not missing})
                    if not missing:
                        errors=np.asarray(values);result[label+'_median_signed_error']=float(np.median(errors));result[label+'_median_absolute_error']=float(np.median(abs(errors)))
                # This is the only pair present for the original A/C/D letters
                # in cm.primary_summary; retain exactly that arithmetic.
                available=result['A_statistics_available']and result['C_statistics_available']
                result['C_vs_A_statistics_available']=available
                if available:
                    aa=abs(np.asarray(result['A_signed_errors']));cc=abs(np.asarray(result['C_signed_errors']))
                    result['C_vs_A_improved_worsened_tied']=[int(np.sum(cc<aa)),int(np.sum(cc>aa)),int(np.sum(cc==aa))]
                metrics[metric]=result
            summary.append({'player':pid,'profile':key,'primary_window_ids':[r['window_id']for r in chosen],'metrics':metrics})
    return summary


def run(context,np):
    cm=bridge_common();mask,_=cm.frozen_inputs();prior=context['D_bridge'];oldmap={(r['player'],r['profile'],r['window_id']):r for r in prior['all64_body_absolute_windows']}
    audio={key:context['audio']['bridge/'+key]for key in cm.KEYS};backgrounds={};background_facts=[]
    for key in cm.KEYS:
        prefix,facts=candidate_background(audio[key]['E'],context['metadata']['bridge/'+key]['E'],np)
        bg=None
        if prefix is not None:
            try:
                bg=cm.upper.bands(prefix,48000,'bh')
                if not all(math.isfinite(v)and v>=0 for v in bg.values()):raise ValueError('nonfinite/negative measured background band')
            except Exception as error:facts.update({'available':False,'unavailable_reason':repr(error)});bg=None
        backgrounds[key]=bg;background_facts.append({'id':'bridge/'+key,**facts,'measured_BH_background_bands':bg})
    rows=[];prefixes=[];matches=[];oldchecks=[];failures=[]
    for fact in mask['references']:
        pid=fact['player'];reference,rate=cm.body.load_crop(R/fact['path'],fact['crop_start_frame'],96000)
        if rate!=48000 or sha(R/fact['path'])!=fact['sha256']:raise RuntimeError('original Bridge reference identity')
        gain={'player':pid,**cm.gains(reference,audio,LETTERS)};oldgain=next(x for x in prior['whole2s_matching']if x['player']==pid)
        for key,value in gain['raw'].items():
            if not key.endswith('/E')and(value!=oldgain['raw'][key]or gain['base_whole2s_RMS_gains'][key]!=oldgain['base_whole2s_RMS_gains'][key]):raise RuntimeError('old whole2s raw/base gain identity')
        if len(gain['raw'])!=17:raise RuntimeError('same17 whole2s gains perplayer')
        matches.append(gain)
        for key in cm.KEYS:
            byid={}
            for frozen in(x for x in mask['all64_source_A_rows']if x['player']==pid and x['profile']==key):
                a,b=frozen['crop_relative_frames']
                if cm.body.observe(reference[a:b])!=frozen['source_metrics']:raise RuntimeError('original source window observer identity')
                sourcebands=cm.otherbands(reference[a:b]);previous=oldmap[(pid,key,frozen['window_id'])]
                row={'player':pid,'profile':key,'window_id':frozen['window_id'],'source_frames':frozen['source_frames'],'crop_relative_frames':[a,b],
                     'center_offset_s':frozen['center_offset_s'],'primary_body':frozen['center_offset_s']in(.1,.2,.35),
                     'source_censored_hold_reasons':frozen['source_censored_hold_reasons'],'source_metrics':frozen['source_metrics'],'source_low_bands':sourcebands,
                     'source_upper_guard_frozen':frozen['source_upper_guard_original'],'letters':{}}
                for label in LETTERS:
                    try:
                        result,src=cm.observe_letter(audio[key][label],frozen,sourcebands,gain['final_gains']['Reference'],gain['final_gains'][key+'/'+label])
                        row['source_matched_levels_dbfs']=src
                        if label=='E':
                            facts=next(x for x in background_facts if x['id']=='bridge/'+key)
                            if backgrounds[key]is None:result['upper_guard']=unavailable_upper(result['upper_guard'],facts)
                            else:
                                result['upper_guard']=cm.upper.observe(audio[key][label][a:b],48000,440*2**((40-69)/12),1,backgrounds[key])['upper_bands']
                                for guard in result['upper_guard'].values():guard['candidate_background_available']=True
                            result['candidate_only_background_policy']='actual full pre-Contact PCM prefix; old default35ms guard replaced only for E'
                        if not finite_record(result):raise ValueError('nonfinite observation; no epsilon or guard repair')
                        result['numeric_observation_available']=True;row['letters'][label]=result
                        result['paired_level_ratio_error_closure_db']={f'h{n}_h1':result['errors'][f'matched_h{n}_db']-result['errors']['matched_h1_db']-result['errors'][f'h{n}_h1_db']for n in(2,3)}
                        if label!='E':
                            prev=previous['letters'][label];same=all(result[m]==prev[m]for m in('observer','low_bands','upper_guard'))
                            delta=max(abs(result['errors'][m]-prev['errors'][m])for m in result['errors'])
                            oldchecks.append({'player':pid,'profile':key,'window_id':frozen['window_id'],'letter':label,'same_raw_observer_and_guard':same,'error_delta_max':delta,'PASS':same and delta<=1e-12})
                    except Exception as error:
                        failures.append({'player':pid,'profile':key,'window_id':frozen['window_id'],'letter':label,'failure':repr(error)})
                        row['letters'][label]={'numeric_observation_available':False,'failure':repr(error),'errors':{m:None for m in previous['letters']['A']['errors']},'upper_guard':{band:{'observable':False,'unavailable_reason':'numeric observation failed'}for band in cm.upper.BANDS}}
                row['paired_E_harmonic_changes_vs_baselines']={}
                for baseline in('A','C','D'):
                    aerrors=row['letters'][baseline]['errors'];eerrors=row['letters']['E']['errors'];changes={}
                    for n in(2,3):
                        keys=('matched_h1_db',f'matched_h{n}_db',f'h{n}_h1_db');available=all(isinstance(values.get(k),(int,float))and math.isfinite(values[k])for values in(aerrors,eerrors)for k in keys)
                        item={'available':available}
                        if available:
                            d1=eerrors['matched_h1_db']-aerrors['matched_h1_db'];dn=eerrors[f'matched_h{n}_db']-aerrors[f'matched_h{n}_db'];dr=eerrors[f'h{n}_h1_db']-aerrors[f'h{n}_h1_db']
                            item.update({'H1_level_change_db':d1,f'H{n}_level_change_db':dn,'ratio_change_db':dr,'paired_level_minus_ratio_change_closure_db':dn-d1-dr})
                        changes[f'h{n}_h1']=item
                    row['paired_E_harmonic_changes_vs_baselines'][baseline]=changes
                rows.append(row);byid[row['window_id']]=row
            for frozen in(x for x in mask['prefixes']if x['player']==pid and x['profile']==key):
                band=frozen['band'];short=band.removeprefix('upper');selected=[byid[x]for x in frozen['window_ids']]
                failed={label:[r['window_id']for r in selected if not r['letters'][label]['upper_guard'][short]['observable']]for label in LETTERS}
                admitted=frozen['admitted_fixed_source_A_prefix_before_B']
                item={'player':pid,'profile':key,'band':band,'frozen_prefix':frozen,'window_ids_unchanged':frozen['window_ids'],'prefrozen_source_A_admitted':admitted,
                      'guard_failed_ids_by_letter':failed,'qualified_fixed_prefix_by_letter':{label:bool(admitted and not failed[label])for label in LETTERS},
                      'failure_is_retained_not_prefix_shortened':True,'slopes':None}
                if frozen['original_prefix']['original_source_admitted']:
                    sourcevalues=[r['source_metrics']['bands'][band]for r in selected]
                    if sourcevalues!=frozen['source_values_Hann_original']:raise RuntimeError('source prefix changed')
                    item['slopes']={'Reference':cm.slopes(selected,sourcevalues)}
                    for label in LETTERS:
                        try:
                            values=[r['letters'][label]['observer']['bands'][band]for r in selected];slope=cm.slopes(selected,values)
                            if not finite_record(slope):raise ValueError('nonfinite fixed-prefix OLS')
                            item['slopes'][label]=slope
                        except Exception as error:
                            item['slopes'][label]=None;item.setdefault('slope_unavailable',{})[label]=repr(error);item['qualified_fixed_prefix_by_letter'][label]=False
                    item['signed_slope_errors_db_s']={label:None if item['slopes'][label]is None else item['slopes'][label]['OLS_slope_db_s']-item['slopes']['Reference']['OLS_slope_db_s']for label in LETTERS}
                    for label in('A','C','D'):
                        e=item['signed_slope_errors_db_s'];item['absolute_slope_error_change_E_minus_'+label]=None if e['E']is None or e[label]is None else abs(e['E'])-abs(e[label])
                else:
                    if pid!=2 or band!='upper4000_8000':raise RuntimeError('unexpected original unavailable prefix')
                    item['unqualified_reason']='OriginalP2source4-8kprefixnotadmitted; no new slope.'
                prefixes.append(item)
    if len(rows)!=64 or len(prefixes)!=16:raise RuntimeError('all64windows/all16prefixes retained')
    # Complete baseline cohorts retain the frozen summary arithmetic; failures
    # suppress affected statistics while preserving every original member.
    # Candidate summaries retain
    # whole original primary cohorts; a missing value suppresses that statistic,
    # never changes the denominator to a favourable subset.
    summaries=baseline_primary_summary(rows,cm,np)
    for group in summaries:
        selected=[r for r in rows if r['player']==group['player']and r['profile']==group['profile']and r['primary_body']and not r['source_censored_hold_reasons']]
        for metric,stats in group['metrics'].items():
            values=[r['letters']['E']['errors'][metric]for r in selected];missing=[r['window_id']for r,v in zip(selected,values)if not isinstance(v,(int,float))or not math.isfinite(v)]
            stats.update({'E_original_primary_n':len(selected),'E_unavailable_ids':missing,'E_signed_errors':values,'E_statistics_available':not missing})
            for label in('A','C','D'):stats['E_vs_'+label+'_statistics_available']=bool(not missing and stats[label+'_statistics_available'])
            if not missing:
                arr=np.asarray(values);stats.update({'E_median_signed_error':float(np.median(arr)),'E_median_absolute_error':float(np.median(abs(arr)))})
                for label in('A','C','D'):
                    if stats['E_vs_'+label+'_statistics_available']:
                        change=abs(arr)-abs(np.asarray(stats[label+'_signed_errors']));stats['E_vs_'+label+'_improved_worsened_tied']=[int(sum(change<0)),int(sum(change>0)),int(sum(change==0))]
    return {'schema':'electry-passive-source-Bridge-analysis/draft-v1','letters':list(LETTERS),'source_A_mask_sha256':sha(R/'build-damped-field-bridge-upper/source-A-mask-before-B.json'),
            'all64_body_absolute_windows':rows,'all16_original_upper_prefix_cases':prefixes,'primary_body_summary':summaries,'whole2s_matching':matches,
            'candidate_preContact_backgrounds':background_facts,'all192_old_outcome_checks':oldchecks,'all_old_outcomes_reproduced':len(oldchecks)==192 and all(x['PASS']for x in oldchecks),
            'failures':failures,'no_prefix_ids_source_admissions_or_observers_changed':True,'P2_4_8k_slope_created':False,
            'qualification':'Descriptive isolated-source/nativepickup diagnostic, not continuous performance, calibratedDI/alias/RT or95percent qualification.'}

if __name__=='__main__':raise SystemExit('DRAFT ONLY; no PCM/metrics authorization exists')
