"""Unexecuted complete sampled source case with raw failure sink."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
from source_inputs_v1 import require,K
from mechanics_v2 import attach,step,energy,state_hash
from analytic_tail_v3 import batches
from storage_v3 import CaseArchive

def run(case,rate,root,bindings,np,signal,native_solve,lam,budget,api):
    archive=None;attempt={'stage':'archive initialization','case_id':case['id'],'rate':rate}
    try:
        archive=CaseArchive(root,case,bindings,np,budget)
        factor=rate//96000;last=(case['calls96']-1)*factor;entry=case['contact_frame96']*factor;h=1/rate
        q=np.zeros((2,len(case['n'])));v=np.zeros_like(q);W=0.;D=np.zeros(3);scalar=[];tail_paths=[]
        taps=np.zeros(case['calls96'])if rate==96000 else None
        attempt={'stage':'attach','q':q,'v':v,'time_s':entry/rate}
        path,contact=attach(case,q,v,entry/rate,np,signal)
        archive.active_sample(entry,q,v,{'event':'unstrained_attach',**contact})
        scalar.append([entry,0.,0.,0.,0.,0.,0.]);retire_frame=None;all_solver=True;last_done=entry
        max_num=0.;maxW=0.;all_finite=True
        for k in range(entry,last):
            attempt={'stage':'contact interval','frame':k,'q':q,'v':v,'W':W,'D':D.copy(),'contact':dict(contact)}
            q1,v1,work,di,record=step(case,path,contact,q,v,k/rate,(k+1)/rate,h,np,native_solve,lam)
            attempt.update({'attempted_q1':q1,'attempted_v1':v1,'native_equation_certificate':record,'raw_interval_D':di})
            all_solver=all_solver and bool(record['native_status']['success']);W+=work;D+=di
            E=energy(case,q1,v1,np);tap=float(np.sum(q1*case['observer']))
            z=path.value((k+1)/rate,'right')[0]
            U=.5*K*max(z-float(np.sum(case['B']*q1)),0.)**2 if contact['active']else 0.
            require(np.isfinite([E,U,W,*D,tap]).all(),'nonfinite applied source scalars')
            numerator=E+U-W+float(D[0]);max_num=max(max_num,abs(numerator));maxW=max(maxW,abs(W))
            record.update({'string_energy_J':E,'contact_U_J':U,'work_J':W,
                'D_Gramian_16_32_J':D.tolist(),'interval_D_Gramian_16_32_J':di.tolist(),
                'linear_pickup_tap':tap,'ledger_numerator_J':numerator})
            archive.active_sample(k+1,q1,v1,record);scalar.append([k+1,E,U,W,float(D[0]),tap,numerator])
            if taps is not None:taps[k+1]=tap
            q,v=q1,v1;last_done=k+1
            if not contact['active']:
                retire_frame=k+1
                archive.tail_checkpoint(frame=retire_frame,event_time_s=retire_frame/rate,q=q,v=v,work=W,D=D.copy(),retirement=contact['retirement'])
                archive.save_arrays('replay-checkpoint',q=q,v=v,event_time_s=retire_frame/rate,frame=retire_frame)
                break
        if retire_frame is not None:
            attempt={'stage':'analytic tail','checkpoint_q':q,'checkpoint_v':v,'retirement_frame':retire_frame,'W':W,'D':D.copy()}
            tail_sink={};attempt['tail_sink']=tail_sink
            for b in batches(case,q,v,retire_frame,last,rate,W,D,api,np,tail_sink):
                attempt={'stage':'analytic tail chunk','raw_attempted_chunk':b,'tail_sink':tail_sink}
                arrays={k:b[k]for k in ('frames','times_s','physical_tap','string_energy_J','work_J','interval_D_Gramian_16_32_J','cumulative_D_Gramian_16_32_J','ledger_numerator_J','last_q','last_v')}
                p=archive.save_analytic_batch('tail-'+str(int(b['frames'][0])),arrays,{
                    'ordered_frame_q_then_v_sha256':b['ordered_frame_q_then_v_sha256'],
                    'own_checkpoint_frame':retire_frame,'raw_D_by_mode_minimum_J':b['raw_D_by_mode_minimum_J'],
                    'method':'qualified analytic API, integer elapsed frames/rate; no recurrence bitidentity'})
                tail_paths.append(p.name);max_num=max(max_num,float(np.max(abs(b['ledger_numerator_J']))))
                all_finite=all_finite and bool(np.isfinite(b['ledger_numerator_J']).all())
                if taps is not None:taps[b['frames']]=b['physical_tap']
                q=b['last_q'];v=b['last_v'];D=b['cumulative_D_Gramian_16_32_J'][-1].copy();last_done=int(b['frames'][-1])
        archive.flush_active();archive.save_arrays('active-scalars',rows=np.asarray(scalar))
        if taps is not None:archive.save_arrays('raw-mechanics-physical-tap',frames=np.arange(case['calls96']),tap=taps)
        scale=max(maxW,1e-30);complete=last_done==last;retired=retire_frame is not None
        summary={'id':case['id'],'rate':rate,'kind':'model','complete':complete,'retired':retired,
            'all_solver_statuses_PASS':all_solver,'all_states_and_D_finite':all_finite,
            'actual_initial_stored_energy_J':0.,'characteristic_not_initial_energy_J':.5*case['Fq_N']*case['D_m'],
            'normalization_J':scale,'maximum_abs_normalized_ledger':max_num/scale,
            'work_gate_PASS':bool(complete and all_finite and max_num/scale<=5e-10),
            'retirement':contact.get('retirement'),'active_at_crop_end':contact['active'],
            'actual_physical_Fq_N':case['Fq_N'],'actual_physical_stroke_D_m':case['D_m'],
            'external_work_J':W,'final_D_Gramian_16_32_J':D.tolist(),
            'actual_force_work_do_not_come_from_legacy_renderer_metadata':True,
            'first_clock':case['source_metadata']['first_clock'],'calls96':case['calls96'],
            'contact_frame96':case['contact_frame96'],'release_frame96':case['release_frame96'],
            'final_qv_sha256':state_hash(q,v),'raw_candidate_taps_not_authorized_for_renderer':True}
        summary['PASS']=bool(complete and retired and all_solver and summary['work_gate_PASS'])
        descriptor={'archive':str(Path(root).resolve()),'kind':'model','rate':rate,'entry_frame':entry,
            'last_frame':last,'retired':retired,'complete':complete,'active_chunks':archive.active_chunks,
            'scalar_columns':['frame','E','U','W','Dtruth','tap','ledger_numerator'],
            'scalar_blocks':tail_paths,'retirement':contact.get('retirement')}
        archive.save_record('trajectory',descriptor);archive.finish(True,summary)
        return summary,descriptor
    except BaseException as error:
        if archive is not None:archive.abort(error,attempted={**attempt,'source_exception_evidence':getattr(error,'source_evidence',None)})
        raise

if __name__=='__main__':raise SystemExit('DRAFT only; no source states/force/fields authorized')
