"""UNEXECUTED consolidated saved-only all96 source audit DRAFT.

No numerical imports/producer payload reads on import. Free closed-form replay
owns saved-tail verification only; no active trajectory/force/ODE is generated.
Final exact producer/closed-schema bindings and root prereview are still required.
"""
if not __debug__:
    raise SystemExit('Optimized Python forbidden')
from pathlib import Path
from fractions import Fraction
import argparse,ast,bisect,gzip,hashlib,importlib.util,json,math,os,platform,struct,sys,traceback,types
P=Path(__file__).resolve().parent;R=P.parent;S=R/'build-passive-source-tone-candidate-capacity'
K=6000.;EPS=2.**-52
METRICS=('full_state','pick_displacement_m','pick_velocity_m_per_s','linear_pickup_tap','linear_pickup_tap_rate_s_inv')

def need(c,m):
    if not c:raise RuntimeError(m)
def read(p):return json.loads(Path(p).read_text())
def fact(p):
    with Path(p).open('rb')as f:h=hashlib.file_digest(f,'sha256').hexdigest()
    return {'bytes':Path(p).stat().st_size,'sha256':h}
def word(v):return struct.pack('>d',float(v)).hex()
def safe(v):
    if isinstance(v,dict):return {str(k):safe(x)for k,x in v.items()}
    if isinstance(v,(tuple,list)):return [safe(x)for x in v]
    if hasattr(v,'tolist'):return safe(v.tolist())
    if isinstance(v,float)and not math.isfinite(v):return {'nonfinite':repr(v),'binary64_hex':word(v)}
    return v
def extract(path,names,namespace):
    tree=ast.parse(path.read_text());nodes=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name in names]
    need({x.name for x in nodes}==set(names),'bound pure helper names')
    exec(compile(ast.Module(body=nodes,type_ignores=[]),str(path),'exec'),namespace)
    return types.SimpleNamespace(**{n:namespace[n]for n in names})

def gate(authpath):
    """No candidate payload hash scan occurs until exact fresh root authority."""
    a=read(authpath);m=read(P/'audit-manifest-v1.json')
    need(a.get('passive_source_saved_audit_authorized')is True,'fresh saved-only authority')
    need(a['audit_manifest_sha256']==fact(P/'audit-manifest-v1.json')['sha256']and a['audit_plan_sha256']==fact(P/'plan-draft-v1.json')['sha256'],'exact audit target')
    for key in('producer_manifest_sha256','producer_protocol_sha256','closed_receipt_sha256'):
        need(a[key]==m[key],'exact selected binding '+key)
    for section,base in(('artifacts',P),('inputs',R),('environment_files',Path('/'))):
        for name,expected in m[section].items():need(fact(base/name)==expected,'audit code/environment '+name)
    closedpath=R/a['closed_receipt_path'];need(fact(closedpath)['sha256']==m['closed_receipt_sha256'],'closed receipt identity')
    closed=read(closedpath)
    need(closed['run_closed']is True and closed['process_exit_code']==0,'closed source execution')
    need(closed['implementation_manifest_sha256']==m['producer_manifest_sha256']and closed['protocol_sha256']==m['producer_protocol_sha256'],'closed source target')
    need(fact(S/'implementation-manifest-v1.json')['sha256']==m['producer_manifest_sha256']and fact(S/'protocol-selected-v1.json')['sha256']==m['producer_protocol_sha256'],'selected producer unchanged')
    producer=read(S/'implementation-manifest-v1.json')
    need(str(Path(sys.executable).resolve())==m['python_real_executable']and sys.version==m['python_version'],'fixed runtime')
    need(sys.path==m['runtime']['direct_entry_sys_path'],'fixed direct-entry import path')
    need(platform.system()=='Darwin'and platform.machine()=='arm64','fixed strict native host')
    for key,value in producer['inherited_environment'].items():need(os.environ.get(key)==value,'fixed inherited environment '+key)
    for name in('numpy','scipy'):
        spec=importlib.util.find_spec(name);need(spec is not None and spec.origin is not None and str(Path(spec.origin).resolve().parent)==producer['packages'][name]['root'],'fixed preimport package root '+name)
    for name,sha in producer['Source24'].items():need(fact(R/name)['sha256']==sha,'current Source '+name)
    # All targets are now authorized closed inputs. No numerical decoding yet.
    for section in('artifacts','inputs','sample_bindings','environment_files'):
        base=S if section=='artifacts'else Path('/')if section=='environment_files'else R
        for name,expected in producer[section].items():need(fact(base/name)==expected,'producer input binding '+name)
    for name,expected in closed['artifacts'].items():need(fact(R/name)==expected,'closed source payload '+name)
    return a,m,producer,closed

class Audit:
    def __init__(self,np,manifest,producer,closed):
        self.np=np;self.manifest=manifest;self.producer=producer;self.closed=closed
        self.issues=[];self.unavailable=[];self.histories=[];self.contexts=[];self.count=0;self.native_sequence=0;self.invalid_D=0;self.literal_rows=None
        self.descriptive={}
        self.root=S/'run-v1';self.study=self.root/'study'
        h=R/'build-damped-native-unit-saved-audit/helpers_draft_v1.py'
        old=extract(h,['reused'],{'R':R,'Fraction':Fraction,'ast':ast,'types':types,'math':math,'importlib':importlib,'sys':sys,'extract':extract})
        self.scalar,self.ex,self.callback,self.analytic,self.oracle=old.reused(R,np)
        self.model_formula=extract(S/'analytic_tail_v3.py',['state'],{'math':math})
        self.ref_formula=extract(S/'reference_equations_v3.py',['state'],{'math':math})
        # Only the previously qualified coefficient API is imported. This does
        # not call its coefficient builder/evaluator or produce a trajectory.
        ap=R/producer['qualified_analytic_api']['path'];spec=importlib.util.spec_from_file_location('saved_source_qualified_api',ap)
        self.api=importlib.util.module_from_spec(spec);sys.modules[spec.name]=self.api;spec.loader.exec_module(self.api)
    def ok(self,c,label,detail=None):
        self.count+=1
        if not c:self.issues.append({'id':label,'detail':detail})
    def unavailable_case(self,identity,error):self.unavailable.append({'id':identity,'reason':repr(error),'traceback':traceback.format_exc(),'qualified':False})
    def close(self,a,b,label,scale=1.):
        np=self.np;a=np.asarray(a);b=np.asarray(b);self.ok(a.shape==b.shape,label+'/shape')
        if a.shape!=b.shape:return
        self.ok(np.array_equal(np.isfinite(a),np.isfinite(b)),label+'/finite pattern')
        valid=np.isfinite(a)&np.isfinite(b);d=float(np.max(abs(a[valid]-b[valid])))if np.any(valid)else 0.
        self.ok(d<=max(512*EPS,1e-15)*scale,label,{'difference':d,'dimensional_scale':scale})
    def compare(self,a,b,label):
        np=self.np
        if isinstance(b,dict):
            for k,v in b.items():
                self.ok(k in a,label+'/'+k+'/present')
                if k in a:self.compare(a[k],v,label+'/'+k)
        elif isinstance(b,(list,tuple,np.ndarray)):
            self.ok(len(a)==len(b),label+'/count')
            for i,(x,y)in enumerate(zip(a,b)):self.compare(x,y,label+'/'+str(i))
        elif b is None or isinstance(b,(str,bool,np.bool_)):self.ok(a==b,label)
        else:self.close(a,b,label,max(abs(float(b)),1.))
    def bound(self,path):
        name=str(Path(path).relative_to(R));need(name in self.closed['artifacts'],'unbound saved payload '+name)
    def js(self,path):self.bound(path);return read(path)
    def arrays(self,path):
        self.bound(path)
        with self.np.load(path,allow_pickle=False)as z:return {k:z[k].copy()for k in z.files}
    def input_arrays(self,path,names):
        need(str(path.relative_to(R))in self.producer['sample_bindings'],'unbound original input array')
        with self.np.load(path,allow_pickle=False)as z:return {k:z[k].copy()for k in names}
    def record(self,path):
        record=self.js(path)
        if 'raw_array_file'in record:return record['record'],Path(path).parent/record['raw_array_file']
        return record,None
    def raw(self,record,arraypath):
        self.bound(arraypath)
        with self.np.load(arraypath,allow_pickle=False)as z:return z[record['raw_array_npz_member']].copy()
    def energy(self,c,q,v):return .5*c['mass']*float(self.np.sum(v*v+(c['omega'][None,:]*q)**2))
    def D(self,c,q,v,u,rate):
        np=self.np;X=np.stack((c['omega'][None,:]*q-u/(c['mass']*c['omega'][None,:]),v),axis=-1)
        raw=c['mass']*np.einsum('ani,ankij,anj->ank',X,c['tables'][rate]['KX'],X)
        valid=bool(np.isfinite(raw).all()and np.all(raw>=0));self.ok(valid,c['id']+'/raw per-mode D');self.invalid_D+=int(not valid)
        return np.sum(raw,axis=(0,1))
    def support(self,c,t,side='right'):
        if t<c['release']or(t==c['release']and side=='left'):
            u=(t-c['contact'])/c['duration'];d=c['D_m']
            return d*(3*u*u-2*u*u*u),d*(6*u-6*u*u)/c['duration'],d*(6-12*u)/c['duration']**2
        knots=c['withdrawal_knots_s'];j=bisect.bisect_left(knots,t)
        if j<len(knots)and float(knots[j])==t:
            i=j-1 if side=='left'else j
            return float(c['withdrawal_z'][j]),float(c['withdrawal_slopes'][i])if 0<=i<len(c['withdrawal_slopes'])else math.nan,0.
        i=j-1;need(0<=i<len(c['withdrawal_slopes']),'support inside unchanged crop')
        return float(c['withdrawal_z'][i]+(t-knots[i])*c['withdrawal_slopes'][i]),float(c['withdrawal_slopes'][i]),0.
    def context(self,pre):
        np=self.np;cid=pre['id'];data=self.arrays(self.study/(cid.replace('/','--')+'.npz'))
        c={'id':cid,**pre,**data}
        # mass is an original declared coordinate, never inferred from response.
        original=next(x for x in self.Cpre['rows']if x['id']==cid)['contact'];c['mass']=float(original['mu_kg_m'])*float(original['L_m'])/2
        c['contact']=pre['contact_frame96']/96000;c['release']=pre['release_frame96']/96000;c['duration']=(pre['release_frame96']-pre['contact_frame96'])/96000
        c['S']=max(math.sqrt(pre['Fq_N']*pre['D_m']),1e-12);c['characteristic']=.5*pre['Fq_N']*pre['D_m']
        c['tables']={r:{k:data[str(r)+'_'+k]for k in('phi11','phi12','phi21','phi22','gamma_q','gamma_v','KX')}for r in(96000,192000)}
        self.ok(np.isfinite(c['omega']).all()and np.all(c['omega']>0)and c['alpha'].shape==c['B'].shape==c['observer'].shape==(2,len(c['n'])),cid+'/modal input scope')
        self.original_inputs(c,pre)
        return c

    def original_inputs(self,c,pre):
        """Literal original C coordinates/geometry and already-qualified tables.

        No coefficient generation or inferred loss law. The prescribed scalar
        withdrawal path is replayed from its original pulse/coefficient only.
        """
        np=self.np;cid=c['id'];old=next(x for x in self.Cpre['rows']if x['id']==cid);contact=old['contact'];case=next(x for x in self.Cprotocol['all96_cases']if x['id']==cid)
        param=R/'build-inherited-field-C'/old['parameter_file'];original=self.input_arrays(param,('n','omega','alpha','force_shape','spatial','pulse'))
        self.ok(fact(param)['sha256']==old['parameter_sha256']==pre['parameter_sha256'],cid+'/original parameter identity')
        for key in('n','omega','alpha','pulse'):self.ok(np.array_equal(c[key],original[key]),cid+'/original '+key)
        raw=np.asarray([contact['vertical_weight'],contact['horizontal_weight']],dtype=float);normal=raw/math.sqrt(float(raw@raw));polarity=float(contact['polarity'])
        expectedB=polarity*normal[:,None]*original['force_shape'][None,:];expectedObserver=(np.asarray([.85,.35])/.040)[:,None]*original['spatial'][None,:]
        self.ok(np.array_equal(c['B'],expectedB)and np.array_equal(c['observer'],expectedObserver),cid+'/original signed actuation/unsigned observation')
        meta_binding=case['artifacts']['old_geometry_metadata'];meta_path=R/meta_binding['path'];need(str(meta_path.relative_to(R))in self.producer['inputs'],'bound original geometry metadata');meta=read(meta_path)
        self.ok(fact(meta_path)['sha256']==meta_binding['sha256'],cid+'/original metadata identity')
        amplitude=float(contact['velocity_amplitude'])if case['scope']=='neck'else float(meta['profile_amplitude'])
        fq=4.8*amplitude;L,T,a=[float(contact[k])for k in('L_m','T_N','pick_fraction')];stroke=fq*(L*a*(1-a)/T+1/K)
        self.ok(word(pre['Fq_N'])==word(fq)and word(pre['D_m'])==word(stroke),cid+'/original physical force/stroke')
        self.ok(pre['contact_frame96']==int(old['first_contact_index'])and pre['release_frame96']==int(old['first_release_index'])and pre['calls96']==int(old['calls'])and pre['first_clock']==meta['first_clock'],cid+'/original clocks/crop')
        self.ok(contact['internal_rate']==96000 and float(contact['release_coefficient'])>=0 and len(original['pulse'])==int(contact['release_length']),cid+'/original prescribed release metadata')
        from scipy.signal import lfilter
        count=int(old['calls'])-int(old['first_release_index']);aa=float(contact['release_coefficient']);rate=lfilter([1-aa],[1,-aa],np.pad(original['pulse'],(0,count-len(original['pulse']))));Rsum=np.cumsum(rate)
        zz=stroke-2*stroke*Rsum;knots=(int(old['first_release_index'])+np.arange(count,dtype=float))/96000;slopes=np.diff(zz)*96000
        for key,value in(('withdrawal_rate',rate),('withdrawal_R',Rsum),('withdrawal_z',zz),('withdrawal_knots_s',knots),('withdrawal_slopes',slopes)):self.ok(np.array_equal(c[key],value),cid+'/owned prescribed path '+key)
        if self.literal_rows is None:
            self.literal_rows={};rows=0;archive=R/self.producer['coefficient_rows_path'];need(str(archive.relative_to(R))in self.producer['sample_bindings'],'bound literal coefficient archive')
            with gzip.open(archive,'rt')as file:
                for line in file:
                    row=json.loads(line);rows+=1;need(row['PASS']is True and row['complete']is True,'qualified literal row');sp=row['specification']
                    if sp['kind']!='actual_C':continue
                    key=(sp['case_id'],sp['axis'],int(sp['n']),float(sp['h']).hex());need(key not in self.literal_rows,'unique literal row key')
                    alpha=float(sp['alpha']);h=float(sp['h'])
                    self.literal_rows[key]={'id':row['id'],'coordinate_words':[word(sp[k])for k in('mass','omega','alpha')],
                        'psi':np.asarray(row['transition']), 'bx':np.asarray(row['force_response']),
                        'KX':np.asarray([row['D_operator'],2*alpha*h*np.asarray(row['W16']),2*alpha*h*np.asarray(row['W32'])])}
            self.ok(rows==70544 and len(self.literal_rows)==68240,'all owned qualified literal rows')
        for rate in(96000,192000):
            expected={k:[]for k in('phi11','phi12','phi21','phi22','gamma_q','gamma_v','KX')};ids=[]
            for axis,name in enumerate(('v','h')):
                rr=[self.literal_rows[(cid,name,int(n),(1/rate).hex())]for n in original['n']];ids.append([x['id']for x in rr])
                for j,row in enumerate(rr):self.ok(row['coordinate_words']==[word(c['mass']),word(original['omega'][j]),word(original['alpha'][axis,j])],cid+'/literal modal coordinates')
                psi=np.asarray([x['psi']for x in rr]);bx=np.asarray([x['bx']for x in rr]);omega=original['omega']
                values={'phi11':psi[:,0,0],'phi12':psi[:,0,1]/omega,'phi21':psi[:,1,0]*omega,'phi22':psi[:,1,1],'gamma_q':bx[:,0]/omega,'gamma_v':bx[:,1],'KX':np.asarray([x['KX']for x in rr])}
                for key,value in values.items():expected[key].append(value)
            self.ok(pre['coefficient_row_ids'][str(rate)]==ids,cid+'/exact literal row IDs')
            for key,value in expected.items():self.ok(np.array_equal(c['tables'][rate][key],np.stack(value)),cid+'/literal table '+str(rate)+'/'+key)
            A=float(np.sum(expectedB*c['tables'][rate]['gamma_q']*expectedB));lam=Fraction.from_float(A)*3000
            declared=next(x for x in pre['actual_mobility']if x['rate']==rate)
            self.ok(word(declared['A_m_per_N'])==word(A)and declared['lambda_exact']==[lam.numerator,lam.denominator],cid+'/original literal-table physical mobility')
    def construction(self,c,q,v,q1,z0,z1,f,cert,rate):
        np=self.np;ex=self.ex;tab=c['tables'][rate];B=c['B'];qfree=tab['phi11']*q+tab['phi12']*v
        bd=list(map(ex.dyad,B.ravel()));mob=list(map(ex.dyad,tab['gamma_q'].ravel()))
        gather=lambda x:ex.exact_products([bd,list(map(ex.dyad,x.ravel()))])
        d0=ex.rational(z0)-gather(q);df=ex.rational(z1)-gather(qfree);d1=ex.rational(z1)-gather(q1)
        gram=ex.exact_products([bd,mob,bd]);A=float(np.sum(B*tab['gamma_q']*B))
        e0=abs(d0-ex.rational(z0-float(np.sum(B*q))));ef=abs(df-ex.rational(z1-float(np.sum(B*qfree))))
        op=abs(gram-ex.rational(A));fr=ex.rational(f);applied=abs(fr-ex.dg(d0,d1));exact=Fraction(*cert['exact_residual_inf_N'])
        expected={'binary_full_Gram_m_per_N':ex.pair(gram),'d0_gather_error_m':ex.pair(e0),'dfree_gather_error_m':ex.pair(ef),'solve_vs_binary_Gram_error_m_per_N':ex.pair(op),'binary_flow_target_residual_bound_N':ex.pair(exact+3000*(e0+ef+op*abs(fr))),'actual_applied_endpoint_residual_N':ex.pair(applied),'contact_residual_work_defect_bound_J':ex.pair(applied*abs(d1-d0))}
        for key,value in expected.items():self.ok(cert['construction'][key]==value,c['id']+'/EXACT applied construction/'+key)
    def native(self,c,record,q,v,q1,z0,z1,rate):
        np=self.np;tab=c['tables'][rate];B=c['B'];f=float(record['force_N']);A=float(np.sum(B*tab['gamma_q']*B));qf=tab['phi11']*q+tab['phi12']*v
        d0=z0-float(np.sum(B*q));df=z1-float(np.sum(B*qf));status=record['native_status'];cert=status['certificate'];native=cert['native_result']
        self.ok(native['id']==self.native_sequence,c['id']+'/native sequence');self.native_sequence+=1
        self.ok(cert['equation_raw']=={'A':[[word(A)]],'d0':[word(d0)],'dfree':[word(df)]},c['id']+'/raw stored equation')
        self.ok(word(f)==native['force'][0],c['id']+'/unmodified returned force')
        proof=False;proof_error=None
        try:self.oracle.accepted_trace([[A]],[d0],[df],native);proof=True
        except Exception as error:proof_error=repr(error)
        self.ok(cert['native_interval_checker_PASS']==proof,c['id']+'/independent interval trace flag')
        exact=self.oracle.exact_saved_residual([[A]],[d0],[df],[f]);self.ok(cert['exact_residual_inf_N']==[exact.numerator,exact.denominator],c['id']+'/exact residual')
        ordinary=float(abs(f-self.callback.dg_force_and_derivative(np.array([d0]),np.array([df])-np.array([[A]])@np.array([f]))[0][0]))
        ordinary_ok=bool(math.isfinite(ordinary)and ordinary<=2e-11);exact_ok=exact<=self.oracle.RAIL;finite_nonnegative=bool(math.isfinite(f)and f>=0)
        good=bool(native['status']==0 and native['exact_stored_residual_certified']and native['cpp_legacy_diagnostic_pass']and native['posterior_error_bound_valid']and proof and exact_ok and ordinary_ok and finite_nonnegative)
        self.ok(cert['success']==status['success']==good and cert['exact_residual_PASS']==exact_ok and cert['finite_nonnegative_force']==finite_nonnegative and cert['actual_original_numpy_residual_PASS']==ordinary_ok,c['id']+'/FULL inherited native admission')
        posterior=exact/(1-self.oracle.LAM);self.ok(cert['exact_posterior_diagnostic_N']==[posterior.numerator,posterior.denominator],c['id']+'/exact posterior diagnostic')
        saved=self.oracle.value(cert['actual_original_numpy_residual_raw']);self.ok(cert['actual_original_numpy_residual_N']==(saved if math.isfinite(saved)else None),c['id']+'/saved residual raw/numeric')
        self.ok(word(status['residual_inf_N'])==word(saved)==word(record['actual_original_numpy_residual_N']),c['id']+'/raw status/record residual join')
        self.ok(status['nfev']==native['map_evaluations']and status['analytic_zero']==native['analytic_zero_solution'],c['id']+'/native map count/analytic flag')
        self.ok(status['negative_force_count']==int(f<0)and status['force_below_negative_1e_minus12_N_count']==int(f< -1e-12)and word(status['minimum_f_N'])==word(f),c['id']+'/native returned force diagnostics')
        self.descriptive[c['id']+'/ordinary recomputed vs saved']=max(self.descriptive.get(c['id']+'/ordinary recomputed vs saved',0.),abs(ordinary-saved))
        # Recomputed-vs-saved ordinary differences remain descriptive.
        self.construction(c,q,v,q1,z0,z1,f,cert,rate)
        return f,good
    def active(self,c,root,kind,rate):
        np=self.np;descriptor=self.js(root/'trajectory.json');completion=self.js(root/'completion.json');summary=completion['summary']
        self.ok(completion['complete']is True,c['id']+'/'+kind+'/complete archive')
        paths=sorted(root.glob('contact-qv-*.npz'));self.ok([p.name for p in paths]==descriptor['active_chunks'],c['id']+'/active chunk ownership')
        scalars=self.arrays(root/'active-scalars.npz')['rows'];rowindex=0;previous=None;first=None;D=np.zeros(3);W=0.;maxnum=maxW=0.;allsolver=True;finite=True;Dnonnegative=True
        exported=None;export_filled=None
        if kind=='model'and rate==96000:
            export=self.arrays(root/'raw-mechanics-physical-tap.npz');exported=export['tap'];expected=np.arange(c['calls96'])
            self.ok(np.array_equal(export['frames'],expected)and exported.shape==(c['calls96'],)and np.isfinite(exported).all(),c['id']+'/all original raw export frames/finite taps')
            self.ok(np.array_equal(exported[:c['contact_frame96']],np.zeros(c['contact_frame96'])),c['id']+'/raw export complete rest prefix')
            export_filled=np.zeros(c['calls96'],dtype=bool);export_filled[:c['contact_frame96']]=True
        path=root/'all-active-native-certificates.jsonl.gz';self.bound(path)
        with gzip.open(path,'rt')as stream:
            for p in paths:
                block=self.arrays(p)
                for frame,q,v in zip(block['frames'],block['q'],block['v']):
                    frame=int(frame);raw=stream.readline();need(bool(raw),'missing active equation record');rec=json.loads(raw)
                    self.ok(rec['frame']==frame and np.isfinite(q).all()and np.isfinite(v).all(),c['id']+'/active raw frame/state')
                    if first is None:first=frame;self.ok(first==c['contact_frame96']*(rate//96000)and np.array_equal(q,np.zeros_like(q))and np.array_equal(v,np.zeros_like(v)),c['id']+'/unstrained rest attachment')
                    else:
                        k,q0,v0=previous;self.ok(frame==k+1,c['id']+'/complete active frames');t0=k/rate;t1=frame/rate
                        if kind=='model':
                            z0=float(rec['support_z0_m']);z1=float(rec['support_z1_m'])
                            self.close([z0,z1],[self.support(c,t0)[0],self.support(c,t1)[0]],c['id']+'/independent support vs actual stored bits',c['D_m'])
                            f,good=self.native(c,rec,q0,v0,q,z0,z1,rate);allsolver=allsolver and good
                            tab=c['tables'][rate];u=c['B']*f;qr=tab['phi11']*q0+tab['phi12']*v0+tab['gamma_q']*u;vr=tab['phi21']*q0+tab['phi22']*v0+tab['gamma_v']*u
                            error=math.sqrt(c['mass'])*math.sqrt(float(np.sum((c['omega'][None,:]*(qr-q))**2+(vr-v)**2)))/c['S'];self.ok(error<=2e-11,c['id']+'/saved held-force interval',error)
                            di=self.D(c,q0,v0,u,rate);D+=di;W+=f*(z1-z0)
                            self.close(rec['interval_D_Gramian_16_32_J'],di,c['id']+'/interval D',c['characteristic']);self.close(rec['D_Gramian_16_32_J'],D,c['id']+'/cumulative D',c['characteristic'])
                    z,zd,_=self.support(c,frame/rate)
                    if kind=='model'and previous is not None:z=float(rec['support_z1_m'])
                    delta=z-float(np.sum(c['B']*q));U=.5*K*max(delta,0.)**2
                    if descriptor['retired']and descriptor['retirement']['time_s']==frame/rate:U=0.
                    E=self.energy(c,q,v);tap=float(np.sum(c['observer']*q))
                    if exported is not None:self.ok(word(exported[frame])==word(tap),c['id']+'/exported active tap raw join');export_filled[frame]=True
                    if kind=='reference':W=float(rec['work_J']);D[0]=float(rec['D_truth_J']);self.ok(D[0]>=0,c['id']+'/reference nonnegative D')
                    numerator=E+U-W+D[0];maxnum=max(maxnum,abs(numerator));maxW=max(maxW,abs(W))
                    finite=finite and bool(np.isfinite(q).all()and np.isfinite(v).all()and np.isfinite([E,U,W,*D,tap,numerator]).all());Dnonnegative=Dnonnegative and bool(np.all(D>=0))
                    energy_scale=max(c['characteristic'],abs(W),1e-30);tap_scale=c['S']/math.sqrt(c['mass'])*float(np.linalg.norm(c['observer']/c['omega'][None,:]))
                    self.ok(scalars[rowindex,0]==frame,c['id']+'/scalar frame');self.close(scalars[rowindex,1:5],[E,U,W,D[0]],c['id']+'/active energy/work/D',energy_scale)
                    self.close(scalars[rowindex,5],tap,c['id']+'/active tap',tap_scale);self.close(scalars[rowindex,6],numerator,c['id']+'/active ledger numerator',energy_scale);rowindex+=1
                    previous=(frame,q.copy(),v.copy())
            self.ok(not stream.readline(),c['id']+'/no extra active equations')
        self.ok(rowindex==len(scalars),c['id']+'/all active scalar rows')
        return descriptor,summary,{'W':W,'D':D,'maxnum':maxnum,'maxW':maxW,'allsolver':allsolver,'finite':finite,'Dnonnegative':Dnonnegative,'last':previous,'exported':exported,'export_filled':export_filled}

    # Tail/reference/coverage functions below are deliberately part of this one
    # checker, not separately executed studies or an alternative producer.
    def tail(self,c,root,descriptor,stats):
        np=self.np;rate=descriptor['rate'];kind=descriptor['kind'];ret=descriptor['retired']
        if not ret:return
        cp=self.arrays(root/'replay-checkpoint.npz');qret,vret=cp['q'],cp['v'];event=float(cp['event_time_s']);frame0=int(cp['frame'])
        fullcp=self.arrays(root/'post-retirement-initial-state.npz');self.ok(np.array_equal(fullcp['q'],qret)and np.array_equal(fullcp['v'],vret),c['id']+'/own checkpoint identity')
        z,zd,_=self.support(c,event);delta=z-float(np.sum(c['B']*qret));rv=zd-float(np.sum(c['B']*vret))
        self.ok(delta<=0 and rv<=0 and .5*K*max(delta,0.)**2==0,c['id']+'/zero storage retirement')
        digest=hashlib.sha256(qret.tobytes()+vret.tobytes()).hexdigest();self.ok(descriptor['retirement']['qv_before_sha256']==descriptor['retirement']['qv_after_sha256']==digest,c['id']+'/unchanged retirement field')
        if kind=='model':self.ok(np.array_equal(stats['last'][1],qret)and np.array_equal(stats['last'][2],vret),c['id']+'/sampled retirement exact checkpoint')
        D=np.asarray(fullcp['D']).copy();D=np.asarray(D if kind=='model'else [float(D),float(D)])
        W=float(fullcp['work']);last=stats['last'][0];qleft,vleft=qret,vret
        for name in descriptor['scalar_blocks']:
            b=self.arrays(root/name);prov,_=self.record(root/(Path(name).stem+'-provenance.json'));frames=b['frames'];times=b['times_s']
            self.ok(np.array_equal(times,frames.astype(float)/rate)and np.array_equal(np.diff(frames),np.ones(max(len(frames)-1,0),dtype=int)),c['id']+'/tail canonical grid')
            elapsed=(frames-frame0).astype(float)/rate if kind=='model'else times-event
            if kind=='model':q,v=self.model_formula.state(c,qret,vret,elapsed,self.api,np)
            else:q,v=self.ref_formula.state(c,qret,vret,elapsed,np)
            ordered=np.stack((np.moveaxis(q,2,0),np.moveaxis(v,2,0)),axis=1)
            self.ok(hashlib.sha256(ordered.tobytes()).hexdigest()==prov['ordered_frame_q_then_v_sha256'],c['id']+'/all saved tail qv hash')
            self.ok(np.array_equal(q[:,:,-1],b['last_q'])and np.array_equal(v[:,:,-1],b['last_v']),c['id']+'/tail endpoint raw identity')
            self.ok(int(frames[0])==last+1,c['id']+'/no skipped tail samples');last=int(frames[-1])
            # Exact real-event first partial uses the registered positive16/32
            # rule; subsequent intervals use literal full-h operators.
            offset=1 if kind=='reference'and name==descriptor['scalar_blocks'][0]else 0
            inc=np.empty((len(times),3 if kind=='model'else 2))
            if offset:
                width=float(times[0]-event);values=[]
                for order in(16,32):
                    from scipy.special import roots_legendre
                    if width==0:raw=np.zeros_like(qret)
                    else:
                        nodes,weights=roots_legendre(order);_,vv=self.ref_formula.state(c,qret,vret,(nodes+1)*width/2,np)
                        raw=c['mass']*c['alpha']*width*np.sum(vv*vv*weights[None,None,:],axis=2)
                    valid=bool(np.isfinite(raw).all()and np.all(raw>=0));self.ok(valid,c['id']+'/raw partial D');self.invalid_D+=int(not valid);values.append(float(np.sum(raw)))
                inc[0]=values
            if len(times)>offset:
                lq=q[:,:,:-1]if offset else np.concatenate((qleft[:,:,None],q[:,:,:-1]),axis=2)
                lv=v[:,:,:-1]if offset else np.concatenate((vleft[:,:,None],v[:,:,:-1]),axis=2)
                X=np.stack((c['omega'][None,:,None]*lq,lv),axis=-1);operators=c['tables'][rate]['KX']if kind=='model'else c['tables'][rate]['KX'][:,:,1:3]
                raw=c['mass']*np.einsum('anti,ankij,antj->antk',X,operators,X)
                valid=bool(np.isfinite(raw).all()and np.all(raw>=0));self.ok(valid,c['id']+'/raw batched tail D');self.invalid_D+=int(not valid);inc[offset:]=np.sum(raw,axis=(0,1))
            qleft=q[:,:,-1].copy();vleft=v[:,:,-1].copy()
            # Preserve the producer's two declared chronological sum orders.
            cumulative=D[None,:]+np.cumsum(inc,axis=0)if kind=='model'else np.cumsum(np.concatenate((D[None,:],inc)),axis=0)[1:]
            self.close(b['interval_D_Gramian_16_32_J'if kind=='model'else'interval_D16_32_J'],inc,c['id']+'/tail interval D',c['characteristic'])
            self.close(b['cumulative_D_Gramian_16_32_J'if kind=='model'else'D16_32_J'],cumulative,c['id']+'/tail cumulative D',c['characteristic']);D=cumulative[-1].copy()
            E=.5*c['mass']*np.sum(v*v+(c['omega'][None,:,None]*q)**2,axis=(0,1));tap=np.sum(c['observer'][:,:,None]*q,axis=(0,1));numerator=E-W+cumulative[:,0 if kind=='model'else 1]
            if stats['exported']is not None:
                self.ok(np.array_equal(stats['exported'][frames],b['physical_tap']),c['id']+'/raw export tail tap join');stats['export_filled'][frames]=True
            stats['finite']=stats['finite']and bool(np.isfinite(q).all()and np.isfinite(v).all()and np.isfinite(cumulative).all()and np.isfinite(E).all()and np.isfinite(tap).all()and np.isfinite(numerator).all());stats['Dnonnegative']=stats['Dnonnegative']and bool(np.all(cumulative>=0))
            for key,value in [('string_energy_J',E),('physical_tap',tap),('work_J',np.full(len(times),W)),('ledger_numerator_J',numerator)]:
                scale=c['S']/math.sqrt(c['mass'])*float(np.linalg.norm(c['observer']/c['omega'][None,:]))if key=='physical_tap'else max(c['characteristic'],abs(W),1e-30)
                self.close(b[key],value,c['id']+'/tail '+key,scale)
            stats['maxnum']=max(stats['maxnum'],float(np.max(abs(numerator))));stats['maxW']=max(stats['maxW'],abs(W))
        self.ok(last==descriptor['last_frame'],c['id']+'/complete tail crop')

    def reference_facts(self,c,root,summary,stats):
        np=self.np;raw,arraypath=self.record(root/'raw-contact-reference.json');counts=raw['event_counts'];facts=raw['all_root_facts']
        for kind in('closing','opening','retirement_policy'):self.ok(counts[kind]==sum(x['type']==kind for x in facts),c['id']+'/root counts '+kind)
        self.ok(counts['genuine_interior_ambiguity']==sum(bool(x.get('genuine_interior_ambiguity'))for x in facts if x['type']!='retirement_representation'),c['id']+'/ambiguity count')
        for rootfact in facts:
            if rootfact['type']in('closing','opening','retirement_policy'):
                kind=rootfact['type'];direction=1 if kind=='closing'else-1;derivative=rootfact['event_surface_derivative_m_per_s']
                if kind!='retirement_policy'or rootfact['raw_surface_active_face']=='compression':self.ok(derivative==rootfact['relative_velocity_m_per_s'],c['id']+'/compression surface derivative')
                consistent=derivative is not None and derivative*direction>0
                self.ok(rootfact['derivative_matches_requested_direction']==consistent,c['id']+'/event direction fact')
                self.ok(rootfact['known_analytic_C1_entry_zero']==(rootfact['time_s']==c['contact']),c['id']+'/known entry classification')
                index=bisect.bisect_left(c['withdrawal_knots_s'],rootfact['time_s']);known=bool(index<len(c['withdrawal_knots_s'])and float(c['withdrawal_knots_s'][index])==rootfact['time_s'])
                self.ok(rootfact['known_sided_physical_boundary']==known,c['id']+'/known sided knot classification')
            if 'bracket_s'in rootfact:
                left,right=rootfact['bracket_s'];self.ok(math.nextafter(left,math.inf)==right and rootfact['chosen_time_s']==right,c['id']+'/adjacent represented-side times')
                # Final source schema supplies BOTH feasibility faces. Do not
                # require compression to be the surface for a velocity event.
                self.ok((rootfact['infeasible_left_delta_m']>0 or rootfact['infeasible_left_relative_velocity_m_per_s']>0)and rootfact['feasible_right_delta_m']<=0 and rootfact['feasible_right_relative_velocity_m_per_s']<=0 and rootfact['search_inspections']<=4096,c['id']+'/represented feasible set')
        for record in raw['accepted_solver_records']:
            vals=self.raw(record['accepted_states'],arraypath);times=self.raw(record['accepted_times_s'],arraypath);N=2*len(c['n']);sm=math.sqrt(c['mass']);omega=self.np.broadcast_to(c['omega'],c['B'].shape).ravel()
            self.ok(np.isfinite(vals).all(),c['id']+'/accepted reference finite')
            for j,t in enumerate(times):
                q=(vals[:N,j]/(sm*omega)).reshape(c['B'].shape);z=self.support(c,float(t),'left'if float(t)==record['end_s']else'right')[0]
                E=.5*float(np.sum(vals[:2*N,j]**2));U=.5*K*max(z-float(np.sum(c['B']*q)),0.)**2;W=float(vals[2*N,j])*raw['coordinate_work_D_scale_J'];D=float(vals[2*N+1,j])*raw['coordinate_work_D_scale_J']
                self.ok(math.isfinite(D)and D>=0,c['id']+'/accepted nonnegative D');stats['Dnonnegative']=stats['Dnonnegative']and D>=0;stats['finite']=stats['finite']and bool(np.isfinite([E,U,W,D]).all())
                stats['maxnum']=max(stats['maxnum'],abs(E+U-W+D));stats['maxW']=max(stats['maxW'],abs(W))
        qret=self.raw(raw['q_retire_or_end'],arraypath);vret=self.raw(raw['v_retire_or_end'],arraypath);t=float(raw['retirement']['time_s'])if raw['retired']else(c['calls96']-1)/96000
        if raw['retired']:
            cp=self.arrays(root/'post-retirement-initial-state.npz');self.ok(np.array_equal(cp['q'],qret)and np.array_equal(cp['v'],vret)and float(cp['event_time_s'])==t,c['id']+'/reference raw event/checkpoint join')
            self.close(cp['work'],raw['work_J'],c['id']+'/reference event work join',c['characteristic']);self.close(cp['D'],raw['D_J'],c['id']+'/reference event D join',c['characteristic'])
        z=self.support(c,t,'right'if raw['retired']else'left')[0];U=.5*K*max(z-float(np.sum(c['B']*qret)),0.)**2;W=float(raw['work_J']);D=float(raw['D_J'])
        stats['maxnum']=max(stats['maxnum'],abs(self.energy(c,qret,vret)+U-W+D));stats['maxW']=max(stats['maxW'],abs(W));stats['Dnonnegative']=stats['Dnonnegative']and D>=0
        self.ok(summary['no_genuine_interior_ambiguity']==(counts['genuine_interior_ambiguity']==0),c['id']+'/reference ambiguity admission')
        # Nonterminal dense-root q/v and interpolants are not separately saved.
        # Their velocity-face acceleration/root-location facts remain bound
        # scalar/static evidence, not independently reconstructed array proof.

    def states(self,c,descriptor,frames):
        """Read saved active q/v; replay only its declared own free checkpoint."""
        np=self.np;root=Path(descriptor['archive']);need(root.is_relative_to(self.root),'bound source trajectory directory')
        frames=np.asarray(frames,dtype=np.int64);q=np.full((len(frames),2,len(c['n'])),np.nan);v=np.full_like(q,np.nan);filled=np.zeros(len(frames),dtype=bool)
        pre=frames<descriptor['entry_frame'];q[pre]=0.;v[pre]=0.;filled[pre]=True
        for name in descriptor['active_chunks']:
            block=self.arrays(root/name);ff=block['frames'];ids=np.flatnonzero((frames>=ff[0])&(frames<=ff[-1]))
            if not len(ids):continue
            pos=np.searchsorted(ff,frames[ids]);need(np.all(pos<len(ff))and np.array_equal(ff[pos],frames[ids]),'complete saved active frame ownership')
            need(not np.any(filled[ids]),'duplicate active state');q[ids]=block['q'][pos];v[ids]=block['v'][pos];filled[ids]=True
        if descriptor['retired']:
            cp=self.arrays(root/'replay-checkpoint.npz');event=float(cp['event_time_s']);ids=np.flatnonzero(~filled&(frames.astype(float)/descriptor['rate']>=event))
            if len(ids):
                if descriptor['kind']=='model':qq,vv=self.model_formula.state(c,cp['q'],cp['v'],(frames[ids]-int(cp['frame'])).astype(float)/descriptor['rate'],self.api,np)
                else:qq,vv=self.ref_formula.state(c,cp['q'],cp['v'],frames[ids].astype(float)/descriptor['rate']-event,np)
                q[ids]=np.moveaxis(qq,2,0);v[ids]=np.moveaxis(vv,2,0);filled[ids]=True
        need(np.all(filled)and np.isfinite(q).all()and np.isfinite(v).all(),'unavailable active state; no reconstruction/rescue')
        return q,v

    def comparison(self,c,left,right):
        np=self.np;rate=min(left['rate'],right['rate']);last=(c['calls96']-1)*(rate//96000)
        maxima={k:0. for k in METRICS};absolute={k:0. for k in METRICS if k!='full_state'}
        for first in range(0,last+1,4096):
            frames=np.arange(first,min(first+4096,last+1));qa,va=self.states(c,left,frames*(left['rate']//rate));qb,vb=self.states(c,right,frames*(right['rate']//rate));dq=qa-qb;dv=va-vb
            error=math.sqrt(c['mass'])*np.sqrt(np.sum((c['omega'][None,None,:]*dq)**2+dv*dv,axis=(1,2)));maxima['full_state']=max(maxima['full_state'],float(np.max(error))/c['S'])
            for read,disp,vel in((c['B'],'pick_displacement_m','pick_velocity_m_per_s'),(c['observer'],'linear_pickup_tap','linear_pickup_tap_rate_s_inv')):
                for diff,weights,key in((dq,read/c['omega'][None,:],disp),(dv,read,vel)):
                    raw=float(np.max(abs(np.sum(diff*read[None,:,:],axis=(1,2)))));bound=c['S']/math.sqrt(c['mass'])*float(np.linalg.norm(weights));absolute[key]=max(absolute[key],raw);maxima[key]=max(maxima[key],raw/bound)
        return {'fixed_metrics':maxima,'absolute_observation_maxima':absolute,'state_normalization_sqrtJ':c['S'],'characteristic_not_initial_storage_J':c['characteristic'],'compared_rate_hz':rate,'no_time_or_phase_alignment':True}

    def run(self):
        np=self.np;pre=self.js(self.study/'preflight-receipt-v1.json');self.Cpre=read(R/self.producer['C_preflight_path']);self.Cprotocol=read(R/self.producer['C_protocol_path']);aggregate=self.js(self.study/'aggregate-v1.json')
        self.ok(len(pre['rows'])==96 and [x['id']for x in pre['rows']]==self.producer['all96_case_ids'],'fixed all96 ordered contexts')
        for row in pre['rows']:
            cid=row['id'];c=self.context(row);root=self.root/cid.replace('/','--');case=self.js(self.study/('case-'+cid.replace('/','--')+'.json'));hist={}
            for sub,kind,rate in(('model96','model',96000),('model192','model',192000),('reference-first','reference',192000),('reference-refined','reference',192000)):
                identity=cid+'/'+sub
                try:
                    previous_invalid_D=self.invalid_D
                    d,s,stats=self.active(c,root/sub,kind,rate);self.tail(c,root/sub,d,stats)
                    if stats['export_filled']is not None:self.ok(bool(np.all(stats['export_filled'])),identity+'/complete export-input ownership, no unjoined samples')
                    if kind=='reference':self.reference_facts(c,root/sub,s,stats)
                    scale=max(stats['maxW'],1e-30);ledger=stats['maxnum']/scale;self.close(s['normalization_J'],scale,identity+'/actual work normalization',max(scale,1e-30));self.close(s['maximum_abs_normalized_ledger'],ledger,identity+'/whole ledger')
                    rail=5e-10 if kind=='model'else 5e-8;work=bool(s['complete']and stats['finite']and stats['Dnonnegative']and ledger<=rail and self.invalid_D==previous_invalid_D)
                    self.ok(s['work_gate_PASS'if kind=='model'else'continuous_work_gate_PASS']==work,identity+'/original whole-work gate')
                    good=bool(work and s['retired']and(stats['allsolver']if kind=='model'else s['no_genuine_interior_ambiguity']))
                    if kind=='model':self.compare(s,{'all_solver_statuses_PASS':stats['allsolver'],'all_states_and_D_finite':stats['finite']},identity+'/saved scientific flags')
                    else:self.compare(s,{'all_D_nonnegative':stats['Dnonnegative']},identity+'/saved scientific flags')
                    self.ok(s['PASS'if kind=='model'else'own_accuracy_PASS']==good,identity+'/own scientific admission')
                    hist[sub]={'descriptor':d,'summary':s,'ledger':ledger,'qualified':good};self.histories.append({'id':identity,'available':True,'scientifically_qualified':good,'summary':s,'reconstructed_ledger':ledger})
                except Exception as error:self.unavailable_case(identity,error);self.histories.append({'id':identity,'available':False,'scientifically_qualified':False})
            joins={'id':cid,'available_histories':len(hist),'comparison_available':False,'scientifically_qualified':False}
            if len(hist)==4:
                try:
                    refinement=self.comparison(c,hist['reference-first']['descriptor'],hist['reference-refined']['descriptor']);e96=self.comparison(c,hist['model96']['descriptor'],hist['reference-refined']['descriptor']);e192=self.comparison(c,hist['model192']['descriptor'],hist['reference-refined']['descriptor'])
                    same=bool(hist['reference-first']['summary']['retired']==hist['reference-refined']['summary']['retired']and hist['reference-first']['summary']['retired'])
                    refgate=bool(hist['reference-first']['qualified']and hist['reference-refined']['qualified']and same and refinement['fixed_metrics']['full_state']<=1e-7);admitted=bool(refgate and hist['model96']['qualified']and hist['model192']['qualified'])
                    rr=[]
                    for metric in METRICS:
                        x=e96['fixed_metrics'][metric];y=e192['fixed_metrics'][metric];resolved=x<1e-9 and y<1e-9;improved=y<x
                        rr.append({'id':cid,'metric':metric,'error96':x,'error192':y,'admitted':admitted,'both_below_existing_1e_minus9':resolved,'strict_improvement':improved,'numeric_improve_or_resolved':bool(resolved or improved),'qualified_rate_PASS':bool(admitted and(resolved or improved))})
                    expected={'reference_refinement':refinement,'reference_gate_PASS':refgate,'model96_vs_refined':e96,'model192_vs_refined':e192,'rate_rows':rr,'all5_rate_rows_PASS':all(x['qualified_rate_PASS']for x in rr),'same_retirement_presence_and_single_contact_order':same,'PASS':bool(admitted and all(x['qualified_rate_PASS']for x in rr))}
                    self.compare(case,expected,cid+'/all original comparison/admission joins');joins.update({'comparison_available':True,'reference_gate_PASS':refgate,'rate_rows':rr,'scientifically_qualified':expected['PASS']})
                except Exception as error:self.unavailable_case(cid+'/comparisons',error)
            if not joins['comparison_available']:joins['rate_rows']=[{'id':cid,'metric':k,'available':False,'qualified_rate_PASS':False}for k in METRICS]
            self.contexts.append(joins)
            self.compare(aggregate['rows'][len(self.contexts)-1],case,cid+'/aggregate/context saved join')
        self.ok(len(self.histories)==384,'complete384 retained history rows')
        self.ok(len(aggregate['rows'])==96 and aggregate['fixed_rate_rows']==480 and all(len(r['rate_rows'])==5 for r in aggregate['rows']),'all96/480 saved coverage')
        self.ok(sum(len(x['rate_rows'])for x in self.contexts)==480,'all480 retained comparison rows')
        transport=self.js(self.root/'native/native-transport-completion-v1.json');self.ok(transport['acknowledged_nonempty_equations']==self.native_sequence and transport['PASS']is True and transport['child_returncode']==0,'complete saved native lifetime')
        return {'contexts':self.contexts,'histories':self.histories,'checks':self.count,'issues':self.issues,'unavailable':self.unavailable,'native_saved_equations':self.native_sequence,
            'audit_complete':not self.unavailable,'all_saved_arithmetic_consistent':not self.issues and not self.unavailable,
            'all96_scientifically_qualified':bool(len(self.contexts)==96 and all(x['scientifically_qualified']for x in self.contexts)),
            'no_independent_dense_offgrid_trajectory_or_continuous_work_integration_claim':True}

def main(authpath):
    authorization,manifest,producer,closed=gate(authpath)
    need(manifest.get('final_draft_closed')is True,'DRAFT not executable until final reviewed implementation')
    output=P/'audit-results-v1.json';need(not output.exists(),'refusing audit overwrite')
    import numpy as np
    import scipy
    need(np.__version__=='1.26.4'and scipy.__version__=='1.14.1','fixed packages')
    for name,module in(('numpy',np),('scipy',scipy)):
        need(str(Path(module.__file__).resolve().parent)==producer['packages'][name]['root']and module.__version__==producer['packages'][name]['version'],'fixed loaded package '+name)
    audit=Audit(np,manifest,producer,closed)
    try:result=audit.run()
    except Exception as error:audit.unavailable_case('orchestration',error);result={'audit_complete':False,'issues':audit.issues,'unavailable':audit.unavailable,'histories':audit.histories}
    result['package_consistency_is_not_scientific_or_audio_authority']=True
    output.write_text(json.dumps(safe(result),indent=2,allow_nan=False)+'\n')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--authorization',required=True);args=parser.parse_args();main(Path(args.authorization))
