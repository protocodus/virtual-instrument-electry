"""UNEXECUTED DRAFT. Functions require a future authenticated caller.

No module-level numerical imports, array reads, coefficient or source calls.
"""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
import gzip,hashlib,json,math

K=6000.0
PHYSICAL_RATE=96000

def require(ok,message):
    if not ok:raise RuntimeError(message)

def file_sha(path):
    h=hashlib.sha256()
    with Path(path).open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
    return h.hexdigest()

def load_literal_tables(archive,expected_sha,np):
    """Read only after a future gate binds the qualified archive and review.

    Keep the small required coefficient arrays, never all parsed row objects.
    Every mode/axis/h lookup remains literal; no coefficient evaluator is run.
    """
    require(file_sha(archive)==expected_sha,'qualified archive identity')
    tables={};count=0
    with gzip.open(archive,'rt') as f:
        for line in f:
            row=json.loads(line);count+=1
            require(row['PASS'] is True and row['complete'] is True,'unqualified coefficient row')
            s=row['specification']
            if s['kind']!='actual_C':continue
            key=(s['case_id'],s['axis'],int(s['n']),float(s['h']).hex())
            require(key not in tables,'duplicate literal coefficient key')
            a,h=float(s['alpha']),float(s['h'])
            tables[key]={'specification':s,'id':row['id'],
                'Psi':np.asarray(row['transition'],dtype=float),
                'bX':np.asarray(row['force_response'],dtype=float),
                'KX':np.asarray([row['D_operator'],2*a*h*np.asarray(row['W16']),
                                  2*a*h*np.asarray(row['W32'])],dtype=float)}
    require(count==70544 and len(tables)==68240,'complete qualified table coverage')
    return tables

def prepare_case(repo,case,preflight_row,literal_tables,np):
    """All96 contexts are supplied by the immutable C protocol, never searched.

    Read n/Omega/alpha and the original pulse/spatial/force-shape only. q0 is
    deliberately not an input: the fresh source starts at rest. Incoming repick
    state is owned by attach() rather than this input loader.
    """
    require(case['id']==preflight_row['id'],'C case join')
    c=preflight_row['contact'];require(c['internal_rate']==PHYSICAL_RATE,'fixed source clock')
    metadata_path=Path(repo)/case['artifacts']['old_geometry_metadata']['path']
    require(file_sha(metadata_path)==case['artifacts']['old_geometry_metadata']['sha256'],'source metadata identity')
    metadata=json.loads(metadata_path.read_text())
    # Both fields are direct captured velocity-profile amplitudes. No F/stroke
    # division, legacy source gain or output-level correction enters Fq.
    amplitude=(float(c['velocity_amplitude']) if case['scope']=='neck'
               else float(metadata['profile_amplitude']))
    require(math.isfinite(amplitude) and amplitude>=0,'captured velocity amplitude')
    parameter_path=Path(repo)/'build-inherited-field-C'/preflight_row['parameter_file']
    require(file_sha(parameter_path)==preflight_row['parameter_sha256'],'parameter identity')
    with np.load(parameter_path,allow_pickle=False) as z:
        data={name:z[name].copy() for name in ('n','omega','alpha','force_shape','spatial','pulse')}
    n,omega,alpha=(data[k] for k in ('n','omega','alpha'))
    require(alpha.shape==(2,len(n)) and omega.shape==n.shape,'two-axis mode ownership')
    require(np.isfinite(alpha).all() and np.all(alpha>=0) and np.all(omega>0),'damping domain')
    raw=np.asarray([c['vertical_weight'],c['horizontal_weight']],dtype=float)
    length=math.sqrt(float(raw@raw));require(length>0,'nonzero existing stroke direction')
    normal=raw/length
    L,T,mu,a=[float(c[k]) for k in ('L_m','T_N','mu_kg_m','pick_fraction')]
    require(0<a<1 and L>0 and T>0 and mu>0,'source physical geometry')
    Fq=4.8*amplitude;D=Fq*(L*a*(1-a)/T+1/K);mass=mu*L/2
    polarity=float(c['polarity']);require(polarity in (-1.0,1.0),'captured signed stroke orientation')
    B=polarity*normal[:,None]*data['force_shape'][None,:]
    require(np.array_equal(data['force_shape'],np.sin(n*math.pi*a)),'actual frozen source point')
    start,end=int(preflight_row['first_contact_index']),int(preflight_row['first_release_index'])
    require(0<=start<end<int(preflight_row['calls']),'actual integer Contact/Release clocks')
    require(int(c['release_length'])==len(data['pulse']) and float(data['pulse'][0])==0,'original normalized release pulse')
    tables={}
    for rate in (96000,192000):
        h=1/rate;axis_tables=[]
        for ai,axis in enumerate(('v','h')):
            rows=[literal_tables[case['id'],axis,int(mode),h.hex()] for mode in n]
            for j,row in enumerate(rows):
                s=row['specification']
                require(float(s['mass']).hex()==mass.hex() and float(s['omega']).hex()==float(omega[j]).hex()
                        and float(s['alpha']).hex()==float(alpha[ai,j]).hex(),'literal modal coordinate bits')
            psi=np.asarray([r['Psi'] for r in rows]);bx=np.asarray([r['bX'] for r in rows])
            axis_tables.append({'phi11':psi[:,0,0],'phi12':psi[:,0,1]/omega,
                'phi21':psi[:,1,0]*omega,'phi22':psi[:,1,1],
                'gamma_q':bx[:,0]/omega,'gamma_v':bx[:,1],
                'KX':np.asarray([r['KX'] for r in rows]),'source_row_ids':[r['id'] for r in rows]})
        tables[h.hex()]={name:np.stack([r[name] for r in axis_tables])
            for name in ('phi11','phi12','phi21','phi22','gamma_q','gamma_v','KX')}
        tables[h.hex()]['source_row_ids']=[r['source_row_ids'] for r in axis_tables]
    # Captured sign belongs to the physical actuator exactly once. Observation
    # retains fixed axis sensitivity and coil geometry, never a second sign.
    observer=(np.asarray([.85,.35])/.040)[:,None]*data['spatial'][None,:]
    return {'id':case['id'],'case':case,'source_metadata':metadata,'contact':c,
        'n':n,'omega':omega,'alpha':alpha,'mass':mass,'B':B,'style_normal':normal,'signed_normal':polarity*normal,'captured_polarity':polarity,
        'Fq_N':Fq,'D_m':D,'source_fraction':a,'source_x_m':L*a,
        'contact_frame96':start,'release_frame96':end,'calls96':int(preflight_row['calls']),
        'pulse':data['pulse'],'release_coefficient':float(c['release_coefficient']),
        'observer':observer,'tables':tables,'parameter_sha256':preflight_row['parameter_sha256'],
        'no_legacy_stroke_gain_or_static_q0':True}

if __name__=='__main__':raise SystemExit('DRAFT only; no input or source execution authorized')
