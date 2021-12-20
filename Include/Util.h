#pragma once

#include <array>
#include <algorithm>
#include <string>
#include "types.h"

namespace File {
    void writeAllBytes(const std::string &, const u8*, u32);
    u8* readAllBytes(const std::string &, u32*);
}