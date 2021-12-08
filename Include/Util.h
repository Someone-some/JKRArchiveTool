#pragma once

#include <array>
#include <algorithm>
#include <string>
#include "types.h"

namespace Util {
    template <typename T>
    void SwapEndian(T &val) {
        union U {
            T val;
            std::array<u8, sizeof(T)> raw;
        } src, dst;

        src.val = val;
        std::reverse_copy(src.raw.begin(), src.raw.end(), dst.raw.begin());
        val = dst.val;
    }
};

namespace File {
    void writeAllBytes(const std::string &, const u8*);
}