#pragma once

#include <string>
#include "BinaryReader.h"

namespace Yaz0 {
    u8* decomp(const std::string &, u32 *, bool);
    // My Yaz0 compression algo sucked so I stole it from Yaz0enc
    u32 compSimple(u8 *, s32, s32, u32 *);
    u32 compAdvanced(u8 *, s32, s32, u32 *);
    void comp(const std::string &);

    struct Ret {
        s32 mSrcPos;
        s32 mDstPos;
    };
};