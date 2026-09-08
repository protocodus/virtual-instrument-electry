#include "DSP/ElectryFx.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace electry;
int main(int argc,char** argv) {
 if(argc!=2) return 2;
 std::ofstream out(argv[1],std::ios::binary);
 for(int mode=0;mode<5;++mode) {
  constexpr int rate=48000, frames=rate*2,block=127;
  ElectryFx fx; fx.prepare(rate);
  std::vector<float> left(frames),right(frames);
  for(int i=0;i<frames;++i) {
   left[i]=static_cast<float>(.25*std::sin(i*.13)+.125*std::sin(i*.037));
   right[i]=left[i];
   if((i>=23417&&i<31971)||(i>=42193&&i<51197)||(i>=75017&&i<79439))
    right[i]=static_cast<float>(.17*std::sin(i*.14));
   if(mode==2 && i<18000) { left[i]=0.;right[i]=i>=3197?-0.f:0.f; }
   if(mode==3) { left[i]=(i%2)?-0.f:0.f;right[i]=(i%3)?-0.f:0.f; }
   if(i==11117) {left[i]=std::numeric_limits<float>::quiet_NaN();right[i]=std::numeric_limits<float>::infinity();}
  }
  for(int offset=0;offset<frames;) {
   int count=std::min(block,frames-offset);
   // Split at explicit reset/prepare points; input divergence itself remains
   // deliberately inside processing blocks.
   if(offset<36001) count=std::min(count,36001-offset);
   if(offset<60013) count=std::min(count,60013-offset);
   if(offset==36001&&mode==4) fx.reset();
   if(offset==60013&&mode==4) fx.prepare(96000.);
   const int stage=offset/12000;
   FxParameters parameters {.75f,.9f,static_cast<AmpModel>(stage%3),.5f,.4f,.5f};
   if(mode==1&&(stage==1||stage==4)) parameters={};
   if(mode==3) parameters={};
   if(stage==2) parameters.distortion=0;
   if(stage==5) parameters.amp=0;
   fx.setParameters(parameters);
   fx.process(left.data()+offset,right.data()+offset,count);
   offset+=count;
  }
  for(const auto* values:{&left,&right}) {
   for(float value:*values) if(!std::isfinite(value)) throw std::runtime_error("nonfinite");
   out.write(reinterpret_cast<const char*>(values->data()),values->size()*sizeof(float));
  }
 }
 return out?0:3;
}
