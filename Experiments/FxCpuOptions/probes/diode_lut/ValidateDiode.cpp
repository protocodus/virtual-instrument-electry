#include "DSP/ElectryFx.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdint>

namespace electry {
struct ElectryFxTestAccess {
 static void prepareInverse(ElectryFx& fx, double rate) {
   fx.oversampledRate_ = static_cast<float>(rate);
   fx.prepareDiodeInverse();
 }
 static float step(ElectryFx& fx, double in, double& v, double& d) {
   return fx.diodePairLookup(in,v,d);
 }
 static float reference(double rate, double in, double& v, double& d) {
   return ElectryFx::diodePairStep(in,rate,v,d);
 }
 static double domain(const ElectryFx& fx) { return fx.diodeInverseMaximum_; }
};
}
int main() {
  using electry::ElectryFxTestAccess;
  constexpr double rc=2200.0*10.0e-9, thermal=1.752*.0258, is=2.52e-9,c=10.0e-9;
  std::cout << std::setprecision(17);
  std::cout << "rate,mode,peak_voltage_error,rms_voltage_error,peak_lookup_residual,peak_reference_residual,minimum_derivative,maximum_abs_voltage\n";
  for (double rate : {64000.,88200.,176400.,352800.,384000.,705600.,768000.}) {
    electry::ElectryFx fx;
    ElectryFxTestAccess::prepareInverse(fx,rate);
    const double h=1/rate,a=1+.5*h/rc,b=h*is/c;
    const double domain=ElectryFxTestAccess::domain(fx);
    double peakResidual=0,minimumDerivative=1e9,previous=0;
    for(int i=0;i<=65536;++i) {
      const double rhs=domain*static_cast<double>(i)/65537;
      double v=rhs,d=0;
      ElectryFxTestAccess::step(fx,0,v,d);
      peakResidual=std::max(peakResidual,std::abs(a*v+b*std::sinh(v/thermal)-rhs));
      if(i>0) minimumDerivative=std::min(minimumDerivative,(v-previous)/(domain/65537));
      previous=v;
    }
    std::cout << rate << ",inverse-domain,0,0," << peakResidual << ",0," << minimumDerivative << "," << previous << "\n";
    for(int mode=0;mode<5;++mode) {
      double v=0,d=0,rv=0,rd=0,peak=0,sumsq=0,maxResidual=0,maxRefResidual=0,maxV=0;
      std::uint32_t random=12345;
      constexpr int frames=100000;
      for(int i=0;i<frames;++i) {
        random=random*1664525u+1013904223u;
        const double noise=static_cast<double>(random>>8)/8388608.-1.;
        const double t=i/rate;
        double in=0;
        if(mode==0) in=.001*(std::sin(2*M_PI*80*t)+.3*std::sin(2*M_PI*7039*t));
        if(mode==1) in=3*(std::sin(2*M_PI*80*t)+.3*std::sin(2*M_PI*7039*t));
        if(mode==2) in=12*noise;
        if(mode==3) in=(i%2)?12:-12;
        if(mode==4) in=i<frames/4?12:i<frames/2?-12:0;
        const double rhs=v+.5*h*(d+in/rc);
        const double rrhs=rv+.5*h*(rd+in/rc);
        ElectryFxTestAccess::step(fx,in,v,d);
        ElectryFxTestAccess::reference(rate,in,rv,rd);
        const double error=v-rv;
        peak=std::max(peak,std::abs(error)); sumsq+=error*error;
        maxResidual=std::max(maxResidual,std::abs(a*v+b*std::sinh(v/thermal)-rhs));
        maxRefResidual=std::max(maxRefResidual,std::abs(a*rv+b*std::sinh(rv/thermal)-rrhs));
        maxV=std::max(maxV,std::abs(v));
      }
      std::cout << rate << ",sequence-" << mode << "," << peak << "," << std::sqrt(sumsq/frames) << "," << maxResidual << "," << maxRefResidual << ",0," << maxV << "\n";
    }
  }
}
