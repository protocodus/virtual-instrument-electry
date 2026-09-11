#include "DSP/ElectryEngine.h"
#include <array>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
namespace electry {
struct ElectryEngineTestAccess {
  static void state(const ElectryEngine& e, int index, double time) {
    const auto& v=e.voices_[index]; const auto& l=v.vertical;
    std::cout << time << ',' << index << ',' << v.livePalmContact << ',' << v.livePalmPressure << ',' << l.handLossSolvedDepth << ',' << l.handLossDepth << ',' << l.handEnvelope << ',' << l.handEnvelopePeak << ',' << int(v.excitationPhase) << ',' << v.excitationRemaining << ',' << v.attackPitchTensionRatio << '\n';
  }
}; }
using namespace electry;
static std::vector<float> render(ElectryEngine& e, int samples, int block=1) {
  std::vector<float> result(samples),right(samples);
  for(int at=0;at<samples;at+=block)e.process(result.data()+at,right.data()+at,std::min(block,samples-at));
  return result;
}
int main(int argc,char** argv) {
 if(argc!=2)return 2; std::filesystem::create_directories(argv[1]);
 for(int note:{28,40,55,62,67}) for(int scenario=0;scenario<4;++scenario){
  int index=note==28?0:note==40?2:note==55?5:note==62?6:7;
  auto e=std::make_unique<ElectryEngine>(); EngineParameters p;applyGuitarBuild(p,defaultGuitarBuild);
  p.sympatheticAmount=0;p.strumSpreadSeconds=0;p.fingerNoise=0;p.pickNoise=0;p.artifactAmount=0;p.outputGain=1;
  p.muteDamping=.72f;p.pickHardness=.72f;p.stringAge=.15f;p.velocityAmount=.7f;
  e->prepare(48000,256);e->setParameters(p);e->reset();e->setSoloStringMask(1<<index);
  e->noteOn(ElectryEngine::firstPlayStyleKeyswitchNote+int(scenario==2?PlayStyle::Sustain:PlayStyle::PalmMute),1);
  e->noteOn(note,.8f);auto output=render(*e,19200,256);
  std::cout<<"case,"<<note<<','<<scenario<<'\n';ElectryEngineTestAccess::state(*e,index,-.00001);
  if(scenario==0||scenario==1)e->noteOn(ElectryEngine::firstPlayStyleKeyswitchNote,1);
  if(scenario!=1)e->noteOn(ElectryEngine::firstRepickNote+index,.8f);
  else { e->setSoloStringMask(1<<((index+1)%8));constexpr int openNotes[]={28,35,40,45,50,55,59,64};e->noteOn(openNotes[(index+1)%8],.01f); }
  ElectryEngineTestAccess::state(*e,index,0);
  for(int j=0;j<3840;++j){auto a=render(*e,1);output.push_back(a[0]);if(j<144||j%48==0)ElectryEngineTestAccess::state(*e,index,(j+1)/48000.0);}
  auto tail=render(*e,10560,256);output.insert(output.end(),tail.begin(),tail.end());
  std::string path=std::string(argv[1])+"/"+std::to_string(note)+"_"+std::to_string(scenario)+".f32";std::ofstream out(path,std::ios::binary);out.write((char*)output.data(),output.size()*4);
 }
}
