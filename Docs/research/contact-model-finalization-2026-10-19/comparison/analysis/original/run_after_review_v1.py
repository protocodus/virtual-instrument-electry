"""UNEXECUTED one separately authorized metrics-only run, no renderer/fields."""
if not __debug__:raise RuntimeError('Optimized Python is forbidden')
from pathlib import Path
import argparse,json,traceback
from gate_v1 import require_review
P=Path(__file__).resolve().parent

def main(authpath,expected):
    m,auth=require_review(authpath,expected)
    # Before these imports no numeric library/sample/source/window was accessed.
    import numpy as np,scipy
    if np.__version__!=m['numpy_version']or scipy.__version__!=m['scipy_version']:raise RuntimeError('fixed numeric versions changed')
    for name,module in(('numpy',np),('scipy',scipy)):
        if str(Path(module.__file__).resolve().parent)!=m['package_roots'][name]:raise RuntimeError('fixed numeric root changed')
    from scipy.signal.windows import blackmanharris
    from context_v1 import load
    from analyze_neck_v1 import run as neck
    from analyze_bridge_v1 import run as bridge
    try:
        context=load(np,auth)
        for name,function,args in(('neck-analysis-v1.json',neck,(context,np,blackmanharris)),('bridge-analysis-v1.json',bridge,(context,np))):
            value=function(*args);path=P/name
            with path.open('x')as out:out.write(json.dumps(value,allow_nan=False,separators=(',',':'))+'\n')
    except BaseException as error:
        path=P/'incomplete-analysis-v1.json'
        if not path.exists():path.write_text(json.dumps({'complete':False,'failure':repr(error),'traceback':traceback.format_exc(),'no_retry_masks_gains_or_physical_changes':True},indent=2)+'\n')
        raise

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--authorization',type=Path,required=True);parser.add_argument('--authorization-sha256',required=True);args=parser.parse_args();main(args.authorization,args.authorization_sha256)
