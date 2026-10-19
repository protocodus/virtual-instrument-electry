"""UNEXECUTED serial source mechanics/reference entry; no rendering authority."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
from fractions import Fraction
import importlib.util,json,sys,datetime
from gate_v1 import require_review,require,sha,P,R

METRICS=('full_state','pick_displacement_m','pick_velocity_m_per_s','linear_pickup_tap','linear_pickup_tap_rate_s_inv')

def main():
    manifest,pointer=require_review()
    output=P/'run-v1';require(not output.exists(),'source run already attempted; no retry/mixing')
    output.mkdir();(output/'native').mkdir()
    # Numerical imports occur only after exact joint authorization and all
    # source/dependency/environment bindings. No state is constructed yet.
    import numpy as np
    import scipy
    from scipy import signal
    from scipy.integrate import solve_ivp
    from scipy.special import roots_legendre
    for name,module in [('numpy',np),('scipy',scipy)]:
        fact=manifest['packages'][name]
        require(module.__version__==fact['version'],'pinned package version '+name)
        require(str(Path(module.__file__).resolve().parent)==fact['root'],'pinned package root '+name)
    api_path=R/manifest['qualified_analytic_api']['path']
    require(sha(api_path)==manifest['qualified_analytic_api']['sha256'],'qualified analytic API identity')
    spec=importlib.util.spec_from_file_location('passive_source_qualified_analytic_api',api_path)
    api=importlib.util.module_from_spec(spec);sys.modules[spec.name]=api;spec.loader.exec_module(api)
    from storage_v3 import StudyBudget,CaseArchive
    from preflight_v1 import run as input_preflight
    from run_model_v3 import run as model_run
    from run_reference_v1 import run as reference_run
    from trajectory_v1 import Trajectory,compare
    import native_adapter_v1 as native
    budget=None;study=None;attempt={'stage':'shared administrative evidence reserve'};rows=[];case_ids=manifest['all96_case_ids']
    started=datetime.datetime.now(datetime.timezone.utc).isoformat()
    try:
        budget=StudyBudget(output)
        study=CaseArchive(output/'study',{'id':'all96-source-mechanics'},
            {'manifest_sha256':sha(P/'implementation-manifest-v1.json'),'pointer':pointer},np,budget)
        attempt={'stage':'all96 input/scaling/path/coefficient preflight'}
        cases,preflight_rows=input_preflight(manifest,np,signal,
            lambda name,obj:study.save_record(name,obj),lambda name,**arr:study.save_arrays(name,**arr))
        require([c['id']for c in cases]==case_ids,'all96 preflight order')
        # The preflight receipt with ALL absolute tolerances is closed on disk
        # before the first native force or physical integration call.
        native.start();lam=Fraction(*manifest['fixed_native_physical_lambda_pair'])
        for c,pre in zip(cases,preflight_rows):
            cid=c['id'];root=output/cid.replace('/','--');root.mkdir()
            bind={'id':cid,'preflight':pre,'source_parameter_sha256':c['parameter_sha256'],
                'implementation_manifest_sha256':sha(P/'implementation-manifest-v1.json'),
                'coefficient_qualification':manifest['coefficient_qualification'],
                'damped_unit_qualification':manifest['damped_unit_qualification']}
            attempt={'stage':'model96','id':cid}
            m96,d96=model_run(c,96000,root/'model96',bind,np,signal,native.solve,lam,budget,api)
            attempt={'stage':'model192','id':cid}
            m192,d192=model_run(c,192000,root/'model192',bind,np,signal,native.solve,lam,budget,api)
            attempt={'stage':'fresh first reference','id':cid}
            rf,df=reference_run(c,False,root/'reference-first',bind,np,signal,solve_ivp,roots_legendre,budget)
            attempt={'stage':'fresh refined reference','id':cid}
            rr,dr=reference_run(c,True,root/'reference-refined',bind,np,signal,solve_ivp,roots_legendre,budget)
            attempt={'stage':'fixed saved trajectory comparisons','id':cid,'descriptors':[d96,d192,df,dr]}
            t96,t192,tf,tr=[Trajectory(c,d,np,api)for d in (d96,d192,df,dr)]
            refinement=compare(c,tf,tr,np);e96=compare(c,t96,tr,np);e192=compare(c,t192,tr,np)
            # Single physical contact: retirement-presence/order is explicit;
            # its real or grid time is retained and never aligned/snapped.
            same_policy=bool(rf['retired']==rr['retired'] and rf['retired'])
            refgate=bool(rf['own_accuracy_PASS']and rr['own_accuracy_PASS']and same_policy
                and refinement['fixed_metrics']['full_state']<=1e-7)
            admitted=bool(m96['PASS']and m192['PASS']and refgate)
            rate_rows=[]
            for metric in METRICS:
                a=e96['fixed_metrics'][metric];b=e192['fixed_metrics'][metric]
                resolved=bool(a<1e-9 and b<1e-9);improved=bool(b<a)
                rate_rows.append({'id':cid,'metric':metric,'error96':a,'error192':b,
                    'admitted':admitted,'both_below_existing_1e_minus9':resolved,
                    'strict_improvement':improved,'numeric_improve_or_resolved':bool(resolved or improved),
                    'qualified_rate_PASS':bool(admitted and(resolved or improved))})
            row={'id':cid,'scope':c['case']['scope'],'old_key':c['case']['old_key'],
                'model96':m96,'model192':m192,'reference_first':rf,'reference_refined':rr,
                'same_retirement_presence_and_single_contact_order':same_policy,
                'reference_refinement':refinement,'reference_gate_PASS':refgate,
                'model96_vs_refined':e96,'model192_vs_refined':e192,'rate_rows':rate_rows,
                'all5_rate_rows_PASS':all(x['qualified_rate_PASS']for x in rate_rows),
                'no_audio_field_or_tone_authority':True}
            row['PASS']=bool(admitted and row['all5_rate_rows_PASS'])
            study.save_record('case-'+cid.replace('/','--'),row);rows.append(row)
        attempt={'stage':'native transport close after all96 source contexts'}
        native_end=native.finish()
        for path in (output/'native').iterdir():budget.reserve(path.stat().st_size,output)
        require(len(rows)==96 and sum(len(r['rate_rows'])for r in rows)==480,'complete fixed source coverage')
        aggregate={'schema':'electry-passive-source-mechanics-completion/v1','candidate':'E / passive-source-v1',
            'started_utc':started,'completed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'all96_completed':True,'rows':rows,'fixed_rate_rows':480,'native_transport_completion':native_end,
            'all96_mechanics_selfreported_PASS':bool(native_end['PASS']and all(r['PASS']for r in rows)),
            'independent_review_pending':True,'field_PCM_tone_and_production_authorized':False,
            'no_reference_retiming_source_gain_or_gate_change':True,
            'emergency_status_is_dormant_administrative_marker_not_completion_or_scientific_gate':True}
        study.save_record('aggregate-v1',aggregate);study.finish(True,{'complete':True,
            'all96_mechanics_selfreported_PASS':aggregate['all96_mechanics_selfreported_PASS'],
            'independent_review_pending':True,'field_PCM_authorized':False})
        budget.finish_success()
        return aggregate
    except BaseException as error:
        close_error=None
        try:native.abort_close()
        except BaseException as failure:close_error=repr(failure)
        if study is not None:
            study.abort(error,attempted={**attempt,'completed_case_ids':[x['id']for x in rows],
                'remaining_case_ids_unavailable':[x for x in case_ids if x not in {r['id']for r in rows}],
                'native_close_error':close_error,'no_retry_restart_shortened_history_or_validity_claim':True})
        raise

if __name__=='__main__':main()
