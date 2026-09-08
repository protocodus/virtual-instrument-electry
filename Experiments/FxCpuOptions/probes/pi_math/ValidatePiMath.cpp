#include "../Source/DSP/ElectryFx.cpp"
#include <iostream>
#include <iomanip>
#include <random>

int main()
{
    using namespace electry;
    std::cout << std::setprecision(14);
    std::cout << "table_bytes," << sizeof(PhaseMathTables) << "\n";
    double softAbs=0,softRel=0,softSlopeAbs=0,powerRel=0,powerSlopeRel=0;
    for(int i=0;i<=100000;++i)
    {
        double x=-40.0 + 48.0*i/100000.0;
        const auto y=phaseSoftplus(x);
        const double exact=std::max(x,0.0)+std::log1p(std::exp(-std::abs(x)));
        const double slope=1/(1+std::exp(-x));
        softAbs=std::max(softAbs,std::abs(y.value-exact));
        softRel=std::max(softRel,std::abs(y.value-exact)/exact);
        softSlopeAbs=std::max(softSlopeAbs,std::abs(y.slope-slope));
        for(const auto model:{AmpModel::AmericanClean,AmpModel::BritishCrunch})
        {
            const double exponent=phaseInverterParameters(model).exponent;
            const double v=std::exp2(-60.0+70.0*i/100000.0);
            const auto p=phasePower(v,model);
            const double q=std::pow(v,exponent);
            const double d=exponent*q/v;
            powerRel=std::max(powerRel,std::abs(p.value-q)/q);
            powerSlopeRel=std::max(powerSlopeRel,std::abs(p.slope-d)/d);
        }
    }
    std::cout << "soft_abs," << softAbs << "\nsoft_relative," << softRel
              << "\nsoft_slope_abs," << softSlopeAbs << "\npower_relative," << powerRel
              << "\npower_slope_relative," << powerSlopeRel << "\n";
    for(const auto model:{AmpModel::AmericanClean,AmpModel::BritishCrunch})
    {
        double currentAbs=0,currentRel=0,plateSlopeAbs=0,gridSlopeAbs=0;
        std::mt19937_64 rng(624215);
        std::uniform_real_distribution<double> plate(-100,1200),grid(-100,2);
        for(int i=0;i<300000;++i)
        {
            const double vp=plate(rng),vg=grid(rng);
            const auto a=phaseInverterCurrentAndSlopes(model,vp,vg);
            const auto b=exactPhaseInverterCurrentAndSlopes(model,vp,vg);
            currentAbs=std::max(currentAbs,std::abs(a.plate-b.plate));
            if(b.plate>1e-18) currentRel=std::max(currentRel,std::abs(a.plate-b.plate)/b.plate);
            plateSlopeAbs=std::max(plateSlopeAbs,std::abs(a.plateSlope-b.plateSlope));
            gridSlopeAbs=std::max(gridSlopeAbs,std::abs(a.gridSlope-b.gridSlope));
            if(!std::isfinite(a.plate)||!std::isfinite(a.plateSlope)||!std::isfinite(a.gridSlope)) return 1;
        }
        std::cout << "model," << static_cast<int>(model) << "\ncurrent_abs_A," << currentAbs
                  << "\ncurrent_relative," << currentRel << "\nplate_slope_abs," << plateSlopeAbs
                  << "\ngrid_slope_abs," << gridSlopeAbs << "\n";
    }
}
