#pragma once

// DRAFT ONLY: no compile or execution authority. Simultaneous capacity-five
// research stored-equation kernel. Not a physical update or realtime claim.
// Compile all callers with the bound strict flags; no FMA/reassociation/LTO.
#include <array>
#include <cfenv>
#include <cfloat>
#include <cstdint>
#include <cstring>
#include <limits>

#pragma STDC FENV_ACCESS ON
#pragma clang fp contract(off)
#pragma clang fp reassociate(off)

namespace electry_multicontact_research {

static_assert(sizeof(double) == 8 && sizeof(std::uint64_t) == 8, "binary64 storage required");
static_assert(std::numeric_limits<double>::radix == 2 &&
              std::numeric_limits<double>::digits == 53 &&
              std::numeric_limits<double>::min_exponent == -1021 &&
              std::numeric_limits<double>::max_exponent == 1024 &&
              std::numeric_limits<double>::is_iec559 &&
              std::numeric_limits<double>::has_denorm == std::denorm_present,
              "IEEE binary64 with subnormals required");
static_assert(FLT_EVAL_METHOD == 0, "no excess precision is admitted");

constexpr double stiffness = 6000.0;
constexpr double half_stiffness = 3000.0;
constexpr double residual_rail = 0x1.5fd7fe1796495p-36; // original binary64 2e-11
constexpr unsigned maximum_contacts = 5;
constexpr double unit_roundoff = 0x1p-53;
constexpr double four_u = 0x1p-51;
constexpr double sigma = 0x0.0000000000001p-1022;
constexpr double two_sigma = 0x0.0000000000002p-1022;
// Administrative resource limit; not a certified universal convergence cap.
constexpr unsigned maximum_updates = 4096;
static_assert(residual_rail == 2e-11, "original binary64 residual rail");

enum class Status : unsigned {
    stored_certificate_and_cpp_diagnostic_pass,
    empty_system,
    unsupported_host,
    arithmetic_environment,
    dimension,
    nonfinite_input,
    invalid_interval,
    nonpositive_denominator,
    nonfinite_primitive,
    nonfinite_neighbor,
    operator_bound,
    endpoint_envelope,
    invalid_budget,
    nonpositive_slack,
    cap_resource_limit,
    trace_capacity,
    negative_or_nonfinite_candidate,
    force_box,
    interval_width,
    exhausted_cap,
    nonfinite_cpp_diagnostic,
    cpp_legacy_residual
};

struct Interval { double lo = 0.0; double hi = 0.0; };
struct Input {
    unsigned j = 0;
    std::array<std::array<double, maximum_contacts>, maximum_contacts> A {};
    std::array<double, maximum_contacts> d0 {};
    std::array<double, maximum_contacts> dfree {};
};
struct Budgets {
    std::array<double, maximum_contacts> row_sum_upper {};
    std::array<double, maximum_contacts> dot_excess_upper {};
    std::array<double, maximum_contacts> gap_excess_upper {};
    double local_lambda_upper = 0.0;
    double a_upper = 0.0;
    double one_minus_lambda_lower = 0.0;
    double box_R = 0.0;
    double endpoint_P = 0.0;
    double scaled_Q = 0.0;
    double scaled_Q2 = 0.0;
    double scalar_excess_upper = 0.0;
    double width_W = 0.0;
    double recurrence_b_upper = 0.0;
    double residual_C_upper = 0.0;
    double slack_lower = 0.0;
    double geometric_upper_at_cap = 0.0;
    unsigned cap_products = 0;
    unsigned cap_updates = 0;
    bool cap_bound_certified = false;
};
struct Iteration {
    std::array<double, maximum_contacts> force {};
    std::array<Interval, maximum_contacts> mapped {};
    std::array<Interval, maximum_contacts> residual {};
    double certificate_residual = 0.0;
    double measured_width_upper = 0.0;
    unsigned updates = 0;
};
// Optional caller-owned fixed trace; no heap or required large local history.
struct Trace {
    std::array<Iteration, maximum_updates + 1> rows {};
    unsigned used = 0;
};
struct Result {
    Status status = Status::invalid_budget;
    std::array<double, maximum_contacts> force {};
    Budgets budgets {};
    Iteration final_iteration {};
    double posterior_error_upper = std::numeric_limits<double>::quiet_NaN();
    double cpp_legacy_residual = 0.0;
    std::uint64_t primitive_operations = 0;
    std::uint64_t entry_fpcr = 0;
    std::uint64_t exit_fpcr = 0;
    unsigned map_evaluations = 0;
    unsigned actual_updates = 0;
    bool analytic_zero_solution = false;
    bool exact_stored_residual_certified = false;
    bool cpp_legacy_diagnostic_pass = false;
    bool posterior_error_bound_valid = false;
    // This API does not own NumPy dot/power rounding or any applied state/work.
    bool original_numpy_legacy_bit_identity_owned = false;
    bool physical_endpoint_admitted = false;
};

namespace detail {

inline std::uint64_t bits(double x) noexcept {
    std::uint64_t u = 0;
    std::memcpy(&u, &x, sizeof u);
    return u;
}
inline double from_bits(std::uint64_t u) noexcept {
    double x = 0.0;
    std::memcpy(&x, &u, sizeof x);
    return x;
}
inline bool finite(double x) noexcept {
    return (bits(x) & UINT64_C(0x7ff0000000000000)) != UINT64_C(0x7ff0000000000000);
}
inline double absolute(double x) noexcept {
    return from_bits(bits(x) & UINT64_C(0x7fffffffffffffff));
}
inline double minimum(double x, double y) noexcept { return x < y ? x : y; }
inline double maximum(double x, double y) noexcept { return x > y ? x : y; }

inline bool host_supported() noexcept {
#if defined(__APPLE__) && defined(__aarch64__)
    return true;
#else
    return false;
#endif
}
inline std::uint64_t read_fpcr() noexcept {
#if defined(__APPLE__) && defined(__aarch64__)
    std::uint64_t u = 0;
    __asm__ __volatile__("mrs %0, fpcr" : "=r"(u) : : "memory");
    return u;
#else
    return UINT64_MAX;
#endif
}
inline bool environment_ok(std::uint64_t& fpcr) noexcept {
    fpcr = read_fpcr();
    // Deliberately narrow host contract: RN-even, no FTZ/FIZ/AH/DN or traps.
    // Traits/checks are not a proof against an asynchronous caller mode change.
    return host_supported() && fpcr == 0 && std::fegetround() == FE_TONEAREST;
}

// Every non-identity primitive is its own volatile binary64 operation.
inline double rn_add(double a, double b) noexcept {
    volatile double x = a, y = b;
    volatile double z = x + y;
    return z;
}
inline double rn_sub(double a, double b) noexcept {
    volatile double x = a, y = b;
    volatile double z = x - y;
    return z;
}
inline double rn_mul(double a, double b) noexcept {
    volatile double x = a, y = b;
    volatile double z = x * y;
    return z;
}
inline double rn_div(double a, double b) noexcept {
    volatile double x = a, y = b;
    volatile double z = x / y;
    return z;
}

struct Ops {
    Status failure = Status::stored_certificate_and_cpp_diagnostic_pass;
    std::uint64_t count = 0;
    unsigned map_count = 0;
    bool ok() const noexcept { return failure == Status::stored_certificate_and_cpp_diagnostic_pass; }
    void reject(Status s) noexcept { if (ok()) failure = s; }
    bool valid(Interval x) noexcept {
        if (!finite(x.lo) || !finite(x.hi) || x.lo > x.hi) {
            reject(Status::invalid_interval);
            return false;
        }
        return true;
    }
    double neighbor(double x, bool upper) noexcept {
        if (!finite(x)) { reject(Status::nonfinite_primitive); return 0.0; }
        std::uint64_t u = bits(x);
        if (x == 0.0) u = upper ? UINT64_C(1) : UINT64_C(0x8000000000000001);
        else if ((x > 0.0) == upper) ++u;
        else --u;
        const double y = from_bits(u);
        if (!finite(y)) { reject(Status::nonfinite_neighbor); return 0.0; }
        return y;
    }
    Interval around(double x) noexcept { return {neighbor(x, false), neighbor(x, true)}; }
    Interval add_point(double a, double b) noexcept {
        if (!finite(a) || !finite(b)) { reject(Status::nonfinite_input); return {}; }
        // These exact real identities do not require a rounding allowance.
        if (a == 0.0) return {b, b};
        if (b == 0.0) return {a, a};
        ++count;
        return around(rn_add(a, b));
    }
    Interval sub_point(double a, double b) noexcept {
        if (!finite(a) || !finite(b)) { reject(Status::nonfinite_input); return {}; }
        if (a == b) return {};
        if (b == 0.0) return {a, a};
        ++count;
        return around(rn_sub(a, b));
    }
    Interval mul_point(double a, double b) noexcept {
        if (!finite(a) || !finite(b)) { reject(Status::nonfinite_input); return {}; }
        if (a == 0.0 || b == 0.0) return {};
        ++count;
        return around(rn_mul(a, b));
    }
    Interval div_point(double a, double b) noexcept {
        if (!finite(a) || !finite(b)) { reject(Status::nonfinite_input); return {}; }
        if (!(b > 0.0)) { reject(Status::nonpositive_denominator); return {}; }
        if (a == 0.0) return {};
        if (a == b) return {1.0, 1.0};
        ++count;
        return around(rn_div(a, b));
    }
    Interval add(Interval a, Interval b) noexcept {
        if (!valid(a) || !valid(b)) return {};
        const auto l = add_point(a.lo, b.lo);
        const auto u = add_point(a.hi, b.hi);
        return {l.lo, u.hi};
    }
    Interval sub(Interval a, Interval b) noexcept {
        if (!valid(a) || !valid(b)) return {};
        const auto l = sub_point(a.lo, b.hi);
        const auto u = sub_point(a.hi, b.lo);
        return {l.lo, u.hi};
    }
    Interval multiply(Interval a, Interval b) noexcept {
        if (!valid(a) || !valid(b)) return {};
        const std::array<Interval, 4> p {{mul_point(a.lo,b.lo),mul_point(a.lo,b.hi),
                                         mul_point(a.hi,b.lo),mul_point(a.hi,b.hi)}};
        Interval r = p[0];
        for (unsigned i = 1; i < p.size(); ++i) {
            r.lo = minimum(r.lo,p[i].lo); r.hi = maximum(r.hi,p[i].hi);
        }
        return r;
    }
    Interval divide(Interval a, Interval b) noexcept {
        if (!valid(a) || !valid(b)) return {};
        if (!(b.lo > 0.0)) { reject(Status::nonpositive_denominator); return {}; }
        const std::array<Interval, 4> p {{div_point(a.lo,b.lo),div_point(a.lo,b.hi),
                                         div_point(a.hi,b.lo),div_point(a.hi,b.hi)}};
        Interval r = p[0];
        for (unsigned i = 1; i < p.size(); ++i) {
            r.lo = minimum(r.lo,p[i].lo); r.hi = maximum(r.hi,p[i].hi);
        }
        return r;
    }
    Interval intersect(Interval x, double lo, double hi) noexcept {
        if (!valid(x) || !finite(lo) || !finite(hi) || lo > hi) {
            reject(Status::invalid_interval); return {};
        }
        x.lo = maximum(x.lo,lo); x.hi = minimum(x.hi,hi);
        if (!valid(x)) return {};
        return x;
    }
    Interval nonnegative(Interval x) noexcept {
        if (!valid(x)) return {};
        x.lo = maximum(x.lo,0.0);
        if (!valid(x)) return {};
        return x;
    }
    double upper_add(double a,double b) noexcept { return add_point(a,b).hi; }
    double upper_mul(double a,double b) noexcept { return mul_point(a,b).hi; }
    double upper_div(double a,double b) noexcept { return div_point(a,b).hi; }
    double e(double x) noexcept {
        if (!finite(x) || x < 0.0) { reject(Status::invalid_budget); return 0.0; }
        return upper_add(upper_mul(four_u,x),two_sigma);
    }
};

inline Interval scalar_g(double a,double b,double Qcap,Ops& o) noexcept {
    if (!finite(a) || !finite(b) || !finite(Qcap) || Qcap < 0.0) {
        o.reject(Status::nonfinite_input); return {};
    }
    if (a <= 0.0 && b <= 0.0) return {};
    if (a > 0.0 && b > 0.0) {
        auto x = o.intersect(o.mul_point(half_stiffness,a),0.0,Qcap);
        auto y = o.intersect(o.mul_point(half_stiffness,b),0.0,Qcap);
        return o.nonnegative(o.add(x,y));
    }
    const double p = a > 0.0 ? a : b;
    const double c = absolute(a > 0.0 ? b : a);
    Interval r {1.0,1.0};
    if (c != 0.0) {
        const auto t = o.intersect(o.div_point(p >= c ? c : p,p >= c ? p : c),0.0,1.0);
        const auto d = o.intersect(o.add({1.0,1.0},t),1.0,2.0);
        r = o.intersect(o.divide(p >= c ? Interval{1.0,1.0} : t,d),0.0,1.0);
    }
    const auto s = o.intersect(o.multiply({p,p},r),0.0,p);
    return o.nonnegative(o.multiply({half_stiffness,half_stiffness},s));
}

inline std::array<Interval,maximum_contacts> map(const Input& x,const std::array<double,maximum_contacts>& f,
                                  double Qcap,double Pcap,Ops& o) noexcept {
    ++o.map_count;
    std::array<Interval,maximum_contacts> out {};
    for (unsigned i = 0; i < x.j; ++i) {
        Interval dot = o.mul_point(x.A[i][0],f[0]);
        for (unsigned j = 1; j < x.j; ++j)
            dot = o.add(dot,o.mul_point(x.A[i][j],f[j]));
        const auto gap = o.sub({x.dfree[i],x.dfree[i]},dot);
        if (absolute(x.d0[i]) > Pcap || absolute(gap.lo) > Pcap || absolute(gap.hi) > Pcap) {
            o.reject(Status::endpoint_envelope); return out;
        }
        const auto l = scalar_g(x.d0[i],gap.lo,Qcap,o);
        const auto u = scalar_g(x.d0[i],gap.hi,Qcap,o);
        out[i] = o.nonnegative({l.lo,u.hi});
    }
    return out;
}

// Before force iteration, q_k encloses local_lambda_upper^k*a_upper.
// Only a proven crossing q_k<=slack creates a cap; ceiling exhaustion rejects.
inline bool derive_cap(Budgets& b,Ops& o) noexcept {
    b.geometric_upper_at_cap = b.a_upper;
    b.cap_updates = 0;
    b.cap_products = 0;
    for (;;) {
        if (!o.ok()) return false;
        if (b.geometric_upper_at_cap <= b.slack_lower) {
            b.cap_bound_certified = true;
            return true;
        }
        if (b.cap_updates == maximum_updates) {
            o.reject(Status::cap_resource_limit); return false;
        }
        b.geometric_upper_at_cap = o.upper_mul(b.local_lambda_upper,
                                                b.geometric_upper_at_cap);
        ++b.cap_updates;
        ++b.cap_products;
    }
}

inline bool prepare_budgets(const Input& x,Budgets& b,Ops& o) noexcept {
    for (unsigned i = 0; i < x.j; ++i) {
        double sum = absolute(x.A[i][0]);
        for (unsigned j = 1; j < x.j; ++j)
            sum = o.upper_add(sum,absolute(x.A[i][j]));
        b.row_sum_upper[i] = sum;
        b.local_lambda_upper = maximum(b.local_lambda_upper,
                                       o.upper_mul(half_stiffness,sum));
    }
    if (!o.ok()) return false;
    if (!(b.local_lambda_upper < 1.0)) {
        o.reject(Status::operator_bound); return false;
    }
    const std::array<double,maximum_contacts> zero_force {};
    const auto initial = map(x,zero_force,std::numeric_limits<double>::max(),
                             std::numeric_limits<double>::max(),o);
    for (unsigned i = 0; i < x.j; ++i)
        b.a_upper = maximum(b.a_upper,initial[i].hi);
    if (!o.ok()) return false;
    if (b.a_upper == 0.0) {
        b.cap_bound_certified = true; // Unique exact zero fixed point.
        return true;
    }
    b.one_minus_lambda_lower = o.sub_point(1.0,b.local_lambda_upper).lo;
    if (!(b.one_minus_lambda_lower > 0.0)) {
        o.reject(Status::invalid_budget); return false;
    }
    b.box_R = o.upper_div(b.a_upper,b.one_minus_lambda_lower);
    if (!o.ok()) return false;
    for (unsigned i = 0; i < x.j; ++i) {
        // prefix_abs_sum is UNscaled. prefix_scaled bounds R*sum(abs(Aij)).
        double prefix_abs_sum = absolute(x.A[i][0]);
        double edot = o.e(o.upper_mul(prefix_abs_sum,b.box_R));
        for (unsigned j = 1; j < x.j; ++j) {
            prefix_abs_sum = o.upper_add(prefix_abs_sum,absolute(x.A[i][j]));
            const double ep = o.e(o.upper_mul(absolute(x.A[i][j]),b.box_R));
            const double previous_plus_product = o.upper_add(edot,ep);
            const double prefix_scaled = o.upper_mul(prefix_abs_sum,b.box_R);
            edot = o.upper_add(previous_plus_product,
                o.e(o.upper_add(prefix_scaled,previous_plus_product)));
        }
        b.dot_excess_upper[i] = edot;
        const double sr = o.upper_mul(b.row_sum_upper[i],b.box_R);
        const double ag = o.upper_add(absolute(x.dfree[i]),sr);
        b.gap_excess_upper[i] = o.upper_add(edot,o.e(o.upper_add(ag,edot)));
        b.endpoint_P = maximum(b.endpoint_P,absolute(x.d0[i]));
        b.endpoint_P = maximum(b.endpoint_P,o.upper_add(ag,b.gap_excess_upper[i]));
        if (!o.ok()) return false;
    }
    b.scaled_Q = o.upper_mul(half_stiffness,b.endpoint_P);
    b.scaled_Q2 = o.upper_mul(2.0,b.scaled_Q);
    const double et = o.e(1.0);
    const double ed = o.upper_add(et,o.e(2.0));
    const double er = o.upper_add(o.upper_add(et,ed),o.e(1.0));
    const double es = o.upper_add(o.upper_mul(b.endpoint_P,er),o.e(b.endpoint_P));
    const double emixed = o.upper_add(o.upper_mul(half_stiffness,es),o.e(b.scaled_Q));
    const double epos = o.upper_add(o.upper_mul(2.0,o.e(b.scaled_Q)),o.e(b.scaled_Q2));
    b.scalar_excess_upper = maximum(epos,emixed);
    for (unsigned i = 0; i < x.j; ++i) {
        const double w = o.upper_add(o.upper_mul(stiffness,b.gap_excess_upper[i]),
                                      o.upper_mul(2.0,b.scalar_excess_upper));
        b.width_W = maximum(b.width_W,w);
    }
    const double one_plus = o.upper_add(1.0,b.local_lambda_upper);
    b.recurrence_b_upper = o.upper_div(o.upper_mul(one_plus,b.width_W),
                                      b.one_minus_lambda_lower);
    b.residual_C_upper = o.upper_add(b.width_W,o.e(o.upper_add(b.box_R,b.width_W)));
    const double s1 = o.sub_point(residual_rail,b.recurrence_b_upper).lo;
    b.slack_lower = o.sub_point(s1,b.residual_C_upper).lo;
    if (!o.ok()) return false;
    if (!(b.slack_lower > 0.0)) {
        o.reject(Status::nonpositive_slack); return false;
    }
    return derive_cap(b,o);
}

// Ordinary C++ scalar graph for the inherited Python DG formula, explicitly
// not a NumPy/BLAS bit-identity oracle. It never changes the selected force.
inline double legacy_g(double a,double b) noexcept {
    if (a <= 0.0 && b <= 0.0) return 0.0;
    if (a > 0.0 && b > 0.0) return rn_mul(half_stiffness,rn_add(a,b));
    const double p = a > 0.0 ? a : b;
    const double gap = a > 0.0 ? rn_sub(a,b) : rn_sub(b,a);
    return rn_div(rn_mul(half_stiffness,rn_mul(p,p)),gap);
}
inline double legacy_residual(const Input& x,const std::array<double,maximum_contacts>& f) noexcept {
    double out = 0.0;
    for (unsigned i = 0; i < x.j; ++i) {
        double dot = rn_mul(x.A[i][0],f[0]);
        for (unsigned j = 1; j < x.j; ++j)
            dot = rn_add(dot,rn_mul(x.A[i][j],f[j]));
        const double g = legacy_g(x.d0[i],rn_sub(x.dfree[i],dot));
        const double r = absolute(rn_sub(f[i],g));
        if (!finite(g) || !finite(r)) return from_bits(UINT64_C(0x7ff8000000000000));
        out = maximum(out,r);
    }
    return out;
}

} // namespace detail

inline void record_exit_environment(Result& r) noexcept {
    if (!detail::environment_ok(r.exit_fpcr)) {
        r.status = Status::arithmetic_environment;
        r.analytic_zero_solution = false;
        r.exact_stored_residual_certified = false;
        r.cpp_legacy_diagnostic_pass = false;
        r.posterior_error_bound_valid = false;
        r.budgets.cap_bound_certified = false;
        r.posterior_error_upper = std::numeric_limits<double>::quiet_NaN();
    }
}

inline Result solve(const Input& x,Trace* trace = nullptr) noexcept {
    Result r;
    if (trace != nullptr) trace->used = 0;
    if (x.j > maximum_contacts) { r.status = Status::dimension; return r; }
    if (!detail::host_supported()) { r.status = Status::unsupported_host; return r; }
    if (!detail::environment_ok(r.entry_fpcr)) { r.status = Status::arithmetic_environment; return r; }
    if (x.j == 0) {
        r.status = Status::empty_system;
        r.analytic_zero_solution = true;
        r.exact_stored_residual_certified = true;
        r.cpp_legacy_diagnostic_pass = true;
        r.posterior_error_upper = 0.0;
        r.posterior_error_bound_valid = true;
        record_exit_environment(r);
        return r;
    }
    for (unsigned i = 0; i < x.j; ++i) {
        if (!detail::finite(x.d0[i]) || !detail::finite(x.dfree[i])) {
            r.status = Status::nonfinite_input; record_exit_environment(r); return r;
        }
        for (unsigned j = 0; j < x.j; ++j) if (!detail::finite(x.A[i][j])) {
            r.status = Status::nonfinite_input; record_exit_environment(r); return r;
        }
    }
    detail::Ops o;
    if (!detail::prepare_budgets(x,r.budgets,o)) {
        r.status = o.failure; r.primitive_operations = o.count;
        r.map_evaluations = o.map_count; record_exit_environment(r); return r;
    }
    r.analytic_zero_solution = r.budgets.a_upper == 0.0;
    const double Qcap = r.analytic_zero_solution ? std::numeric_limits<double>::max() : r.budgets.scaled_Q;
    const double Pcap = r.analytic_zero_solution ? std::numeric_limits<double>::max() : r.budgets.endpoint_P;
    for (unsigned k = 0; k <= r.budgets.cap_updates; ++k) {
        Iteration it;
        it.force = r.force; it.updates = k;
        it.mapped = detail::map(x,r.force,Qcap,Pcap,o);
        for (unsigned i = 0; i < x.j; ++i) {
            if (!detail::finite(r.force[i]) || r.force[i] < 0.0) {
                o.reject(Status::negative_or_nonfinite_candidate); break;
            }
            if (r.force[i] > r.budgets.box_R) { o.reject(Status::force_box); break; }
            const double width = o.sub_point(it.mapped[i].hi,it.mapped[i].lo).hi;
            it.measured_width_upper = detail::maximum(it.measured_width_upper,width);
            if (width > r.budgets.width_W) { o.reject(Status::interval_width); break; }
            const auto lower = o.sub_point(r.force[i],it.mapped[i].hi);
            const auto upper = o.sub_point(r.force[i],it.mapped[i].lo);
            it.residual[i] = {lower.lo,upper.hi};
            if (!o.valid(it.residual[i])) break;
            it.certificate_residual = detail::maximum(it.certificate_residual,
                detail::maximum(detail::absolute(it.residual[i].lo),detail::absolute(it.residual[i].hi)));
        }
        r.final_iteration = it;
        if (trace != nullptr) {
            if (trace->used >= trace->rows.size()) {
                o.reject(Status::trace_capacity); break;
            }
            trace->rows[trace->used++] = it;
        }
        if (!o.ok()) break;
        if (it.certificate_residual <= residual_rail) {
            r.exact_stored_residual_certified = true;
            r.posterior_error_upper = r.analytic_zero_solution ? 0.0
                : o.upper_div(it.certificate_residual,r.budgets.one_minus_lambda_lower);
            r.posterior_error_bound_valid = o.ok();
            r.cpp_legacy_residual = detail::legacy_residual(x,r.force);
            if (!detail::finite(r.cpp_legacy_residual)) o.reject(Status::nonfinite_cpp_diagnostic);
            else if (r.cpp_legacy_residual > residual_rail) o.reject(Status::cpp_legacy_residual);
            else r.cpp_legacy_diagnostic_pass = true;
            break;
        }
        if (k == r.budgets.cap_updates) { o.reject(Status::exhausted_cap); break; }
        for (unsigned i = 0; i < x.j; ++i) {
            const double candidate = it.mapped[i].lo;
            if (!detail::finite(candidate) || candidate < 0.0) {
                o.reject(Status::negative_or_nonfinite_candidate); break;
            }
            if (candidate > r.budgets.box_R) { o.reject(Status::force_box); break; }
        }
        if (!o.ok()) break;
        for (unsigned i = 0; i < x.j; ++i) r.force[i] = it.mapped[i].lo;
        ++r.actual_updates;
    }
    r.status = o.failure;
    r.primitive_operations = o.count;
    r.map_evaluations = o.map_count;
    record_exit_environment(r);
    return r;
}

} // namespace electry_multicontact_research
