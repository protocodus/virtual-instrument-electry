"""UNEXECUTED v2: full contact history, prospectively analytic free tail.

V1 remains preserved. No physical operation is invoked on import or by entry.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
from mechanics_v1 import attach,step,energy,state_hash
from source_inputs_v1 import K,PHYSICAL_RATE,require
from storage_v2 import CaseArchive
from analytic_tail_v2 import batches

def save_tail_batch(evidence,batch):
    np=evidence.np
    names=('frames','times_s','physical_tap','string_energy_J','work_J',
        'interval_D_Gramian_16_32_J','cumulative_D_Gramian_16_32_J','ledger_numerator_J','last_q','last_v')
    count=sum(batch[k].nbytes for k in names)+4096;evidence._capacity(count)
    path=evidence.root/f'analytic-free-tail-{int(batch["frames"][0]):08d}.npz'
    with path.open('xb')as f:np.savez_compressed(f,
        **{k:batch[k]for k in names},
        ordered_frame_q_then_v_sha256=batch['ordered_frame_q_then_v_sha256'],
        raw_D_by_mode_minimum_J=batch['raw_D_by_mode_minimum_J'],
        retirement_checkpoint_time_s=batch['retirement_checkpoint_time_s'])
    evidence.bytes+=path.stat().st_size

def run_case(case,rate,archive_root,bindings,np,signal,native_solve,physical_lambda_bound,budget,qualified_api):
    require(rate in (96000,192000),'fixed prospective refinement rates')
    factor=rate//PHYSICAL_RATE;last=(case['calls96']-1)*factor
    entry=case['contact_frame96']*factor;h=1/rate
    q=np.zeros((2,len(case['n'])));v=np.zeros_like(q)
    evidence=CaseArchive(archive_root,case,bindings,np,budget)
    taps=np.zeros(case['calls96']) if rate==96000 else None
    W=0.;D=np.zeros(3);max_numerator=0.;max_abs_work=0.;all_solver=True
    path,contact=attach(case,q,v,entry/rate,np,signal)
    evidence.active_sample(entry,q,v,{'event':'unstrained_attach',**contact})
    contact_intervals=0;tail_frames=0;retire_frame=None;last_complete=entry
    try:
        for k in range(entry,last):
            q1,v1,work,di,record=step(case,path,contact,q,v,k/rate,(k+1)/rate,h,np,native_solve,physical_lambda_bound)
            all_solver=all_solver and bool(record['native_status']['success'])
            W+=work;D+=di;contact_intervals+=1
            E=energy(case,q1,v1,np);tap=float(np.sum(q1*case['observer']))
            if taps is not None:taps[k+1]=tap
            z=path.value((k+1)/rate,'right')[0]
            U=.5*K*max(z-float(np.sum(case['B']*q1)),0.)**2 if contact['active'] else 0.
            numerator=E+U-W+float(D[0]);max_numerator=max(max_numerator,abs(numerator));max_abs_work=max(max_abs_work,abs(W))
            record.update({'time_s':(k+1)/rate,'physical_tap':tap,'string_energy_J':E,
                'external_work_J':W,'cumulative_D_Gramian_16_32_J':D.tolist(),
                'ledger_numerator_J':numerator,'interval_D_Gramian_16_32_J':di.tolist()})
            evidence.active_sample(k+1,q1,v1,record);q,v=q1,v1;last_complete=k+1
            if not contact['active']:
                retire_frame=k+1;evidence.tail_checkpoint(retire_frame,q,v,W,D,contact['retirement']);break
        if retire_frame is not None:
            for batch in batches(case,q,v,retire_frame,last,rate,W,D,qualified_api,np):
                save_tail_batch(evidence,batch)
                max_numerator=max(max_numerator,float(np.max(abs(batch['ledger_numerator_J']))))
                if taps is not None:taps[batch['frames']]=batch['physical_tap']
                q=batch['last_q'];v=batch['last_v'];D=batch['cumulative_D_Gramian_16_32_J'][-1].copy()
                last_complete=int(batch['frames'][-1]);tail_frames+=len(batch['frames'])
        scale=max(max_abs_work,1e-30)
        summary={'case_id':case['id'],'numerical_rate_hz':rate,'physical_path_rate_hz':96000,
            'source_callback_frames96':case['calls96'],'first_clock':case['source_metadata']['first_clock'],
            'source_contact_frame96':case['contact_frame96'],'source_release_frame96':case['release_frame96'],
            'complete_fixed_crop':last_complete==last,'all_solver_statuses_PASS':all_solver,
            'retired_zero_storage':retire_frame is not None,'retirement':contact.get('retirement'),
            'active_at_fixed_crop_end':contact['active'],'initial_stored_energy_J':0.,
            'state_error_normalization_energy_J':.5*case['Fq_N']*case['D_m'],
            'Fq_N':case['Fq_N'],'D_m':case['D_m'],'external_work_J':W,
            'final_D_Gramian_16_32_J':D.tolist(),'work_normalization_J':scale,
            'maximum_absolute_work_ledger_normalized':max_numerator/scale,
            'original5e_minus10_workrail_PASS':max_numerator/scale<=5e-10,
            'unapplied_original_kernel_mass_at_crop':path.unapplied_kernel_mass,
            'final_qv_sha256':state_hash(q,v),'contact_intervals':contact_intervals,'analytic_tail_frames':tail_frames,
            'free_tail_algorithm':'qualified stable analytic API from own checkpoint; integer elapsed frames/rate; actual analytic-left-state compositeD',
            'not_bit_identical_to_v1_recurrence':True,'no_PCM_or_full_mechanics_authority':True}
        summary['main_mechanics_gate_PARTIAL']=bool(all_solver and retire_frame is not None and summary['complete_fixed_crop']and summary['original5e_minus10_workrail_PASS'])
        if taps is not None:
            # Raw mechanics observation only. A separate exacthash all96 gate
            # must authorize conversion/export for the external-tap renderer.
            evidence._capacity(taps.nbytes)
            with (Path(archive_root)/'raw-mechanics-physical-tap.npz').open('xb')as f:
                np.savez_compressed(f,callback_frames=np.arange(case['calls96']),tap=taps)
        evidence.finish(True,summary)
        return summary
    except BaseException as error:
        evidence.finish(False,{'case_id':case['id'],'failure':repr(error),
            'source_failure_evidence':getattr(error,'source_evidence',None),
            'last_complete_frame':last_complete,'qv_last_accepted_sha256':state_hash(q,v),
            'all96_or_candidate_qualification':False,'no_retry_or_tail_rescue':True})
        raise

if __name__=='__main__':raise SystemExit('DRAFT only; no state/reference/force/field/PCM execution authorized')
