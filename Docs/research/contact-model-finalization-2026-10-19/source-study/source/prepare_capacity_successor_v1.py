"""Administrative successor after a closed storage interruption; stdlib only.

Copy all frozen I/O-successor artifacts exactly. Preserve its scientific
protocol byte for byte, including the historical provenance section. Only
the new manifest's directory, administrative lineage and metadata change.
No arrays, numerical packages, solvers, sample hashes or audio are read here.
"""
if not __debug__:
    raise SystemExit('Optimized Python forbidden')
from pathlib import Path
import ast
import copy
import datetime
import hashlib
import json
import os
import shutil
import subprocess
import sys

P = Path(__file__).resolve().parent
R = P.parent
OLD = R / 'build-passive-source-tone-candidate-iofix'
MH = '5c7a7fdbdf894367e0dedf72cfb48f568defb3474b20f821a2c4de87dbddc97f'
PH = '7d6c94db920cd607a0fccdfe05cb236cf65d9e8a9b63adf75d5a11a2fc138058'
CLOSED = 'build-passive-source-tone-candidate-iofix-root-review/execution-receipt-v1.json'
CLOSED_SHA = 'fee2b2ad3dde54a02c554fdd45e55bc7189fb01f575fb1a573f80999a13276a4'


def require(value, message):
    if not value:
        raise RuntimeError(message)


def fact(path):
    path = Path(path)
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    return {'bytes': path.stat().st_size, 'sha256': digest}


def read(path):
    return json.loads(Path(path).read_text())


def write(path, value):
    with Path(path).open('x') as stream:
        stream.write(json.dumps(value, indent=2, allow_nan=False) + '\n')


def main():
    require(P.parent == OLD.parent and P != OLD, 'fresh immediate sibling required')
    require(not (P / 'implementation-manifest-v1.json').exists(), 'already prepared')
    require(not (P / 'run-v1').exists(), 'fresh output directory required')
    require(fact(OLD / 'implementation-manifest-v1.json')['sha256'] == MH, 'old manifest')
    require(fact(OLD / 'protocol-selected-v1.json')['sha256'] == PH, 'old protocol')
    require(fact(R / CLOSED)['sha256'] == CLOSED_SHA, 'closed interrupted attempt')
    closed = read(R / CLOSED)
    require(closed['run_closed'] and closed['process_exit_code'] == -2
            and not closed['completed'] and closed['post_static_input_bindings_PASS'],
            'preserve incomplete original result')
    old = read(OLD / 'implementation-manifest-v1.json')
    require(str(Path(sys.executable).resolve()) == old['python_real_executable']
            and sys.version == old['python_version'], 'fixed Python312')
    for name, value in old['inherited_environment'].items():
        require(os.environ.get(name) == value, 'fixed inherited environment ' + name)
    require(not subprocess.check_output(['git', 'diff', old['immutable_source_commit'],
            '--', 'Source', 'Tests', 'CMakeLists.txt'], cwd=R), 'production unchanged')
    for name, expected in old['artifacts'].items():
        source, target = OLD / name, P / name
        require(not source.is_symlink() and fact(source) == expected, 'old artifact ' + name)
        require(not target.exists(), 'exclusive artifact copy ' + name)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        require(fact(target) == expected, 'exact copied artifact ' + name)
        if target.suffix == '.py':
            ast.parse(target.read_text(), filename=str(target))
    for name, expected in old['sample_bindings'].items():
        require((R / name).stat().st_size == expected['bytes'], 'sample STAT ' + name)
    cleanup_path = 'build-authorized-cleanup-20261019/cleanup-summary-v1.json'
    cleanup = read(R / cleanup_path)
    require(cleanup['cleanup_completed'] and not cleanup['binding_failures']
            and cleanup['obsolete_worktrees_removed'] == 25, 'capacity changed by cleanup')
    lineage_paths = [
        str((OLD / 'implementation-manifest-v1.json').relative_to(R)),
        str((OLD / 'preparation-receipt-v1.json').relative_to(R)), CLOSED,
        'build-passive-source-tone-candidate-iofix-root-review/administrative-storage-watchdog-receipt-v1.json',
        'build-passive-source-tone-candidate-iofix/run-v1/study/emergency-original-error-and-attempt.json',
        cleanup_path,
    ]
    lineage = {name: fact(R / name) for name in lineage_paths}
    plan = {
        'schema': 'electry-passive-source-capacity-successor/v1',
        'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'reason': 'Actual storage interrupted the prior attempt; user-authorized cleanup changed available capacity.',
        'predecessor_root': str(OLD.relative_to(R)), 'predecessor_manifest_sha256': MH,
        'unchanged_scientific_protocol_sha256': PH,
        'predecessor_remains_incomplete_and_unqualified': True,
        'all_old_artifacts_copied_byte_identically': len(old['artifacts']),
        'same_full_ordered96_cases384_histories480_rate_rows': True,
        'no_reuse_or_skip_of11_previously_completed_cases': True,
        'no_numerical_parameter_gate_cohort_model_or_IO_code_changes': True,
        'same12GiB_budget256MiB_emergency_reserve': True,
        'source_failure_policy_remains_in_force': True,
        'historical_IO_provenance_is_retained_unchanged': True,
        'lineage': lineage,
        'actual_free_bytes_at_preparation': shutil.disk_usage(R).free,
        'fresh_exact_root_peer_reviews_and_execution_authorization_required': True,
        'source_states_fields_PCM_tone_or_production_authorized': False,
    }
    write(P / 'capacity-successor-plan-v1.json', plan)
    m = copy.deepcopy(old)
    m['utc'] = plan['utc']
    m['sys_path'][0] = str(P)
    m['artifacts'].update({name: fact(P / name) for name in (
        'prepare_capacity_successor_v1.py', 'capacity-successor-plan-v1.json')})
    m['inputs'].update(lineage)
    m['administrative_capacity_successor'] = {
        'plan_path': 'capacity-successor-plan-v1.json',
        'plan_sha256': fact(P / 'capacity-successor-plan-v1.json')['sha256'],
        'selected_metadata_preparer': 'prepare_capacity_successor_v1.py',
        'same_protocol_whole_file_sha256': PH,
        'old_attempt_preserved_closed_incomplete': True,
    }
    allowed = {'utc', 'sys_path', 'artifacts', 'inputs', 'administrative_capacity_successor'}
    require(all(m[key] == value for key, value in old.items() if key not in allowed),
            'all other fixed facts unchanged')
    require(m['sys_path'][1:] == old['sys_path'][1:], 'fixed package lookup tail')
    require(fact(P / 'protocol-selected-v1.json')['sha256'] == PH, 'same whole protocol')
    write(P / 'implementation-manifest-v1.json', m)
    receipt = {
        'schema': 'electry-passive-source-preparation/v1',
        'manifest_sha256': fact(P / 'implementation-manifest-v1.json')['sha256'],
        'protocol_sha256': PH, 'metadata_AST_only': True,
        'parameter_NPZ_and_coefficient_archive_STAT_only': True,
        'counts': {**{key: len(m[key]) for key in (
            'artifacts', 'inputs', 'sample_bindings', 'environment_files', 'Source24')},
            'contexts': len(m['all96_case_ids'])},
        'states_authorized': False,
    }
    write(P / 'preparation-receipt-v1.json', receipt)
    print(json.dumps(receipt), flush=True)


if __name__ == '__main__':
    main()
