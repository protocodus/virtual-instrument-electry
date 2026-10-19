"""Unexecuted all96 input/scaling/path proof, before any physical state calls."""
if not __debug__:raise SystemExit('Optimized Python forbidden')
from pathlib import Path
import json,math
from fractions import Fraction
from source_inputs_v1 import prepare_case,load_literal_tables,require
from support_path_v1 import SupportPath

def run(manifest,np,signal,save_record,save_arrays):
    root=Path(__file__).resolve().parent.parent
    C=json.loads((root/manifest['C_protocol_path']).read_text());pre=json.loads((root/manifest['C_preflight_path']).read_text())
    tables=load_literal_tables(root/manifest['coefficient_rows_path'],manifest['coefficient_rows_sha256'],np)
    cases=[];rows=[];bound=Fraction(*manifest['fixed_native_physical_lambda_pair'])
    for case,old in zip(C['all96_cases'],pre['rows']):
        c=prepare_case(root,case,old,tables,np)
        require(c['Fq_N']>0 and c['D_m']>0,'fixed fresh-source characteristic normalization')
        path=SupportPath(c,0.,0.,np,signal);Arows=[]
        for rate in(96000,192000):
            tab=c['tables'][(1/rate).hex()];A=float(np.sum(c['B']*tab['gamma_q']*c['B']))
            lam=Fraction.from_float(A)*3000
            require(math.isfinite(A) and A>=0 and lam<=bound,'actual source contraction bound')
            Arows.append({'rate':rate,'A_m_per_N':A,'lambda_exact':[lam.numerator,lam.denominator]})
        char=.5*c['Fq_N']*c['D_m'];S=max(math.sqrt(2*char),1e-12);workcoord=max(char,1e-30)
        row={'id':c['id'],'scope':case['scope'],'old_key':case['old_key'],'first_clock':c['source_metadata']['first_clock'],
            'calls96':c['calls96'],'contact_frame96':c['contact_frame96'],'release_frame96':c['release_frame96'],
            'Fq_N':c['Fq_N'],'D_m':c['D_m'],'initial_stored_energy_J':0.,'characteristic_not_initial_energy_J':char,
            'state_scale_sqrtJ':S,'work_D_ODE_coordinate_scale_J':workcoord,
            'first_rtol':2e-10,'first_state_absolute_atol':2e-13*S,'first_work_D_absolute_atol_J':2e-13*workcoord,
            'refined_rtol':5e-12,'refined_state_absolute_atol':5e-15*S,'refined_work_D_absolute_atol_J':5e-15*workcoord,
            'style_normal':c['style_normal'].tolist(),'signed_normal':c['signed_normal'].tolist(),
            'physical_source_x_m':c['source_x_m'],'physical_source_fraction':c['source_fraction'],
            'parameter_sha256':c['parameter_sha256'],'actual_mobility':Arows,
            'coefficient_row_ids':{str(rate):c['tables'][(1/rate).hex()]['source_row_ids']for rate in(96000,192000)},
            'legacy_renderer_force_metadata_is_not_actual_E_force':True,'all_inputs_preflight_PASS':True}
        arrays={'n':c['n'],'omega':c['omega'],'alpha':c['alpha'],'B':c['B'],'observer':c['observer'],
            'pulse':c['pulse'],'withdrawal_rate':path.rate,'withdrawal_R':path.R,'withdrawal_z':path.z,
            'withdrawal_knots_s':path.knots,'withdrawal_slopes':path.slopes}
        for rate in(96000,192000):
            for key,value in c['tables'][(1/rate).hex()].items():
                if key!='source_row_ids':arrays[str(rate)+'_'+key]=value
        save_arrays(c['id'].replace('/','--'),**arrays);rows.append(row);cases.append(c)
    require(len(cases)==96,'all96 preflight required before any state')
    save_record('preflight-receipt-v1',{'all96_inputs_PASS':True,'all96_absolute_tolerances_frozen_before_state_or_force_calls':True,'rows':rows})
    return cases,rows

if __name__=='__main__':raise SystemExit('DRAFT only; no source input computation authorized')
