// Pressure-only full-waveform fingerprints; no articulation hand loss.
#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
namespace electry {
struct ElectryEngineTestAccess {
 static std::vector<float> decay(ElectryEngine& e, int string, float handT60, float bend) {
  e.pitchBendSemitones_=bend;
  auto& v=e.voices_[string];
  e.configureSympatheticString(v); v.sympatheticReady=true;
  const double w=6.2831853071795864769*v.lastConfiguredFrequency/e.sampleRate_;
  for(int n=0;n<ElectryEngine::delayLineSize;++n)
   v.vertical.line[n]=.01f*std::sin(w*(n-ElectryEngine::delayLineSize));
  e.sympatheticInjection_=0; e.feedbackDrive_=0;
  e.sympatheticHandGain_=handT60>0?std::pow(10.0f,-3.0f/(handT60*float(e.sampleRate_))):1;
  e.sympatheticHandGainTarget_=e.sympatheticHandGain_;
#if ELECTRY_PERIOD_AWARE_IDLE_HAND
  e.sympatheticHandLossRate_=handT60>0?1.0f/handT60:0;
  e.sympatheticHandLossRateTarget_=e.sympatheticHandLossRate_;
#endif
  std::vector<float> x(int(e.sampleRate_*.45));
  for(int i=0;i<int(x.size());++i) {
   if(i%16==0) e.updateVoiceControl(v);
   ElectryEngine::RenderSums s{}; e.renderSympatheticString(v,s,0);
   x[i]=v.vertical.line[(v.vertical.writeIndex-1)&(ElectryEngine::delayLineSize-1)];
  }
  return x;
 }
 static double rate(const ElectryEngine& e) {return e.sampleRate_;}
}; }
static double rms(const std::vector<float>& x,double sr,double a,double b) {
 double sum=0;for(int i=int(a*sr);i<int(b*sr);++i)sum+=double(x[i])*x[i];
 return std::sqrt(sum/(int(b*sr)-int(a*sr)));
}
int main() {
 std::cout << "host_rate,string,bend_semitones,pressure,frames,checksum,finite,peak\n";
 for(double rate:{44100.,48000.,96000.,192000.})
 for(int string:{0,3,7}) for(float bend:{-2.f,0.f,2.f})
 for(float pressure:{0.f,.3f,1.f}) {
  auto e=std::make_unique<electry::ElectryEngine>(); e->prepare(rate,256);
  electry::EngineParameters p; p.sympatheticAmount=.2f; p.palmMute=pressure;
  e->setParameters(p); e->reset();
  const auto x=electry::ElectryEngineTestAccess::decay(*e,string,0,bend);
  std::uint64_t hash=14695981039346656037ull;
  bool finite=true; float peak=0;
  for(float value:x) {
   const auto bits=std::bit_cast<std::uint32_t>(value);
   for(int shift=0;shift<32;shift+=8) {hash^=(bits>>shift)&255u; hash*=1099511628211ull;}
   finite=finite&&std::isfinite(value); peak=std::max(peak,std::abs(value));
  }
  std::cout<<std::setprecision(9)<<rate<<','<<string<<','<<bend<<','<<pressure
   <<','<<x.size()<<','<<hash<<','<<finite<<','<<peak<<'\n';
 }
}
