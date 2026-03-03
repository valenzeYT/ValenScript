#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace math_lib {

static constexpr double PI_VAL = 3.14159265358979323846;
static constexpr double E_VAL = 2.71828182845904523536;

double factorial(double n) {
    if (n < 0) throw std::runtime_error("factorial expects non-negative integer");
    if (std::floor(n) != n) throw std::runtime_error("factorial expects integer");
    double result = 1.0;
    for (int i = 1; i <= static_cast<int>(n); ++i) result *= i;
    return result;
}

double abs_val(double n) { return std::fabs(n); }
double sqrt_val(double n) {
    if (n < 0) throw std::runtime_error("sqrt expects non-negative number");
    return std::sqrt(n);
}
double pow_val(double a, double b) { return std::pow(a, b); }
double floor_val(double n) { return std::floor(n); }
double ceil_val(double n) { return std::ceil(n); }
double round_val(double n) { return std::round(n); }
double mod_val(double a, double b) {
    if (b == 0.0) throw std::runtime_error("mod division by zero");
    return std::fmod(a, b);
}
bool is_even_val(int n) { return (n % 2) == 0; }
bool is_odd_val(int n) { return (n % 2) != 0; }

double min_val(double a, double b) { return (a < b) ? a : b; }
double max_val(double a, double b) { return (a > b) ? a : b; }
double clamp_val(double v, double lo, double hi) {
    if (lo > hi) std::swap(lo, hi);
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
double lerp_val(double a, double b, double t) { return a + (b - a) * t; }
double sin_val(double n) { return std::sin(n); }
double cos_val(double n) { return std::cos(n); }
double tan_val(double n) { return std::tan(n); }
double asin_val(double n) {
    if (n < -1.0 || n > 1.0) throw std::runtime_error("asin expects value in [-1,1]");
    return std::asin(n);
}
double acos_val(double n) {
    if (n < -1.0 || n > 1.0) throw std::runtime_error("acos expects value in [-1,1]");
    return std::acos(n);
}
double atan_val(double n) { return std::atan(n); }
double atan2_val(double y, double x) { return std::atan2(y, x); }
double log_val(double n) {
    if (n <= 0.0) throw std::runtime_error("log expects positive number");
    return std::log(n);
}
double log10_val(double n) {
    if (n <= 0.0) throw std::runtime_error("log10 expects positive number");
    return std::log10(n);
}
double exp_val(double n) { return std::exp(n); }
double cbrt_val(double n) { return std::cbrt(n); }
double hypot_val(double a, double b) { return std::hypot(a, b); }
double sign_val(double n) { return (n > 0.0) ? 1.0 : ((n < 0.0) ? -1.0 : 0.0); }
double trunc_val(double n) { return std::trunc(n); }
double frac_val(double n) { return n - std::floor(n); }
double gcd_val(double a, double b) {
    long long x = static_cast<long long>(std::llround(a));
    long long y = static_cast<long long>(std::llround(b));
    x = std::llabs(x);
    y = std::llabs(y);
    while (y != 0) {
        long long t = x % y;
        x = y;
        y = t;
    }
    return static_cast<double>(x);
}
double lcm_val(double a, double b) {
    long long x = static_cast<long long>(std::llround(a));
    long long y = static_cast<long long>(std::llround(b));
    x = std::llabs(x);
    y = std::llabs(y);
    if (x == 0 || y == 0) return 0.0;
    long long g = static_cast<long long>(gcd_val(static_cast<double>(x), static_cast<double>(y)));
    return static_cast<double>((x / g) * y);
}
double deg2rad_val(double deg) { return deg * PI_VAL / 180.0; }
double rad2deg_val(double rad) { return rad * 180.0 / PI_VAL; }
double pi_val() { return PI_VAL; }

} // namespace math_lib
