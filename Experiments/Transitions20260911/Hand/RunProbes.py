#!/usr/bin/env python3
"""Run matched old-string contact checks against a baseline and current tree."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--baseline', type=Path, default=ROOT/'build-transitions-20260911/baseline-src')
    parser.add_argument('--candidate', type=Path, default=ROOT)
    parser.add_argument('--build-dir', type=Path, default=ROOT/'build-transitions-20260911/hand-final')
    args = parser.parse_args()
    args.build_dir.mkdir(parents=True, exist_ok=True)
    results = {}
    for label, root in [('baseline',args.baseline.resolve()),('candidate',args.candidate.resolve())]:
        files = ['Source/DSP/ElectryEngine.cpp','Source/DSP/ElectryEngine.h','Tests/ElectryEngineTests.cpp']
        hashes = {name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in files}
        results[label] = {'sources_sha256':hashes}
        flags = ['-std=c++20','-O3','-DNDEBUG','-DELECTRY_DECOUPLED_PICK_RELEASE=1',
                 '-DELECTRY_ENERGY_ATTACK_PITCH=1','-DELECTRY_MEASURED_BODY_RESPONSE=1','-I'+str(root/'Source')]
        for probe in ['Probe','Energy']:
            output=args.build_dir/(label+'-'+probe.lower())
            defines=[]
            if probe=='Probe':
                defines=['-DELECTRY_HAND_TEST_SOURCE="'+str(root/'Tests/ElectryEngineTests.cpp')+'"']
                if label=='candidate': defines+=['-DELECTRY_HAND_FINITE_TEST=1']
            subprocess.run(['clang++',*flags,*defines,str(HERE/(probe+'.cpp')),
                            *(str(root/('Source/DSP/'+name+'.cpp')) for name in ['ElectryEngine','ElectryFx','ElectryVisuals']),
                            '-o',str(output)],check=True)
            run=subprocess.run([str(output)],capture_output=True,text=True)
            (HERE/(label+'-'+probe.lower()+'.log')).write_text(run.stdout+run.stderr)
            results[label][probe.lower()]={'exit_code':run.returncode,'output':run.stdout+run.stderr}
            run.check_returncode()
        if hashes != {name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in files}:
            raise RuntimeError(label+' source changed while probes compiled')
    (HERE/'results.json').write_text(json.dumps(results,indent=2)+'\n')
if __name__=='__main__': main()
