#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace conversions_lib {

static std::string normalize(std::string s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isalnum(uc)) out.push_back(static_cast<char>(std::tolower(uc)));
        else if (c == '_' || c == '-' || c == ' ') out.push_back('_');
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    size_t start = 0;
    while (start < out.size() && out[start] == '_') ++start;
    if (start > 0) out = out.substr(start);
    return out;
}

static double mapConvert(
    const std::unordered_map<std::string, double>& factorToBase,
    const std::string& from,
    const std::string& to,
    double value,
    const std::string& name) {
    auto f = factorToBase.find(normalize(from));
    auto t = factorToBase.find(normalize(to));
    if (f == factorToBase.end()) throw std::runtime_error(name + ": unknown from unit '" + from + "'");
    if (t == factorToBase.end()) throw std::runtime_error(name + ": unknown to unit '" + to + "'");
    return value * f->second / t->second;
}

double temperature(const std::string& from, const std::string& to, double value) {
    auto toKelvin = [](const std::string& u, double v) {
        const std::string n = normalize(u);
        if (n == "c" || n == "celsius" || n == "centigrade") return v + 273.15;
        if (n == "f" || n == "fahrenheit" || n == "fahrenheir") return (v + 459.67) * 5.0 / 9.0;
        if (n == "k" || n == "kelvin") return v;
        if (n == "r" || n == "rankine") return v * 5.0 / 9.0;
        throw std::runtime_error("temperature: unknown unit '" + u + "'");
    };
    auto fromKelvin = [](const std::string& u, double k) {
        const std::string n = normalize(u);
        if (n == "c" || n == "celsius" || n == "centigrade") return k - 273.15;
        if (n == "f" || n == "fahrenheit" || n == "fahrenheir") return k * 9.0 / 5.0 - 459.67;
        if (n == "k" || n == "kelvin") return k;
        if (n == "r" || n == "rankine") return k * 9.0 / 5.0;
        throw std::runtime_error("temperature: unknown unit '" + u + "'");
    };
    return fromKelvin(to, toKelvin(from, value));
}

double length(const std::string& from, const std::string& to, double value) {
    static const std::unordered_map<std::string, double> m = {
        {"m", 1.0}, {"meter", 1.0}, {"meters", 1.0}, {"metre", 1.0}, {"metres", 1.0},
        {"km", 1000.0}, {"kilometer", 1000.0}, {"kilometers", 1000.0}, {"kilometre", 1000.0},
        {"cm", 0.01}, {"centimeter", 0.01}, {"centimeters", 0.01},
        {"mm", 0.001}, {"millimeter", 0.001}, {"millimeters", 0.001},
        {"in", 0.0254}, {"inch", 0.0254}, {"inches", 0.0254},
        {"ft", 0.3048}, {"foot", 0.3048}, {"feet", 0.3048},
        {"yd", 0.9144}, {"yard", 0.9144}, {"yards", 0.9144},
        {"mi", 1609.344}, {"mile", 1609.344}, {"miles", 1609.344},
        {"nmi", 1852.0}, {"nauticalmile", 1852.0}, {"nautical_mile", 1852.0}
    };
    return mapConvert(m, from, to, value, "length");
}

double weight(const std::string& from, const std::string& to, double value) {
    static const std::unordered_map<std::string, double> kg = {
        {"kg", 1.0}, {"kilogram", 1.0}, {"kilograms", 1.0},
        {"g", 0.001}, {"gram", 0.001}, {"grams", 0.001},
        {"mg", 0.000001}, {"milligram", 0.000001}, {"milligrams", 0.000001},
        {"lb", 0.45359237}, {"lbs", 0.45359237}, {"pound", 0.45359237}, {"pounds", 0.45359237},
        {"oz", 0.028349523125}, {"ounce", 0.028349523125}, {"ounces", 0.028349523125},
        {"stone", 6.35029318}, {"st", 6.35029318},
        {"tonne", 1000.0}, {"metricton", 1000.0}, {"metric_ton", 1000.0},
        {"ton", 907.18474}, {"us_ton", 907.18474}
    };
    return mapConvert(kg, from, to, value, "weight");
}

double time(const std::string& from, const std::string& to, double value) {
    static const std::unordered_map<std::string, double> sec = {
        {"s", 1.0}, {"sec", 1.0}, {"second", 1.0}, {"seconds", 1.0},
        {"ms", 0.001}, {"millisecond", 0.001}, {"milliseconds", 0.001},
        {"min", 60.0}, {"minute", 60.0}, {"minutes", 60.0},
        {"h", 3600.0}, {"hr", 3600.0}, {"hour", 3600.0}, {"hours", 3600.0},
        {"day", 86400.0}, {"days", 86400.0}, {"d", 86400.0},
        {"week", 604800.0}, {"weeks", 604800.0}, {"wk", 604800.0}
    };
    return mapConvert(sec, from, to, value, "time");
}

double area(const std::string& from, const std::string& to, double value) {
    static const std::unordered_map<std::string, double> sqm = {
        {"m2", 1.0}, {"squaremeter", 1.0}, {"square_meter", 1.0},
        {"km2", 1000000.0}, {"squarekilometer", 1000000.0}, {"square_kilometer", 1000000.0},
        {"cm2", 0.0001}, {"squarecentimeter", 0.0001}, {"square_centimeter", 0.0001},
        {"mm2", 0.000001}, {"squaremillimeter", 0.000001}, {"square_millimeter", 0.000001},
        {"ft2", 0.09290304}, {"squarefoot", 0.09290304}, {"square_foot", 0.09290304},
        {"yd2", 0.83612736}, {"squareyard", 0.83612736}, {"square_yard", 0.83612736},
        {"acre", 4046.8564224}, {"acres", 4046.8564224},
        {"hectare", 10000.0}, {"hectares", 10000.0}, {"ha", 10000.0}
    };
    return mapConvert(sqm, from, to, value, "area");
}

double volume(const std::string& from, const std::string& to, double value) {
    static const std::unordered_map<std::string, double> liter = {
        {"l", 1.0}, {"liter", 1.0}, {"liters", 1.0}, {"litre", 1.0}, {"litres", 1.0},
        {"ml", 0.001}, {"milliliter", 0.001}, {"milliliters", 0.001},
        {"m3", 1000.0}, {"cubicmeter", 1000.0}, {"cubic_meter", 1000.0},
        {"gallon", 3.785411784}, {"gallon_us", 3.785411784}, {"gal", 3.785411784},
        {"quart", 0.946352946}, {"quart_us", 0.946352946}, {"qt", 0.946352946},
        {"pint", 0.473176473}, {"pint_us", 0.473176473}, {"pt", 0.473176473},
        {"cup", 0.2365882365}, {"cup_us", 0.2365882365},
        {"floz", 0.0295735295625}, {"fluidounce", 0.0295735295625}, {"fluid_ounce_us", 0.0295735295625}
    };
    return mapConvert(liter, from, to, value, "volume");
}

double speed(const std::string& from, const std::string& to, double value) {
    static const std::unordered_map<std::string, double> mps = {
        {"mps", 1.0}, {"m_s", 1.0}, {"meterpersecond", 1.0}, {"meter_per_second", 1.0},
        {"kph", 0.2777777777777778}, {"kmh", 0.2777777777777778}, {"kilometerperhour", 0.2777777777777778},
        {"mph", 0.44704}, {"mileperhour", 0.44704},
        {"knot", 0.5144444444444445}, {"knots", 0.5144444444444445}, {"kts", 0.5144444444444445},
        {"fps", 0.3048}, {"footpersecond", 0.3048}
    };
    return mapConvert(mps, from, to, value, "speed");
}

double data(const std::string& from, const std::string& to, double value) {
    static const std::unordered_map<std::string, double> bytes = {
        {"bit", 0.125}, {"bits", 0.125},
        {"byte", 1.0}, {"bytes", 1.0}, {"b", 1.0},
        {"kb", 1000.0}, {"mb", 1000000.0}, {"gb", 1000000000.0}, {"tb", 1000000000000.0},
        {"kib", 1024.0}, {"mib", 1048576.0}, {"gib", 1073741824.0}, {"tib", 1099511627776.0}
    };
    return mapConvert(bytes, from, to, value, "data");
}

double convert(const std::string& category, const std::string& from, const std::string& to, double value) {
    const std::string c = normalize(category);
    if (c == "temperature" || c == "temp") return temperature(from, to, value);
    if (c == "length" || c == "distance") return length(from, to, value);
    if (c == "weight" || c == "mass") return weight(from, to, value);
    if (c == "time") return time(from, to, value);
    if (c == "area") return area(from, to, value);
    if (c == "volume") return volume(from, to, value);
    if (c == "speed") return speed(from, to, value);
    if (c == "data" || c == "storage") return data(from, to, value);
    throw std::runtime_error("convert: unknown category '" + category + "'");
}

} // namespace conversions_lib
