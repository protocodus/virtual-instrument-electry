#include "DSP/ElectryFx.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
namespace electry {
struct ElectryFxTestAccess {
 static void prepareInverse(ElectryFx& fx, double rate) {
   fx.oversampledRate_ = static_cast<float>(rate);
   fx.prepareDiodeInverse();
 }
};
}
int main() {
 std::cout << "internal_rate,preparations,median_ms,min_ms,max_ms,table_bytes\n";
 for (double rate : {64000.,352800.,384000.,768000.}) {
  std::vector<double> elapsed;
  for(int repeat=0;repeat<21;++repeat) {
   electry::ElectryFx fx;
   const auto start=std::chrono::steady_clock::now();
   electry::ElectryFxTestAccess::prepareInverse(fx,rate);
   const auto end=std::chrono::steady_clock::now();
   if(repeat) elapsed.push_back(std::chrono::duration<double,std::milli>(end-start).count());
  }
  std::sort(elapsed.begin(),elapsed.end());
  std::cout << rate << ",20," << .5*(elapsed[9]+elapsed[10]) << "," << elapsed.front() << "," << elapsed.back() << ",65552\n";
 }
}
