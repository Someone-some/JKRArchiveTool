#pragma once

#include <string>
#include "BinaryReader.h"

namespace Yaz0 {
    u8* decomp(const std::string &);
    u32 encodeSimple(u8 *, s32, s32, u32 *);
};