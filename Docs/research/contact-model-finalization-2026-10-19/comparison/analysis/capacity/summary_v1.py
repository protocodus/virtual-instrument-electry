"""UNEXECUTED fixed-mask summaries retaining unavailable members, no pruning."""
if not __debug__:raise RuntimeError('Optimized Python is forbidden')
import math,types
from helpers_v1 import METRICS


def neck_summary(rows,original):
    """Call exact old summary code per predetermined metric when ALL are known.

    A missing source-admitted row disqualifies that aggregate; it never changes
    the denominator to the remaining favourable rows. Nonadmitted rows remain.
    """
    result={}
    for metric in METRICS:
        intended=[r for r in rows if r['source_admitted'][metric]]
        missing=[r['id']for r in intended if any(not isinstance(r.get(section,{}).get(metric),(float,int))or not math.isfinite(r[section][metric])for section in('source','model','errors'))]
        if missing:
            result[metric]={'statistics_available':False,'original_source_admitted_n':len(intended),'admitted_ids':[r['id']for r in intended],'unavailable_ids':missing,'no_changed_denominator_or_subset_statistic':True}
        else:
            # Unchanged function bytecode, with ALL fixed to this predeclared
            # metric. No data-selected frequency/window/statistic is introduced.
            namespace=dict(original.__globals__);namespace['ALL']=(metric,)
            f=types.FunctionType(original.__code__,namespace)
            result[metric]={**f(rows)[metric],'statistics_available':True,'original_source_admitted_n':len(intended),'unavailable_ids':[]}
    return result


def comparison(baseline,candidate,dist):
    result={}
    for metric in METRICS:
        a=[r for r in baseline if r['source_admitted'][metric]];b=[r for r in candidate if r['source_admitted'][metric]]
        if [r['id']for r in a]!=[r['id']for r in b]:raise RuntimeError('original source denominator changed')
        missing=[y['id']for x,y in zip(a,b)if not all(isinstance(z,(int,float))and math.isfinite(z)for z in(x.get('errors',{}).get(metric),y.get('errors',{}).get(metric)))]
        item={'n':len(a),'source_admitted_ids':[r['id']for r in a],'unavailable_ids':missing,'statistics_available':not missing}
        if not missing:
            change=[abs(y['errors'][metric])-abs(x['errors'][metric])for x,y in zip(a,b)]
            item.update({'candidate_minus_baseline_absolute_error':dist(change),'improved_worsened_tied':[sum(z<0 for z in change),sum(z>0 for z in change),sum(z==0 for z in change)]})
        result[metric]=item
    return result


def paired_harmonic_changes(baseline,candidate,dist):
    """Paired same-window level movements before any distribution summary.

    Ratios are never explained by subtracting medians from different cohorts.
    Both original ratio cohorts and all raw per-row failures are retained.
    """
    if [r['id']for r in baseline]!=[r['id']for r in candidate]:raise RuntimeError('paired row identity')
    rows=[]
    for a,b in zip(baseline,candidate):
        item={'id':a['id'],'string_index':a['string_index'],'fret':a['fret'],'slot':a['slot'],'offset_s':a['offset_s'],'ratios':{}}
        for n in(2,3):
            ratio=f'H{n}_H1_db';keys=('absolute_H1',f'absolute_H{n}',ratio)
            available=all(isinstance(row['errors'].get(key),(int,float))and math.isfinite(row['errors'][key])for row in(a,b)for key in keys)
            q={'original_source_admitted':a['source_admitted'][ratio],'available':available}
            if available:
                h1=b['errors']['absolute_H1']-a['errors']['absolute_H1'];hn=b['errors'][f'absolute_H{n}']-a['errors'][f'absolute_H{n}'];rr=b['errors'][ratio]-a['errors'][ratio]
                q.update({'candidate_minus_baseline_H1_level_db':h1,f'candidate_minus_baseline_H{n}_level_db':hn,'candidate_minus_baseline_ratio_db':rr,'paired_level_change_minus_ratio_change_closure_db':hn-h1-rr})
            item['ratios'][ratio]=q
        rows.append(item)
    summaries={}
    for n in(2,3):
        key=f'H{n}_H1_db';intended=[r for r in rows if r['ratios'][key]['original_source_admitted']];missing=[r['id']for r in intended if not r['ratios'][key]['available']]
        summary={'original_source_admitted_n':len(intended),'ids':[r['id']for r in intended],'unavailable_ids':missing,'statistics_available':not missing}
        if not missing:
            for metric in('candidate_minus_baseline_H1_level_db',f'candidate_minus_baseline_H{n}_level_db','candidate_minus_baseline_ratio_db','paired_level_change_minus_ratio_change_closure_db'):
                summary[metric]=dist([r['ratios'][key][metric]for r in intended])
        summaries[key]=summary
    return {'all_paired_rows':rows,'fixed_original_ratio_cohort_summaries':summaries,'no_subtraction_of_level_medians':True}
