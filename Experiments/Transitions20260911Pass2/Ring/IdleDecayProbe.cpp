#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>
namespace electry {
struct ElectryEngineTestAccess {
 static std::vector<float> decay(ElectryEngine& e, int string, float handT60) {
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
 std::cout<<"host_rate,string,hand_t60,early_rms,late_rms,late_db_relative_open,measured_added_t60\n";
 for(double sr:{44100.,48000.,96000.,192000.})for(int s:{0,3,7}) {
  auto e=std::make_unique<electry::ElectryEngine>();e->prepare(sr,256);
  electry::EngineParameters p;p.sympatheticAmount=.2f;e->setParameters(p);e->reset();
  const auto open=electry::ElectryEngineTestAccess::decay(*e,s,0);
  const double rate=electry::ElectryEngineTestAccess::rate(*e);
  const double openRms=rms(open,rate,.19,.29);
  for(float handT60:{.12f,1.6f}) {
   e->reset();const auto x=electry::ElectryEngineTestAccess::decay(*e,s,handT60);
   auto late=rms(x,rate,.19,.29);
   double sumT=0,sumY=0,sumTT=0,sumTY=0;int count=0;
   for(double t=.05;t<.25;t+=.01) {
    double y=20*std::log10(rms(x,rate,t,t+.04)/rms(open,rate,t,t+.04));
    sumT+=t;sumY+=y;sumTT+=t*t;sumTY+=t*y;++count;
   }
   const double slope=(count*sumTY-sumT*sumY)/(count*sumTT-sumT*sumT);
   const double measured=-60/slope;
   std::cout<<std::setprecision(9)<<sr<<','<<s<<','<<handT60<<','<<rms(x,rate,.04,.10)<<','<<late<<','<<20*std::log10(late/openRms)<<','<<measured<<'\n';
  }
 }
}
