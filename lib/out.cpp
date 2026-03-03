#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace out_lib {

void configure_stdout() {
    std::cout << std::fixed << std::setprecision(std::numeric_limits<double>::max_digits10);
}

void print_line(const std::vector<std::string>& parts) {
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << parts[i];
    }
    std::cout << "\n";
    std::cout.flush();
}

void write(const std::string& text) {
    std::cout << text;
}

void flush() {
    std::cout.flush();
}

} // namespace out_lib
