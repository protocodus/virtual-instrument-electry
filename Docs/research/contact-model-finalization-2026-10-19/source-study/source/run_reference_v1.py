"""Unexecuted complete fresh contact/independent analytic-tail reference."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
from source_inputs_v1 import require,K
from mechanics_v2 import energy,state_hash
from support_path_v1 import SupportPath
from continuous_reference_draft_v3 import integrate_active
from reference_tail_draft_v4 import tail,_first_grid_at_or_after
from storage_v3 import CaseArchive

def run(case,refined,root,bindings,np,signal,solve_ivp,roots_legendre,budget):
    archive=None;sink={};attempt={'stage':'reference archive initialization','case_id':case['id'],'refined':refined}
    try:
        archive=CaseArchive(root,case,bindings,np,budget);rate=192000
        entry=case['contact_frame96']*2;last=(case['calls96']-1)*2
        q0=np.zeros((2,len(case['n'])));v0=np.zeros_like(q0)
        path=SupportPath(case,0.,0.,np,signal);scalar=[];last_saved=entry-1;max_num=0.;maxW=0.;tail_paths=[];all_D_nonnegative=True
        def save_dense(sol,left,right,shape,sm,omega,coordinate_scale):
            nonlocal last_saved,max_num,maxW,all_D_nonnegative
            # Every fixed sample is written once. The right endpoint belongs
            # to the next physical segment or the method's own retired tail.
            first=max(last_saved+1,_first_grid_at_or_after(left))
            stop=min(last+1,_first_grid_at_or_after(right))
            frames=np.arange(first,stop,dtype=np.int64)
            if not len(frames):return
            tt=frames.astype(float)/rate;out=sol(tt);N=len(omega)
            qq=(out[:N]/(sm*omega[:,None])).reshape(2,len(case['n']),len(frames))
            vv=(out[N:2*N]/sm).reshape(2,len(case['n']),len(frames))
            for j,frame in enumerate(frames):
                q,v=qq[:,:,j],vv[:,:,j];t=float(tt[j]);z,zd,_=path.value(t,'right')
                delta=z-float(np.sum(case['B']*q));U=.5*K*max(delta,0.)**2
                W=float(out[2*N,j])*coordinate_scale;D=float(out[2*N+1,j])*coordinate_scale
                E=energy(case,q,v,np);tap=float(np.sum(case['observer']*q));num=E+U-W+D
                require(np.isfinite([E,U,W,D,tap,num]).all(),'nonfinite continuous source scalar')
                require(np.isfinite(q).all()and np.isfinite(v).all(),'nonfinite continuous source state')
                all_D_nonnegative=all_D_nonnegative and D>=0
                archive.active_sample(int(frame),q,v,{'reference':True,'physical_endpoint_force_N':K*max(delta,0.),
                    'delta_m':delta,'relative_velocity_m_per_s':zd-float(np.sum(case['B']*v)),
                    'string_energy_J':E,'contact_U_J':U,'work_J':W,'D_truth_J':D,'linear_pickup_tap':tap,'ledger_numerator_J':num})
                scalar.append([int(frame),E,U,W,D,tap,num]);max_num=max(max_num,abs(num));maxW=max(maxW,abs(W));last_saved=int(frame)
        attempt={'stage':'fresh continuous contact','q_entry':q0,'v_entry':v0,'reference_sink':sink}
        result=integrate_active(case,path,q0,v0,refined,np,solve_ivp,save_dense,sink)
        archive.save_record('raw-contact-reference',result)
        qret=result['q_retire_or_end'];vret=result['v_retire_or_end'];W=result['work_J'];D=result['D_J']
        require(np.isfinite(qret).all()and np.isfinite(vret).all()and np.isfinite([W,D]).all(),'nonfinite continuous event/end state')
        all_D_nonnegative=all_D_nonnegative and D>=0
        # The whole-ledger maximum includes every accepted ODE state and its
        # exact segment endpoint, in addition to all fixed output samples.
        # No extra trajectory, denser observation or tolerance is introduced.
        N=2*len(case['n']);sm=case['mass']**.5;om=np.broadcast_to(case['omega'],(2,len(case['n']))).ravel()
        for rec in result['accepted_solver_records']:
            vals=rec['accepted_states'];tt=rec['accepted_times_s']
            require(np.isfinite(vals).all(),'nonfinite accepted ODE coordinates')
            for j,t in enumerate(tt):
                qa=(vals[:N,j]/(sm*om)).reshape(2,len(case['n']))
                zz=path.value(float(t),'left'if float(t)==rec['end_s']else'right')[0]
                UU=.5*K*max(zz-float(np.sum(case['B']*qa)),0.)**2
                EE=.5*float(np.sum(vals[:2*N,j]**2))
                WW=float(vals[2*N,j])*result['coordinate_work_D_scale_J'];DD=float(vals[2*N+1,j])*result['coordinate_work_D_scale_J']
                require(np.isfinite([EE,UU,WW,DD]).all(),'nonfinite accepted ODE energy/work')
                all_D_nonnegative=all_D_nonnegative and DD>=0
                max_num=max(max_num,abs(EE+UU-WW+DD));maxW=max(maxW,abs(WW))
        retz=path.value(float(result['retirement']['time_s'])if result['retired']else last/rate,'right'if result['retired']else'left')[0]
        retU=.5*K*max(retz-float(np.sum(case['B']*qret)),0.)**2
        max_num=max(max_num,abs(energy(case,qret,vret,np)+retU-W+D))
        maxW=max(maxW,abs(W));retired=result['retired']
        if retired:
            event=float(result['retirement']['time_s'])
            archive.tail_checkpoint(frame=None,event_time_s=event,q=qret,v=vret,work=W,D=D,retirement=result['retirement'])
            archive.save_arrays('replay-checkpoint',q=qret,v=vret,event_time_s=event,frame=-1)
            first=_first_grid_at_or_after(event)
            require(first==last_saved+1,'active/reference tail sample coverage cannot skip/duplicate')
            times=np.arange(first,last+1,dtype=float)/rate
            if len(times):
                tail_sink={};attempt['tail_sink']=tail_sink
                for b in tail(case,result,times,np,roots_legendre,tail_sink):
                    attempt={'stage':'reference analytical tail','raw_attempted_chunk':b,'contact_reference_sink':sink,'tail_sink':tail_sink}
                    arrays={k:b[k]for k in ('frames','times_s','physical_tap','string_energy_J','work_J','interval_D16_32_J','D16_32_J','ledger_numerator_J','last_q','last_v')}
                    p=archive.save_analytic_batch('tail-'+str(int(b['frames'][0])),arrays,{
                        'own_real_event_time_s':event,'first_partial_interval_s':b['first_partial_interval_s'],
                        'ordered_frame_q_then_v_sha256':b['ordered_frame_q_then_v_sha256'],
                        'raw_full_h_D_by_mode_minimum_J':b['raw_full_h_D_by_mode_minimum_J'],
                        'method':'independent q/v equations; exact event offset; positive firstpartial plus composite perhD'})
                    tail_paths.append(p.name);max_num=max(max_num,float(np.max(abs(b['ledger_numerator_J']))));last_saved=int(b['frames'][-1])
        else:
            # Preserve the fixed final active endpoint; no free-tail invention.
            t=last/rate;z,zd,_=path.value(t,'left');delta=z-float(np.sum(case['B']*qret));U=.5*K*max(delta,0.)**2
            E=energy(case,qret,vret,np);tap=float(np.sum(case['observer']*qret));num=E+U-W+D
            require(np.isfinite([E,U,W,D,tap,num]).all(),'nonfinite active reference crop endpoint')
            archive.active_sample(last,qret,vret,{'reference':True,'active_crop_endpoint':True,
                'string_energy_J':E,'contact_U_J':U,'work_J':W,'D_truth_J':D,'linear_pickup_tap':tap,'ledger_numerator_J':num})
            scalar.append([last,E,U,W,D,tap,num]);max_num=max(max_num,abs(num));last_saved=last
        archive.flush_active();archive.save_arrays('active-scalars',rows=np.asarray(scalar))
        complete=last_saved==last;scale=max(maxW,1e-30);counts=result['event_counts']
        summary={'id':case['id'],'kind':'reference','refined':refined,'rate':rate,'complete':complete,'retired':retired,
            'retirement':result['retirement'],'event_counts':counts,'initial_stored_energy_J':0.,
            'characteristic_not_initial_energy_J':.5*case['Fq_N']*case['D_m'],
            'normalization_J':scale,'maximum_abs_normalized_ledger':max_num/scale,
            'continuous_work_gate_PASS':bool(complete and all_D_nonnegative and max_num/scale<=5e-8),'all_D_nonnegative':all_D_nonnegative,
            'no_genuine_interior_ambiguity':counts['genuine_interior_ambiguity']==0,
            'all_boundary_policy_facts_retained':True,'rtol':result['rtol'],'atol0':result['atol0'],
            'coordinate_work_D_scale_J':result['coordinate_work_D_scale_J'],'state_scale_sqrtJ':result['state_scale_sqrtJ']}
        summary['own_accuracy_PASS']=bool(complete and retired and summary['continuous_work_gate_PASS']and summary['no_genuine_interior_ambiguity'])
        descriptor={'archive':str(Path(root).resolve()),'kind':'reference','rate':rate,'entry_frame':entry,'last_frame':last,
            'retired':retired,'complete':complete,'active_chunks':archive.active_chunks,'scalar_blocks':tail_paths,
            'scalar_columns':['frame','E','U','W','Dtruth','tap','ledger_numerator'],'retirement':result['retirement']}
        archive.save_record('trajectory',descriptor);archive.finish(True,summary)
        return summary,descriptor
    except BaseException as error:
        if archive is not None:archive.abort(error,attempted={**attempt,'full_reference_sink':sink,'source_exception_evidence':getattr(error,'source_evidence',None)})
        raise

if __name__=='__main__':raise SystemExit('DRAFT only; no physical source/reference integration authorized')
