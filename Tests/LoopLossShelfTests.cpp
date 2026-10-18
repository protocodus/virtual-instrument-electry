// The candidate keeps each pre-existing feasible traversal anchor. Compare
// against the independent complex response of that original one-pole/scalar,
// not against a duplicate of the shelf coefficient formula. A physical T60
// additionally depends on the complete dispersive loop's group delay.
#include "DSP/ElectryEngine.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <iostream>
#include <memory>

namespace electry {
struct ElectryEngineTestAccess {
    struct Result { std::array<double,5> coefficients; double gain; double pole, correctionGain, zero; bool active; bool historiesRetained; };
#if ELECTRY_BUTTERWORTH_LOSS_SHELF
    static Result transform(float coefficient, float gain, float omega0, float high,
                            float totalPole = -1, float totalGain = -1) {
        auto loop=std::make_unique<ElectryEngine::PolarisationLoop>();
        loop->loopDampingCoefficient=totalPole<0?coefficient:totalPole;
        loop->loopGain=totalGain<0?gain:totalGain;
        loop->damping.state=.03125f;
        loop->lossShelf.z1=.000125;loop->lossShelf.z2=-.0000625;
        ElectryEngine::configureLoopLossShelf(*loop,omega0,high,coefficient,gain);
        const auto& b=loop->lossShelf;
        return {{b.b0,b.b1,b.b2,b.a1,b.a2},loop->loopGain,
                loop->loopDampingCoefficient,loop->lossShelfGain,loop->lossShelfZeroCoefficient,
                loop->lossShelfActive,b.z1==.000125&&b.z2==-.0000625&&loop->damping.state==.03125f};
    }
    struct Grid { int configurations=0, active=0, fallbacks=0, violations=0, historyChanges=0; double maximum=0; };
    static Grid configurationGrid() {
        Grid result;
        auto e=std::make_unique<ElectryEngine>();
        for(double rate:{48000.,96000.,384000.})
        for(float age:{0.f,.3f,1.f})for(float gauge:{0.f,1.f})
        for(float pressure:{0.f,.5f,1.f})for(auto style:{PlayStyle::Sustain,PlayStyle::PalmMute,PlayStyle::Dead}) {
            EngineParameters p;applyGuitarBuild(p,defaultGuitarBuild);
            p.stringAge=age;p.stringGauge=gauge;p.palmMute=pressure;
            p.strumSpreadSeconds=0;p.bodyResonance=p.sympatheticAmount=p.artifactAmount=0;
            e->setParameters(p);e->prepare(rate,64);
            for(int stringIndex=0;stringIndex<8;++stringIndex)for(int fret:{0,12,22}) {
                e->reset();e->setSoloStringMask(1u<<stringIndex);
                e->noteOn(ElectryEngine::firstPlayStyleKeyswitchNote+static_cast<int>(style),1);
                e->noteOn(e->stringSpecs()[stringIndex].openMidiNote+fret,.85f);
                auto&v=e->voices_[stringIndex];v.attackPitchTensionRatio=0;e->configureVoicePitch(v,true);
                for(auto*loop:{&v.vertical,&v.horizontal}) {
                    ++result.configurations;
                    if(loop->lossShelfActive)++result.active;
                    else if(stringIndex>=2&&loop->loopDampingCoefficient>0)++result.fallbacks;
                    const auto section=[](const auto&b,std::complex<double>z){return (b.b0+b.b1*z+b.b2*z*z)/(1.0+b.a1*z+b.a2*z*z);};
                    for(int bin=0;bin<=128;++bin) {
                        const auto z=std::polar(1.0,-3.14159265358979323846*bin/128);
                        const double a=loop->loopDampingCoefficient;
                        auto h=double(loop->loopGain)*(1-a)/(1.0-a*z);
                        if(loop->lossShelfActive)h*=loop->lossShelfGain*(1.0-double(loop->lossShelfZeroCoefficient)*z)*section(loop->lossShelf,z);
                        if(loop->handDipActive)h*=section(loop->handDip,z);
                        if(loop->materialLossDepth>0)h*=section(loop->materialLossDip,z);
                        const double magnitude=std::abs(h);result.maximum=std::max(result.maximum,magnitude);
                        if(!std::isfinite(magnitude)||magnitude>1+1e-10)++result.violations;
                    }
                }
                // A repeated same-coordinate solve must retain every new
                // running state and must not compound its correction gain.
                auto&l=v.vertical;l.lossShelf.z1=.000125;l.lossShelf.z2=-.0000625;l.lossShelfInputState=.00025;
                const auto before=l.lossShelf;const double beforeGain=l.lossShelfGain;const float beforeLoopGain=l.loopGain;
                e->configureVoiceDamping(v,style,v.lastConfiguredFrequency,float(fret));
                if(l.lossShelf.z1!=.000125||l.lossShelf.z2!=-.0000625||l.lossShelfInputState!=.00025||l.loopGain!=beforeLoopGain||l.lossShelfGain!=beforeGain||l.lossShelf.b0!=before.b0||l.lossShelf.a1!=before.a1)++result.historyChanges;
            }
        }
        return result;
    }
#endif
}; }
namespace {
int failures=0,checks=0;
void expect(bool value,const char*text){++checks;if(!value){++failures;if(failures<12)std::cerr<<text<<'\n';}}
constexpr double pi=3.14159265358979323846;
std::complex<double> response(const electry::ElectryEngineTestAccess::Result&r,double omega){
    const auto z=std::polar(1.0,-omega);const auto&b=r.coefficients;
    return r.gain*(1-r.pole)/(1.0-r.pole*z)*r.correctionGain*(1.0-r.zero*z)
        *(b[0]+b[1]*z+b[2]*z*z)/(1.0+b[3]*z+b[4]*z*z);
}
}
int main(){
#if ELECTRY_BUTTERWORTH_LOSS_SHELF
    using Access=electry::ElectryEngineTestAccess;
    double maxAnchorRelative=0,maxMagnitude=0;int fallback=0;
    // Cover all host/internal rates, ratio extremes and scalar headroom.
    // These are filter-domain coordinates, independent of the corpus.
    for(double rate:{16000.,88200.,96000.,192000.,384000.,768000.})
    for(double f0:{20.,41.203444614,82.406889228,195.99771799,783.99087196,2637.0204553}){
        f0=std::min(f0,.24*rate);const float w0=static_cast<float>(2*pi*f0/rate);
        const float wh=static_cast<float>(2*pi*std::max(std::min(3600.,.32*rate),1.5*f0)/rate);
        for(float pole:{0.f,.00001f,.01f,.1f,.3f,.6f,.9f,.99f,.9995f})
        for(float gain:{.1f,.9f,.9999f,.99999f}){
            const auto r=Access::transform(pole,gain,w0,wh);
            expect(r.historiesRetained,"live transform resets running history");
            expect(r.gain<=gain&&r.gain>0,"scalar exceeds original passive scalar");
            if(!r.active){++fallback;if(pole!=0&&fallback<20)std::cerr<<"fallback rate="<<rate<<" f0="<<f0<<" pole="<<pole<<" gain="<<gain<<'\n';expect(pole==0,"nonflat transform unexpectedly fell back");continue;}
            const auto&b=r.coefficients;
            const auto discriminant=std::sqrt(std::complex<double>(b[3]*b[3]-4*b[4],0));
            expect(std::abs((-b[3]+discriminant)/2.0)<1&&std::abs((-b[3]-discriminant)/2.0)<1,"shelf pole not strictly stable");
            for(double w:{double(w0),double(wh)}){
                const auto z=std::polar(1.0,-w);
                const double old=std::abs(double(gain)*(1-double(pole))/(1.0-double(pole)*z));
                const double error=std::abs(std::abs(response(r,w))/old-1);
                maxAnchorRelative=std::max(maxAnchorRelative,error);
                expect(error<8e-8,"existing traversal anchor changed");
            }
            double previous=2;
            for(int index=0;index<=128;++index){
                const double magnitude=std::abs(response(r,pi*index/128));
                maxMagnitude=std::max(maxMagnitude,magnitude);
                expect(std::isfinite(magnitude)&&magnitude<=gain+2e-10,"shelf amplifies a frequency beyond original scalar");
                expect(magnitude<=previous+2e-10,"shelf is not monotonic");previous=magnitude;
            }
        }
    }
    const auto grid=Access::configurationGrid();
    expect(grid.violations==0,"actual style/age/gauge/pressure family has gain above unity");
    expect(grid.historyChanges==0,"same-coordinate refit changes shelf history or compounds gain");
    std::cout<<"Actual family "<<grid.configurations<<" loops, active "<<grid.active<<", fallback "<<grid.fallbacks<<", max magnitude "<<grid.maximum<<", history changes "<<grid.historyChanges<<'\n';
    // Deliberately active total filters must be rejected if their corrected
    // response remains above unity; the certificate is not an unconditional
    // claim that every algebraic correction is passive.
    const auto unsafe=Access::transform(.3f,.9999f,.01f,.4f,.3f,1.1f);
    expect(!unsafe.active,"passivity certificate accepts an active total filter");
    std::cout<<"Shelf anchor maximum relative error "<<maxAnchorRelative<<", maximum magnitude "<<maxMagnitude<<", flat bypasses "<<fallback<<'\n';
#else
    expect(true,"default bypass builds");
#endif
    std::cout<<checks<<" checks, "<<failures<<" failures\n";return failures?1:0;
}
