// DRAFT compile-only initially. Separate child-only guard controls; no execution authority.
// Never repairs an environment inside solve; parent process never changes modes.
#include "contact_interval_j5_draft_v1.hpp"
#include <iostream>
#include <string>
using namespace electry_multicontact_research;
int main(int argc,char** argv) {
#if defined(__APPLE__) && defined(__aarch64__)
    if(argc!=2)return 2;
    const std::string mode=argv[1];const auto saved=detail::read_fpcr();
    const int saved_round=std::fegetround();
    if(saved!=0||saved_round!=FE_TONEAREST)return 3;
    bool changed=false;
    if(mode=="FE_DOWNWARD")changed=std::fesetround(FE_DOWNWARD)==0;
    else if(mode=="FTZ_BIT24") {
        const auto value=saved|UINT64_C(0x01000000);
        __asm__ __volatile__("msr fpcr, %0\n\tisb"::"r"(value):"memory");changed=true;
    }
    if(!changed)return 4;
    const auto before=detail::read_fpcr();const int before_round=std::fegetround();
    Input input;input.j=1;static Trace trace;
    const Result result=solve(input,&trace);
    const auto after=detail::read_fpcr();const int after_round=std::fegetround();
    std::fesetround(saved_round);
    __asm__ __volatile__("msr fpcr, %0\n\tisb"::"r"(saved):"memory");
    const auto restored=detail::read_fpcr();const int restored_round=std::fegetround();
    const bool pass=before!=saved&&after==before&&before_round==after_round&&
        result.status==Status::arithmetic_environment&&result.entry_fpcr==before&&
        result.map_evaluations==0&&result.primitive_operations==0&&trace.used==0&&
        !result.exact_stored_residual_certified&&!result.posterior_error_bound_valid&&
        !result.cpp_legacy_diagnostic_pass&&!result.analytic_zero_solution&&
        !result.budgets.cap_bound_certified&&restored==saved&&restored_round==saved_round;
    std::cout<<"{\"schema\":\"electry-multicontact-environment-negative/v1\",\"mode\":\""<<mode<<"\",\"status\":"<<static_cast<unsigned>(result.status)
      <<",\"saved_fpcr\":"<<saved<<",\"before_fpcr\":"<<before<<",\"after_fpcr\":"<<after
      <<",\"restored_fpcr\":"<<restored<<",\"saved_round\":"<<saved_round
      <<",\"before_round\":"<<before_round<<",\"after_round\":"<<after_round
      <<",\"restored_round\":"<<restored_round<<",\"entry_fpcr\":"<<result.entry_fpcr
      <<",\"map_evaluations\":"<<result.map_evaluations<<",\"primitive_operations\":"<<result.primitive_operations
      <<",\"trace_rows\":"<<trace.used<<",\"exact_stored_residual_certified\":"<<std::boolalpha<<result.exact_stored_residual_certified
      <<",\"posterior_error_bound_valid\":"<<result.posterior_error_bound_valid
      <<",\"cpp_legacy_diagnostic_pass\":"<<result.cpp_legacy_diagnostic_pass
      <<",\"analytic_zero_solution\":"<<result.analytic_zero_solution
      <<",\"cap_bound_certified\":"<<result.budgets.cap_bound_certified<<",\"PASS\":"<<pass<<"}\n";
    return pass?0:5;
#else
    (void)argc;(void)argv;return 6;
#endif
}
