#include "util.hpp"

std::string ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    assert(file.is_open());

    std::ostringstream ss;
    ss << file.rdbuf();
    assert(!file.bad());

    return ss.str();
}

