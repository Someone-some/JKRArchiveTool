#pragma once

#include <string>
#include "BinaryReaderAndWriter.h"

enum JKRCompressionType {
    JKRCompressionType_NONE = 0x0,
    JKRCompressionType_SZP = 0x1,
    JKRCompressionType_SZS = 0x2,
    JKRCompressionType_ASR = 0x3
};

namespace JKRCompression {
    JKRCompressionType checkCompression(const std::string &);
    u8* decode(const std::string &, u32 *);
    void encode(const std::string &, JKRCompressionType, bool);

    u8* decodeSZS(const u8*, u32);
    u8* decodeSZP(const u8*, u32);
    u32 encodeSimpleSZS(u8 *, s32, s32, u32 *);
    u32 encodeAdvancedSZS(u8 *, s32, s32, u32 *);
    void encodeSZS(const std::string &);
    void fastEncodeSZS(const std::string &);
    void encodeSZP(const std::string &);
};