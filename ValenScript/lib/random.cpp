#include <cstdint>
#include <random>
#include <stdexcept>

namespace {
std::mt19937_64& randomEngine() {
    static std::mt19937_64 eng(std::random_device{}());
    return eng;
}
} // namespace

namespace random_lib {

void seed(std::uint64_t value) { randomEngine().seed(value); }

int randint(int minVal, int maxVal) {
    if (minVal > maxVal) {
        throw std::runtime_error("random.int[] expects min <= max");
    }
    std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(randomEngine());
}

double randfloat(double minVal, double maxVal) {
    if (minVal > maxVal) {
        throw std::runtime_error("random.float[] expects min <= max");
    }
    std::uniform_real_distribution<double> dist(minVal, maxVal);
    return dist(randomEngine());
}

int randindex(int maxExclusive) {
    if (maxExclusive <= 0) {
        throw std::runtime_error("random.choice[] expects non-empty list");
    }
    std::uniform_int_distribution<int> dist(0, maxExclusive - 1);
    return dist(randomEngine());
}

} // namespace random_lib
