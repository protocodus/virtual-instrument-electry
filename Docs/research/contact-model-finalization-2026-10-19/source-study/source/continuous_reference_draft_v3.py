"""UNEXECUTED v3 fresh physical contact reference, no DG/native import.

Support knots are fixed96k at both first/refined tolerances. All velocity jumps
are segment boundaries. At a jump, inspect the actual two policy sides instead
of passing a discontinuous feasibility function to a smooth root finder.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
import math,bisect
from source_inputs_v1 import K,require
from mechanics_v2 import state_hash
ROOT_SIDE_MAX_ULPS=4096 # Already-owned bounded representable-side policy.

def integrate_active(case,path,qentry,ventry,refined,np,solve_ivp,save_dense,record_sink):
    m=case['mass'];shape=qentry.shape;omega=np.broadcast_to(case['omega'],shape).ravel()
    alpha=case['alpha'].ravel();B=case['B'].ravel();N=len(omega);sm=math.sqrt(m)
    S=max(math.sqrt(case['Fq_N']*case['D_m']),1e-12)
    scale=max(.5*case['Fq_N']*case['D_m'],1e-30)
    state=np.r_[sm*omega*qentry.ravel(),sm*ventry.ravel(),0.,0.]
    rtol=5e-12 if refined else 2e-10;atol0=5e-15 if refined else 2e-13
    atol=np.r_[np.full(2*N,atol0*S),atol0,atol0]
    maxstep=2*math.pi/(16*max(math.sqrt(float(max(omega)**2+K/m*sum(B*B))),2*float(max(alpha))))/(2 if refined else 1)
    segments=[(path.contact,path.release,'analytic_press')]+[(float(a),float(b),'fixed96k_PWL')for a,b in zip(path.knots[:-1],path.knots[1:])]
    records=[];boundary_facts=[];root_facts=[];retirement=None
    counts={'closing':0,'opening':0,'retirement_policy':0,'known_analytic_C1_entry_zero':0,'known_sided_physical_boundary':0,'genuine_interior_ambiguity':0}
    record_sink.update({'accepted_solver_records':records,'all_boundary_policy_facts':boundary_facts,'all_root_facts':root_facts,'event_counts':counts,'entry_q':qentry.copy(),'entry_v':ventry.copy(),'last_state':state.copy()})
    def read(value):return value[:N]/(sm*omega),value[N:2*N]/sm
    def info(t,value,side):
        q,v=read(value);z,zd,zdd=path.value(t,side)
        delta=z-float(B@q);rv=zd-float(B@v);force=K*max(delta,0.)
        g=max(delta,path.duration*rv)
        return q,v,delta,rv,g,force,z,zd,zdd
    def retire(t,value,reason,facts):
        q,v,d,rv,g,F,*_=info(t,value,'right')
        require(d<=0 and rv<=0 and .5*K*max(d,0.)**2==0,'reference cannot delete stored contact')
        return {'time_s':t,'reason':reason,'policy':facts,'U_J':0.,
            'qv_before_sha256':state_hash(q.reshape(shape),v.reshape(shape)),
            'qv_after_sha256':state_hash(q.reshape(shape),v.reshape(shape))}
    for left,right,kind in segments:
        if left>=path.release:
            q,v=read(state);facts=path.policy_facts(left,float(B@q),float(B@v));boundary_facts.append(facts)
            if facts['right']['eligible']:
                retirement=retire(left,state,'right_side_of_physical_knot',facts);break
        def rhs(t,value):
            # End-of-segment work uses that interval's left support slope.
            side='left' if t==right else 'right'
            q,v,d,rv,g,F,z,zd,zdd=info(t,value,side)
            x=value[:N];p=value[N:2*N]
            return np.r_[omega*p,-omega*x-2*alpha*p+B*F/sm,
                         F*zd/scale,2*float(np.sum(alpha*p*p))/scale]
        def feasible(t,value):return info(t,value,'left' if t==right else 'right')[4]
        feasible.direction=-1.;feasible.terminal=True
        def closing(t,value):return info(t,value,'left'if t==right else'right')[2]
        closing.direction=1.;closing.terminal=False
        def opening(t,value):return info(t,value,'left'if t==right else'right')[2]
        opening.direction=-1.;opening.terminal=False
        descriptors=[('closing',closing),('opening',opening)]
        if left>=path.release:descriptors.append(('retirement_policy',feasible))
        record_sink.update({'attempt_interval_s':[left,right],'attempt_initial_state':state.copy(),'attempt_kind':kind})
        sol=solve_ivp(rhs,(left,right),state,method='DOP853',rtol=rtol,atol=atol,max_step=maxstep,
            dense_output=True,events=[f for _,f in descriptors])
        record_sink.update({'last_accepted_solver_times':sol.t.copy(),'last_accepted_solver_states':sol.y.copy(),'last_solver_success':bool(sol.success),'last_solver_message':sol.message})
        records.append({'start_s':left,'end_s':float(sol.t[-1]),'kind':kind,'success':bool(sol.success),
            'message':sol.message,'nfev':int(sol.nfev),'accepted_times_s':sol.t.copy(),
            'accepted_states':sol.y.copy(),'max_step_s':maxstep})
        require(sol.success,'fresh physical source reference failed')
        terminal=[]
        for (etype,_),event_times in zip(descriptors,sol.t_events):
            for et in event_times:
                et=float(et);value=sol.sol(et);side='left'if et==right else'right'
                eq,ev,ed,erv,eg,eF,ez,ezd,ezdd=info(et,value,side)
                eacc=ezdd-float(B@(-omega**2*eq-2*alpha*ev+B*eF/m))
                face='compression'if ed>path.duration*erv else'velocity'if ed<path.duration*erv else'tie'
                derivative=(erv if etype!='retirement_policy'or face=='compression'
                    else path.duration*eacc if face=='velocity'else None)
                direction=1 if etype=='closing'else-1
                known_entry=et==path.contact
                ki=bisect.bisect_left(path.knots,et);known_boundary=bool(ki<len(path.knots)and float(path.knots[ki])==et)
                consistent=derivative is not None and derivative*direction>0
                genuine=bool(not known_entry and not known_boundary and (et==left or not consistent))
                facts={'type':etype,'time_s':et,'delta_m':ed,'relative_velocity_m_per_s':erv,
                    'policy_g_m':eg,'raw_surface_active_face':face if etype=='retirement_policy'else'delta',
                    'requested_event_direction':direction,'event_surface_derivative_m_per_s':derivative,
                    'derivative_matches_requested_direction':consistent,'known_analytic_C1_entry_zero':known_entry,
                    'known_sided_physical_boundary':known_boundary,'genuine_interior_ambiguity':genuine}
                if known_boundary:facts['both_policy_sides']=path.policy_facts(et,float(B@eq),float(B@ev))
                counts[etype]+=1;counts['known_analytic_C1_entry_zero']+=int(known_entry)
                counts['known_sided_physical_boundary']+=int(known_boundary);counts['genuine_interior_ambiguity']+=int(genuine)
                root_facts.append(facts)
                if etype=='retirement_policy':terminal.append(et)
        if terminal:
            raw=float(terminal[0]);value=sol.sol(raw)
            q,v,d,rv,g,F,z,zd,zdd=info(raw,value,'left' if raw==right else 'right')
            qdd=-omega**2*q-2*alpha*v+B*F/m;acc=zdd-float(B@qdd)
            face='compression'if d>path.duration*rv else'velocity'if d<path.duration*rv else'tie'
            derivative=rv if face=='compression' else path.duration*acc if face=='velocity' else None
            root={'raw_root_s':raw,'delta_m':d,'relative_velocity_m_per_s':rv,'policy_g_m':g,
                'face':face,'surface_derivative_m_per_s':derivative,
                'known_sided_physical_boundary':raw==right,
                'genuine_interior_ambiguity':bool(raw!=right and (derivative is None or derivative>=0 or raw==left))}
            root['type']='retirement_representation'
            root_facts.append(root) # Same object retains later chosen-side proof.
            # A root exactly at a declared physical knot belongs to the
            # sided boundary scheduler, never the smooth-root classifier.
            if raw==right:
                root['classification']='declared_knot_left_root; right policy checked at next segment'
                boundary_facts.append(path.policy_facts(right,float(B@q),float(B@v)))
                save_dense(sol.sol,left,right,shape,sm,omega,scale);state=sol.sol(right);record_sink['last_state']=state.copy()
                continue
            # Never turn a tie/tangent/interior initial event into a crossing.
            require(not root['genuine_interior_ambiguity'],'unqualified interior smooth retirement root; raw facts retained')
            dense=sol.sol.interpolants[-1];lo=max(left,float(dense.t_min));hi=min(right,float(dense.t_max))
            inspect=lambda t:info(t,sol.sol(t),'right')
            valid=lambda t:inspect(t)[2]<=0 and inspect(t)[3]<=0
            steps=0
            if valid(raw):
                chosen=raw
                while steps<ROOT_SIDE_MAX_ULPS:
                    previous=float(np.nextafter(chosen,-np.inf));require(previous>=lo,'reference root left accepted interval')
                    steps+=1
                    if not valid(previous):infeasible=previous;break
                    chosen=previous
                else:raise RuntimeError('reference feasible-side budget exhausted')
            else:
                infeasible=raw
                while steps<ROOT_SIDE_MAX_ULPS:
                    following=float(np.nextafter(infeasible,np.inf));require(following<=hi,'reference root right accepted interval')
                    steps+=1
                    if valid(following):chosen=following;break
                    infeasible=following
                else:raise RuntimeError('reference feasible-side budget exhausted')
            iq=inspect(infeasible);cq=inspect(chosen)
            require(iq[4]>=0 and cq[4]<=0 and valid(chosen) and not valid(infeasible),'reference represented feasibility proof')
            state=sol.sol(chosen);root.update({'chosen_time_s':chosen,'bracket_s':[infeasible,chosen],
                'infeasible_left_delta_m':iq[2],'infeasible_left_relative_velocity_m_per_s':iq[3],
                'feasible_right_delta_m':cq[2],'feasible_right_relative_velocity_m_per_s':cq[3],
                'search_inspections':steps})
            save_dense(sol.sol,left,chosen,shape,sm,omega,scale);record_sink['last_state']=state.copy()
            facts=path.policy_facts(chosen,float(B@cq[0]),float(B@cq[1]))
            retirement=retire(chosen,state,'smooth_feasible_set_entry',facts);break
        save_dense(sol.sol,left,right,shape,sm,omega,scale);state=sol.sol(right);record_sink['last_state']=state.copy()
    q,v=read(state)
    return {'q_retire_or_end':q.reshape(shape),'v_retire_or_end':v.reshape(shape),
        'work_J':float(state[2*N])*scale,'D_J':float(state[2*N+1])*scale,
        'retirement':retirement,'retired':retirement is not None,
        'all_boundary_policy_facts':boundary_facts,'all_root_facts':root_facts,
        'accepted_solver_records':records,'event_counts':counts,'rtol':rtol,'atol0':atol0,
        'state_scale_sqrtJ':S,'coordinate_work_D_scale_J':scale,
        'no_DG_native_force_or_parameter_fit':True}

if __name__=='__main__':raise SystemExit('DRAFT only; no fresh reference state generation authorized')
