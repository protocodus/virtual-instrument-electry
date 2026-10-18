#!/usr/bin/env python3
"""Observe actual upper-string losses and independently solve damped poles.

No recordings or fitted coefficients are used. A tiny C++ friend observer reads
static Sustain coefficients from production, at zero attack tension, with body,
sympathetic and artifact paths off. The Python complex-pole oracle includes the
actual cubic read, eight allpasses, original one-pole, intrinsic correction and
hand/material biquads. Horizontal detuning and two-axis/bridge coupling are
omitted, so these are isolated vertical loop poles, not the whole instrument.
A pickup is an observation filter, not part of this feedback loss measurement.

The fitted unit-circle traversal gain and actual pole T60 are separate outputs:
dispersive group delay changes their relationship. The weak-damping explanation
is Smith, Physical Audio Signal Processing, Digital Waveguide Theory:
https://www.dsprelated.com/freebooks/pasp/Digital_Waveguide_Theory.html .
The exact root calculation is implemented independently in AuditModalDecay.py.

Run the same frozen source twice with --define
ELECTRY_BUTTERWORTH_LOSS_SHELF=0 and =1. Other shipping feature definitions are
fixed below; --define may override them. No optional Python dependencies.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import subprocess
import tempfile

import AuditDispersion as dispersion
import AuditModalDecay as decay


def observe(root, compiler, definitions):
    source = dispersion.OBSERVER.split('int main(int argc, char**)')[0]
    source = source.replace('int stringIndex, int fret, float bend)',
                            'int stringIndex, int fret, float bend, float age)')
    source = source.replace('p.stringGauge = gauge; p.scaleLength = scale;',
                            'p.stringGauge = gauge; p.scaleLength = scale; p.stringAge = age;')
    source = source.replace('engine->pitchBendSemitones_ = bend;',
                            'engine->pitchBendSemitones_ = bend; v.attackPitchTensionRatio = 0;')
    source = source.replace('<< ",\\\"gauge\\\":" << gauge',
                            '<< ",\\\"age\\\":" << age << ",\\\"gauge\\\":" << gauge')
    source += r'''
int main() {
    std::cout << std::setprecision(17) << '['; bool first=true;
    for(double rate : {48000.,96000.,384000.})
    for(float age : {0.f,.3f,1.f})
    for(auto cell : {std::pair<int,int>{2,0},{2,7},{4,5},{5,0},{7,15}}) {
        if(!first)std::cout<<',';first=false;
        electry::ElectryEngineTestAccess::emit(rate,1.f,.85f,cell.first,cell.second,0.f,age);
    }
    std::cout << "]\n";
}
'''
    with tempfile.TemporaryDirectory(prefix='electry-upper-loss-') as directory:
        directory = Path(directory)
        cpp, binary = directory/'observe.cpp', directory/'observe'
        cpp.write_text(source)
        command = [compiler,'-std=c++17','-O2']+['-D'+d for d in definitions]+[
            '-I',str(root/'Source'),str(cpp),str(root/'Source/DSP/ElectryEngine.cpp'),'-o',str(binary)]
        subprocess.run(command,check=True)
        rows = json.loads(subprocess.check_output([str(binary)],text=True))
    return rows, hashlib.sha256(source.encode()).hexdigest()


def analyze(row):
    anchors = []
    for hz in (row['f0'],1500,2000,2500,3000,3600,4000,6000,8000):
        omega = 2*math.pi*hz/row['rate']
        transfer, derivative = decay.transfer_and_log_derivative(row,complex(0,omega))
        logarithm = math.log(abs(transfer))
        period_t60 = -3*math.log(10)/(row['f0']*logarithm)
        anchors.append({'frequency_hz':hz,'traversal_magnitude':abs(transfer),
            'fundamental_period_mapping_t60_seconds':period_t60,
            'group_delay_t60_seconds':-3*math.log(10)*(-derivative.real)/(row['rate']*logarithm),
            'group_delay_over_fundamental_period':-derivative.real*row['f0']/row['rate'],
            'cubic_log_loss_share':math.log(dispersion.cubic_read_response(row['delay'],omega)[1])/logarithm})
    modes = []
    for n in range(1,128):
        mode = decay.solve_pole(row,n)
        if mode is None or mode['pole_frequency_hz']>8200:
            break
        modes.append(mode)
    nearest = {str(hz):min(modes,key=lambda m:abs(m['pole_frequency_hz']-hz))
        for hz in (row['f0'],1500,2000,2500,3000,3600,4000,6000,8000)}
    return {**row,'anchors':anchors,'nearest_modes':nearest,'modes':modes}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root',type=Path,default=Path(__file__).resolve().parents[1])
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--define',action='append',default=[])
    args=parser.parse_args()
    definitions=dict(d.split('=') for d in dispersion.DEFAULT_DEFINES)
    definitions.update({'ELECTRY_REFERENCE_STRING_MASS':'1','ELECTRY_MODAL_DISPERSION_FIT':'1',
        'ELECTRY_BUTTERWORTH_LOSS_SHELF':'0','ELECTRY_INTRINSIC_LOSS_FRESH':'1',
        'ELECTRY_INTRINSIC_LOSS_LOWPASS':'0'})
    definitions.update(d.split('=') for d in args.define)
    definitions=[k+'='+v for k,v in definitions.items()]
    rows, observer_hash=observe(args.source_root.resolve(),args.compiler,definitions)
    results=[analyze(row) for row in rows]
    provenance=dispersion.provenance(args.source_root,args.compiler,definitions)
    provenance.update({'observer_sha256':observer_hash,
        'audit_script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'modal_decay_script_sha256':hashlib.sha256(Path(decay.__file__).read_bytes()).hexdigest()})
    summary={'observations':len(results),'poles':sum(len(row['modes']) for row in results),
        'maximum_pole_residual':max(m['residual'] for row in results for m in row['modes'])}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps({'schema':'electry-upper-string-loss/v1','provenance':provenance,
        'summary':summary,'observations':results},indent=2,allow_nan=False)+'\n')
    print(json.dumps(summary,indent=2))


if __name__=='__main__':
    main()
