#include "../include/module_registry.h"
#include "../include/interpreter.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <vector>

namespace math_lib {

// CONSTANTS
static constexpr double PI_VAL = 3.14159265358979323846;
static constexpr double E_VAL = 2.71828182845904523536;
static constexpr double TAU_VAL = 2 * PI_VAL;
static constexpr double PHI_VAL = (1 + std::sqrt(5))/2;

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
double tau_val() { return TAU_VAL; }
double e_val() { return E_VAL; }
double phi_val() { return PHI_VAL; }
} // namespace math_lib

extern "C" __declspec(dllexport)
void register_module() {
    module_registry::registerModule("math", [](Interpreter& interp) {
        interp.registerModuleFunction("math", "factorial",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.factorial");
                                          return Value::fromNumber(
                                              math_lib::factorial(interp.expectNumber(
                                                  args[0], "math.factorial expects number")));
                                      });
        interp.registerModuleFunction("math", "abs",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.abs");
                                          return Value::fromNumber(
                                              math_lib::abs_val(interp.expectNumber(
                                                  args[0], "math.abs expects number")));
                                      });
        interp.registerModuleFunction("math", "sqrt",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.sqrt");
                                          return Value::fromNumber(
                                              math_lib::sqrt_val(interp.expectNumber(
                                                  args[0], "math.sqrt expects number")));
                                      });
        interp.registerModuleFunction("math", "pow",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 2, "math.pow");
                                          return Value::fromNumber(
                                              math_lib::pow_val(
                                                  interp.expectNumber(args[0], "math.pow expects number"),
                                                  interp.expectNumber(args[1], "math.pow expects number")));
                                      });
        interp.registerModuleFunction("math", "floor",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.floor");
                                          return Value::fromNumber(
                                              math_lib::floor_val(interp.expectNumber(
                                                  args[0], "math.floor expects number")));
                                      });
        interp.registerModuleFunction("math", "ceil",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.ceil");
                                          return Value::fromNumber(
                                              math_lib::ceil_val(interp.expectNumber(
                                                  args[0], "math.ceil expects number")));
                                      });
        interp.registerModuleFunction("math", "round",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.round");
                                          return Value::fromNumber(
                                              math_lib::round_val(interp.expectNumber(
                                                  args[0], "math.round expects number")));
                                      });
        interp.registerModuleFunction("math", "mod",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 2, "math.mod");
                                          return Value::fromNumber(
                                              math_lib::mod_val(
                                                  interp.expectNumber(args[0], "math.mod expects number"),
                                                  interp.expectNumber(args[1], "math.mod expects number")));
                                      });
        interp.registerModuleFunction("math", "is_even",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.is_even");
                                          int n = static_cast<int>(interp.expectNumber(
                                              args[0], "math.is_even expects number"));
                                          return Value::fromBool(math_lib::is_even_val(n));
                                      });
        interp.registerModuleFunction("math", "is_odd",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.is_odd");
                                          int n = static_cast<int>(interp.expectNumber(
                                              args[0], "math.is_odd expects number"));
                                          return Value::fromBool(math_lib::is_odd_val(n));
                                      });
        interp.registerModuleFunction("math", "min",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 2, "math.min");
                                          return Value::fromNumber(
                                              math_lib::min_val(
                                                  interp.expectNumber(args[0], "math.min expects number"),
                                                  interp.expectNumber(args[1], "math.min expects number")));
                                      });
        interp.registerModuleFunction("math", "max",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 2, "math.max");
                                          return Value::fromNumber(
                                              math_lib::max_val(
                                                  interp.expectNumber(args[0], "math.max expects number"),
                                                  interp.expectNumber(args[1], "math.max expects number")));
                                      });
        interp.registerModuleFunction("math", "clamp",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 3, "math.clamp");
                                          return Value::fromNumber(
                                              math_lib::clamp_val(
                                                  interp.expectNumber(args[0], "math.clamp expects number"),
                                                  interp.expectNumber(args[1], "math.clamp expects number"),
                                                  interp.expectNumber(args[2], "math.clamp expects number")));
                                      });
        interp.registerModuleFunction("math", "lerp",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 3, "math.lerp");
                                          return Value::fromNumber(
                                              math_lib::lerp_val(
                                                  interp.expectNumber(args[0], "math.lerp expects number"),
                                                  interp.expectNumber(args[1], "math.lerp expects number"),
                                                  interp.expectNumber(args[2], "math.lerp expects number")));
                                      });
        interp.registerModuleFunction("math", "sin",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.sin");
                                          return Value::fromNumber(
                                              math_lib::sin_val(interp.expectNumber(
                                                  args[0], "math.sin expects number")));
                                      });
        interp.registerModuleFunction("math", "cos",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.cos");
                                          return Value::fromNumber(
                                              math_lib::cos_val(interp.expectNumber(
                                                  args[0], "math.cos expects number")));
                                      });
        interp.registerModuleFunction("math", "tan",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.tan");
                                          return Value::fromNumber(
                                              math_lib::tan_val(interp.expectNumber(
                                                  args[0], "math.tan expects number")));
                                      });
        interp.registerModuleFunction("math", "asin",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.asin");
                                          return Value::fromNumber(
                                              math_lib::asin_val(interp.expectNumber(
                                                  args[0], "math.asin expects number")));
                                      });
        interp.registerModuleFunction("math", "acos",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.acos");
                                          return Value::fromNumber(
                                              math_lib::acos_val(interp.expectNumber(
                                                  args[0], "math.acos expects number")));
                                      });
        interp.registerModuleFunction("math", "atan",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.atan");
                                          return Value::fromNumber(
                                              math_lib::atan_val(interp.expectNumber(
                                                  args[0], "math.atan expects number")));
                                      });
        interp.registerModuleFunction("math", "atan2",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 2, "math.atan2");
                                          return Value::fromNumber(
                                              math_lib::atan2_val(
                                                  interp.expectNumber(args[0], "math.atan2 expects number"),
                                                  interp.expectNumber(args[1], "math.atan2 expects number")));
                                      });
        interp.registerModuleFunction("math", "log",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.log");
                                          return Value::fromNumber(
                                              math_lib::log_val(interp.expectNumber(
                                                  args[0], "math.log expects number")));
                                      });
        interp.registerModuleFunction("math", "log10",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.log10");
                                          return Value::fromNumber(
                                              math_lib::log10_val(interp.expectNumber(
                                                  args[0], "math.log10 expects number")));
                                      });
        interp.registerModuleFunction("math", "exp",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.exp");
                                          return Value::fromNumber(
                                              math_lib::exp_val(interp.expectNumber(
                                                  args[0], "math.exp expects number")));
                                      });
        interp.registerModuleFunction("math", "cbrt",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.cbrt");
                                          return Value::fromNumber(
                                              math_lib::cbrt_val(interp.expectNumber(
                                                  args[0], "math.cbrt expects number")));
                                      });
        interp.registerModuleFunction("math", "hypot",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 2, "math.hypot");
                                          return Value::fromNumber(
                                              math_lib::hypot_val(
                                                  interp.expectNumber(args[0], "math.hypot expects number"),
                                                  interp.expectNumber(args[1], "math.hypot expects number")));
                                      });
        interp.registerModuleFunction("math", "sign",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.sign");
                                          return Value::fromNumber(
                                              math_lib::sign_val(interp.expectNumber(
                                                  args[0], "math.sign expects number")));
                                      });
        interp.registerModuleFunction("math", "trunc",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.trunc");
                                          return Value::fromNumber(
                                              math_lib::trunc_val(interp.expectNumber(
                                                  args[0], "math.trunc expects number")));
                                      });
        interp.registerModuleFunction("math", "frac",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.frac");
                                          return Value::fromNumber(
                                              math_lib::frac_val(interp.expectNumber(
                                                  args[0], "math.frac expects number")));
                                      });
        interp.registerModuleFunction("math", "gcd",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 2, "math.gcd");
                                          return Value::fromNumber(
                                              math_lib::gcd_val(
                                                  interp.expectNumber(args[0], "math.gcd expects number"),
                                                  interp.expectNumber(args[1], "math.gcd expects number")));
                                      });
        interp.registerModuleFunction("math", "lcm",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 2, "math.lcm");
                                          return Value::fromNumber(
                                              math_lib::lcm_val(
                                                  interp.expectNumber(args[0], "math.lcm expects number"),
                                                  interp.expectNumber(args[1], "math.lcm expects number")));
                                      });
        interp.registerModuleFunction("math", "deg2rad",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.deg2rad");
                                          return Value::fromNumber(
                                              math_lib::deg2rad_val(interp.expectNumber(
                                                  args[0], "math.deg2rad expects number")));
                                      });
        interp.registerModuleFunction("math", "rad2deg",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 1, "math.rad2deg");
                                          return Value::fromNumber(
                                              math_lib::rad2deg_val(interp.expectNumber(
                                                  args[0], "math.rad2deg expects number")));
                                      });
        interp.registerModuleFunction("math", "pi",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 0, "math.pi");
                                          return Value::fromNumber(math_lib::pi_val());
                                      });
        interp.registerModuleFunction("math", "tau",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 0, "math.tau");
                                          return Value::fromNumber(math_lib::tau_val());
                                      });
        interp.registerModuleFunction("math", "e",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 0, "math.e");
                                          return Value::fromNumber(math_lib::e_val());
                                      });
        interp.registerModuleFunction("math", "phi",
                                      [&interp](const std::vector<Value>& args) -> Value {
                                          interp.expectArity(args, 0, "math.phi");
                                          return Value::fromNumber(math_lib::phi_val());
                                      });
    });
}
