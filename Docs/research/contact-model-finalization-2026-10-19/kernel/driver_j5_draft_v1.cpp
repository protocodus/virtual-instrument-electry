// DRAFT ONLY. Offline stream/audit harness, NOT a realtime caller.
// No compile or execution authority. The numerical kernel uses fixed storage;
// this harness deliberately uses allocating strings/streams for evidence.
#include "contact_interval_j5_draft_v1.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace electry_multicontact_research;

static void word(std::ostream& s,double x) {
    const auto flags=s.flags();const auto fill=s.fill();
    s << '"' << std::hex << std::setw(16) << std::setfill('0') << detail::bits(x) << '"';
    s.flags(flags);s.fill(fill);
}
static void words(std::ostream& s,const std::array<double,maximum_contacts>& a) {
    s << '[';
    for(unsigned i=0;i<maximum_contacts;++i){if(i)s << ',';word(s,a[i]);}
    s << ']';
}
static void interval(std::ostream& s,Interval a) {
    s << '[';word(s,a.lo);s << ',';word(s,a.hi);s << ']';
}
static void intervals(std::ostream& s,const std::array<Interval,maximum_contacts>& a) {
    s << '[';
    for(unsigned i=0;i<maximum_contacts;++i){if(i)s << ',';interval(s,a[i]);}
    s << ']';
}
static const char* status_name(Status status) {
    switch(status) {
#define STATUS(name) case Status::name:return #name
        STATUS(stored_certificate_and_cpp_diagnostic_pass);
        STATUS(empty_system);STATUS(unsupported_host);STATUS(arithmetic_environment);
        STATUS(dimension);STATUS(nonfinite_input);STATUS(invalid_interval);
        STATUS(nonpositive_denominator);STATUS(nonfinite_primitive);
        STATUS(nonfinite_neighbor);STATUS(operator_bound);STATUS(endpoint_envelope);
        STATUS(invalid_budget);STATUS(nonpositive_slack);STATUS(cap_resource_limit);
        STATUS(trace_capacity);STATUS(negative_or_nonfinite_candidate);
        STATUS(force_box);STATUS(interval_width);STATUS(exhausted_cap);
        STATUS(nonfinite_cpp_diagnostic);STATUS(cpp_legacy_residual);
#undef STATUS
    }
    return "unknown_status";
}
static void input(std::ostream& s,const Input& x) {
    s << "{\"j\":" << x.j << ",\"A\":[";
    for(unsigned i=0;i<maximum_contacts;++i){if(i)s << ',';words(s,x.A[i]);}
    s << "],\"d0\":";words(s,x.d0);s << ",\"dfree\":";words(s,x.dfree);s << '}';
}
static void iteration(std::ostream& s,const Iteration& a) {
    s << "{\"force\":";words(s,a.force);s << ",\"mapped\":";intervals(s,a.mapped);
    s << ",\"residual\":";intervals(s,a.residual);
    s << ",\"certificate_residual\":";word(s,a.certificate_residual);
    s << ",\"measured_width_upper\":";word(s,a.measured_width_upper);
    s << ",\"updates\":" << a.updates << '}';
}
static void output(unsigned long long id,const Input& x,const Result& r,const Trace& t) {
    std::ostringstream s;s << std::boolalpha;
    s << "{\"schema\":\"electry-multicontact-interval-kernel-result/v1\",\"id\":" << id;
    s << ",\"maximum_contacts\":" << maximum_contacts;
    s << ",\"administrative_maximum_updates\":" << maximum_updates;
    s << ",\"input\":";input(s,x);
    s << ",\"status\":" << static_cast<unsigned>(r.status);
    s << ",\"status_name\":\"" << status_name(r.status) << '"';
    s << ",\"force\":";words(s,r.force);s << ",\"budgets\":{";
    s << "\"row_sum_upper\":";words(s,r.budgets.row_sum_upper);
    s << ",\"dot_excess_upper\":";words(s,r.budgets.dot_excess_upper);
    s << ",\"gap_excess_upper\":";words(s,r.budgets.gap_excess_upper);
#define VALUE(name) s << ",\"" #name "\":";word(s,r.budgets.name)
    VALUE(local_lambda_upper);VALUE(a_upper);VALUE(one_minus_lambda_lower);VALUE(box_R);
    VALUE(endpoint_P);VALUE(scaled_Q);VALUE(scaled_Q2);VALUE(scalar_excess_upper);
    VALUE(width_W);VALUE(recurrence_b_upper);VALUE(residual_C_upper);VALUE(slack_lower);
    VALUE(geometric_upper_at_cap);
#undef VALUE
    s << ",\"cap_products\":" << r.budgets.cap_products;
    s << ",\"cap_updates\":" << r.budgets.cap_updates;
    s << ",\"cap_bound_certified\":" << r.budgets.cap_bound_certified << '}';
    s << ",\"final_iteration\":";iteration(s,r.final_iteration);
    s << ",\"posterior_error_upper\":";word(s,r.posterior_error_upper);
    s << ",\"cpp_legacy_residual\":";word(s,r.cpp_legacy_residual);
    s << ",\"primitive_operations\":" << r.primitive_operations;
    s << ",\"entry_fpcr\":" << r.entry_fpcr << ",\"exit_fpcr\":" << r.exit_fpcr;
    s << ",\"map_evaluations\":" << r.map_evaluations << ",\"actual_updates\":" << r.actual_updates;
    s << ",\"analytic_zero_solution\":" << r.analytic_zero_solution;
    s << ",\"exact_stored_residual_certified\":" << r.exact_stored_residual_certified;
    s << ",\"cpp_legacy_diagnostic_pass\":" << r.cpp_legacy_diagnostic_pass;
    s << ",\"posterior_error_bound_valid\":" << r.posterior_error_bound_valid;
    s << ",\"original_numpy_legacy_bit_identity_owned\":" << r.original_numpy_legacy_bit_identity_owned;
    s << ",\"physical_endpoint_admitted\":" << r.physical_endpoint_admitted;
    s << ",\"trace_used\":" << t.used << ",\"trace\":[";
    for(unsigned i=0;i<t.used;++i){if(i)s << ',';iteration(s,t.rows[i]);}
    s << "]}";std::cout << s.str() << '\n' << std::flush;
}
static void quoted(std::ostream& s,const std::string& text) {
    s << '"';
    for(unsigned char c:text) {
        if(c=='"'||c=='\\')s << '\\' << static_cast<char>(c);
        else if(c<0x20||c>=0x7f) {
            const char* hex="0123456789abcdef";
            s << "\\u00" << hex[c>>4] << hex[c&15];
        } else s << static_cast<char>(c);
    }
    s << '"';
}
static int malformed(const std::string& line,const char* reason) {
    std::cout << "{\"schema\":\"electry-multicontact-transport-failure/v1\",\"raw_line\":";
    quoted(std::cout,line);std::cout << ",\"reason\":";quoted(std::cout,reason);
    std::cout << ",\"no_valid_force_or_endpoint\":true}" << '\n' << std::flush;
    return 2;
}
int main() {
    // No environment repairs, retries, physical updates or timing calls.
    // Caller-owned static history avoids a large main-stack object; this
    // single-stream audit harness is not a shared/thread-safe realtime API.
    static Trace trace;
    std::string line;
    while(std::getline(std::cin,line)) {
        std::istringstream in(line);unsigned long long id=0;Input x;std::string raw,extra;
        if(!(in>>id>>x.j))return malformed(line,"missing unsigned id/dimension");
        auto read=[&](double& value){
            if(!(in>>raw)||raw.size()!=16)return false;
            for(char c:raw)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
            std::uint64_t bits=0;std::istringstream hex(raw);hex>>std::hex>>bits;
            if(!hex||!hex.eof())return false;value=detail::from_bits(bits);return true;
        };
        // Fixed25+5+5 words even for j=0 or j>5. No index uses the input j.
        for(auto& row:x.A)for(auto& value:row)if(!read(value))return malformed(line,"matrix raw words");
        for(auto& value:x.d0)if(!read(value))return malformed(line,"d0 raw words");
        for(auto& value:x.dfree)if(!read(value))return malformed(line,"dfree raw words");
        if(in>>extra)return malformed(line,"extra request token");
        const Result result=solve(x,&trace);output(id,x,result,trace);
    }
    return std::cin.bad()?3:0;
}
