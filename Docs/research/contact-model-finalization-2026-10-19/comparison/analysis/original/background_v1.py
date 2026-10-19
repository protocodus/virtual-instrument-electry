"""UNEXECUTED candidate-only measured pre-Contact background policy.

No numerical module imports, PCM reads or runtime measurements on import.
Existing observer/guard functions are injected after separate authorization.
"""
if not __debug__:raise RuntimeError('Optimized Python is forbidden')
import math


def candidate_background(pcm,metadata,np):
    """Same2x-clock immutable driver, whole host frame at Contact excluded.

    Drivers start engineClock=0 and render all2s without trimming. At internal
    clock c=2*h+p+1, any Contact in host h excludes that entire host frame.
    Use every available preceding PCM sample, never an onset search or zero
    inferred from the planned modal state. The short deterministic model prefix
    is NOT a real-source >=10ms intake witness. No new duration/noise threshold.
    """
    facts={'scope':'candidate E only; old A/C/D backgrounds/guards untouched',
           'available':False,'real_source_10ms_intake_satisfied_by_this_claim':False}
    try:
        first=metadata['first_clock'];index=metadata['first_contact_index'];calls=metadata['calls']
        if not(all(type(x)is int for x in(first,index,calls))and first>=1 and 0<=index<calls):
            raise ValueError('missing/noninteger actual callback/Contact clock')
        clock=first+index;stop=(clock-1)//2
        facts.update({'internal_rate':96000,'output_rate':48000,'Contact_engine_clock':clock,
                      'first_clock':first,'first_contact_index':index,'calls':calls,
                      'frame0_engine_clock_origin':1,'whole_contact_host_frame_excluded':stop,
                      'frames_half_open':[0,stop],'frames':stop,'duration_s':stop/48000,
                      'algorithm':'existing observer on actual finite pre-Contact PCM, no new minimum'})
        if len(pcm)!=96000 or not(0<stop<=len(pcm)):
            raise ValueError('no nonempty prefix in unchanged2s crop')
        prefix=pcm[:stop]
        if not np.isfinite(prefix).all():raise ValueError('nonfinite actual candidate prefix')
        facts.update({'available':True,'measured_raw_rms':float(np.sqrt(np.mean(prefix*prefix))),
                      'measured_raw_peak':float(np.max(abs(prefix))),
                      'exact_zero_sample_count':int(np.sum(prefix==0)),
                      'no_zero_background_inference_from_source_plan':True})
        return prefix,facts
    except Exception as error:
        facts['unavailable_reason']=repr(error)
        return None,facts


def unavailable_upper(guard,facts):
    """Keep power/nuisance/taper facts, but missing background never admits.

    Caller computed the existing helper with zero placeholder only to retain
    its non-background descriptive fields; placeholder is never a measurement.
    """
    for row in guard.values():
        row['preonset_background_power']=None;row['background_margin_db']=None
        row['observable']=False;row['candidate_background_available']=False
        row['guard_unavailable_reason']=facts.get('unavailable_reason','background helper unavailable')
    return guard

if __name__=='__main__':raise SystemExit('DRAFT ONLY; no sample or metric execution authorized')
