"""UNEXECUTED bounded case path. No entry point permits execution yet."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
import math
from mechanics_v1 import attach,step,free_step,energy,state_hash
from source_inputs_v1 import K,PHYSICAL_RATE,require
from storage_v1 import CaseArchive,TailStream

def run_case(case,rate,archive_root,bindings,np,signal,native_solve,physical_lambda_bound,budget):
    """A future authenticated caller owns all dependencies and fresh gates.

    Both numeric rates use the same96k physical support knots and exact source
    crop, never a new release kernel resampled by normalized control time.
    """
    require(rate in (96000,192000),'fixed prospective refinement rates')
    factor=rate//PHYSICAL_RATE;last=(case['calls96']-1)*factor
    entry=case['contact_frame96']*factor;h=1/rate
    q=np.zeros((2,len(case['n'])));v=np.zeros_like(q)
    evidence=CaseArchive(archive_root,case,bindings,np,budget)
    # Complete physical pickup node, including loading; no held-field shortcut.
    primary_taps=np.zeros(case['calls96']) if rate==96000 else None
    W=0.;D=np.zeros(3);max_numerator=0.;max_abs_work=0.;all_solver=True
    path,contact=attach(case,q,v,entry/rate,np,signal)
    evidence.active_sample(entry,q,v,{'event':'unstrained_attach',**contact})
    tail=TailStream(evidence);tail_started=False;intervals=0
    try:
        for k in range(entry,last):
            was_active=contact['active']
            if was_active:
                q1,v1,work,di,record=step(case,path,contact,q,v,k/rate,(k+1)/rate,h,np,native_solve,physical_lambda_bound)
                all_solver=all_solver and bool(record['native_status']['success'])
            else:
                q1,v1,di=free_step(case,q,v,h,np);work=0.;record={}
            W+=work;D+=di;intervals+=1
            E=energy(case,q1,v1,np);tap=float(np.sum(q1*case['observer']))
            if primary_taps is not None:primary_taps[k+1]=tap
            if contact['active']:
                z=path.value((k+1)/rate,'right')[0]
                U=.5*K*max(z-float(np.sum(case['B']*q1)),0.)**2
            else:U=0.
            numerator=E+U-W+float(D[0]);max_numerator=max(max_numerator,abs(numerator));max_abs_work=max(max_abs_work,abs(W))
            if was_active:
                record.update({'time_s':(k+1)/rate,'physical_tap':tap,'string_energy_J':E,
                    'external_work_J':W,'cumulative_D_Gramian_16_32_J':D.tolist(),
                    'ledger_numerator_J':numerator,'interval_D_Gramian_16_32_J':di.tolist()})
                evidence.active_sample(k+1,q1,v1,record)
            if was_active and not contact['active']:
                evidence.tail_checkpoint(k+1,q1,v1,W,D,contact['retirement']);tail_started=True
            elif tail_started:
                tail.append(k+1,q1,v1,tap,E,W,D,numerator)
            q,v=q1,v1
        tail.flush()
        scale=max(max_abs_work,1e-30) # Actual initial stored energy is0.
        retired=not contact['active']
        summary={'case_id':case['id'],'numerical_rate_hz':rate,'physical_path_rate_hz':96000,
            'source_callback_frames96':case['calls96'],'complete_fixed_crop':True,
            'source_contact_frame96':case['contact_frame96'],'source_release_frame96':case['release_frame96'],
            'all_solver_statuses_PASS':all_solver,'retired_zero_storage':retired,
            'retirement':contact.get('retirement'),'active_at_fixed_crop_end':contact['active'],
            'initial_stored_energy_J':0.,'state_error_normalization_energy_J':.5*case['Fq_N']*case['D_m'],
            'Fq_N':case['Fq_N'],'D_m':case['D_m'],'external_work_J':W,
            'final_D_Gramian_16_32_J':D.tolist(),'work_normalization_J':scale,
            'maximum_absolute_work_ledger_normalized':max_numerator/scale,
            'original5e_minus10_workrail_PASS':max_numerator/scale<=5e-10,
            'unapplied_original_kernel_mass_at_crop':path.unapplied_kernel_mass,
            'final_qv_sha256':state_hash(q,v),'intervals':intervals,
            'no_field_or_PCM_authority_from_this_function':True}
        summary['main_mechanics_gate_PARTIAL']=bool(all_solver and retired and summary['original5e_minus10_workrail_PASS'])
        # This is deliberately not all96/mechanics authorization: independent
        # continuous first/refined, exact certificates/construction, refinement
        # and event gates must still qualify before field use or anyPCM.
        if primary_taps is not None:
            path_out=Path(archive_root)/'complete_physical_tap.f64'
            evidence._capacity(primary_taps.nbytes)
            with path_out.open('xb')as f:primary_taps.astype('<f8').tofile(f)
        evidence.finish(True,summary)
        return summary
    except BaseException as error:
        evidence.finish(False,{'case_id':case['id'],'failure':repr(error),
            'source_failure_evidence':getattr(error,'source_evidence',None),
            'completed_intervals':intervals,'qv_last_accepted_sha256':state_hash(q,v),
            'complete_all96_or_candidate_qualification':False,'no_retry_or_tail_rescue':True})
        raise

if __name__=='__main__':raise SystemExit('DRAFT only; independent mechanics/reference gates and fresh root authorization required')
