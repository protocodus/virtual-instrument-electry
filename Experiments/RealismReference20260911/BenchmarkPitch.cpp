#include "DSP/ElectryEngine.h"
#include <array>
#include <chrono>
#include <iostream>
int main() {
 constexpr int rate=96000, block=256, total=rate*4;
 constexpr std::array<int,8> notes{28,35,40,45,50,55,59,64};
 std::cout << "{";
 for(int mode=0; mode<2; ++mode) {
  electry::ElectryEngine engine; engine.prepare(rate,block);
  electry::EngineParameters p; p.pickupSelector=electry::PickupSelector::Both;
  p.outputMode=electry::OutputMode::Stereo; engine.setParameters(p); engine.reset();
  std::array<float,block> left{},right{};
  for(int note:notes) engine.noteOn(note,.9f);
  engine.process(left.data(),right.data(),block);
  double checksum=0;
  auto start=std::chrono::steady_clock::now();
  for(int i=0;i<total;) {
   if(mode==1 && i>0 && i%(rate/4)==0) {
    for(int note:notes) engine.noteOff(note);
    for(int note:notes) engine.noteOn(note,.9f);
   }
   int n=std::min({block,total-i,rate/4-i%(rate/4)});
   engine.process(left.data(),right.data(),n);checksum+=left[0]+right[0];i+=n;
  }
  auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  if(mode) std::cout << ",";
  std::cout << "\"" << (mode?"repicked":"held") << "\":{\"seconds\":" << elapsed
            << ",\"cpu_ratio\":" << elapsed/4 << ",\"checksum\":" << checksum << "}";
 }
 std::cout << "}\n";
}
