/* Copyright 2022 the GurupiaReader project authors (see AUTHORS file).
   License: GPLv3 */

#include "utils/BaseUtil.h"
#include "EncodingInfo.h"

// Comprehensive list of supported encodings
static EncodingInfo gEncodings[] = {
    // Unicode encodings (most common)
    {CP_UTF8, "UTF-8", "Unicode (UTF-8)", nullptr, true},
    {1200, "UTF-16LE", "Unicode (UTF-16 LE)", nullptr, true},
    {1201, "UTF-16BE", "Unicode (UTF-16 BE)", nullptr, false},
    {12000, "UTF-32LE", "Unicode (UTF-32 LE)", nullptr, false},
    {12001, "UTF-32BE", "Unicode (UTF-32 BE)", nullptr, false},

    // Western European
    {1252, "Windows-1252", "Western European (Windows-1252)", nullptr, true},
    {28591, "ISO-8859-1", "Western European (ISO-8859-1)", nullptr, false},
    {28592, "ISO-8859-2", "Central European (ISO-8859-2)", nullptr, false},
    {28605, "ISO-8859-15", "Western European (ISO-8859-15)", nullptr, false},

    // East Asian - Korean
    {949, "Windows-949", "Korean (Windows-949)", nullptr, true},
    {51949, "EUC-KR", "Korean (EUC-KR)", nullptr, true},

    // East Asian - Japanese
    {932, "Shift-JIS", "Japanese (Shift-JIS)", nullptr, true},
    {51932, "EUC-JP", "Japanese (EUC-JP)", nullptr, false},
    {50220, "ISO-2022-JP", "Japanese (ISO-2022-JP)", nullptr, false},

    // East Asian - Chinese Simplified
    {936, "GBK", "Chinese Simplified (GBK)", nullptr, true},
    {54936, "GB18030", "Chinese Simplified (GB18030)", nullptr, false},
    {20936, "GB2312", "Chinese Simplified (GB2312)", nullptr, false},

    // East Asian - Chinese Traditional
    {950, "Big5", "Chinese Traditional (Big5)", nullptr, true},
    {20000, "Big5-HKSCS", "Chinese Traditional (Big5-HKSCS)", nullptr, false},

    // Cyrillic
    {1251, "Windows-1251", "Cyrillic (Windows-1251)", nullptr, true},
    {20866, "KOI8-R", "Cyrillic (KOI8-R)", nullptr, false},
    {21866, "KOI8-U", "Cyrillic (KOI8-U)", nullptr, false},
    {28595, "ISO-8859-5", "Cyrillic (ISO-8859-5)", nullptr, false},

    // Greek
    {1253, "Windows-1253", "Greek (Windows-1253)", nullptr, false},
    {28597, "ISO-8859-7", "Greek (ISO-8859-7)", nullptr, false},

    // Turkish
    {1254, "Windows-1254", "Turkish (Windows-1254)", nullptr, false},
    {28599, "ISO-8859-9", "Turkish (ISO-8859-9)", nullptr, false},

    // Hebrew
    {1255, "Windows-1255", "Hebrew (Windows-1255)", nullptr, false},
    {28598, "ISO-8859-8", "Hebrew (ISO-8859-8)", nullptr, false},

    // Arabic
    {1256, "Windows-1256", "Arabic (Windows-1256)", nullptr, false},
    {28596, "ISO-8859-6", "Arabic (ISO-8859-6)", nullptr, false},

    // Thai
    {874, "Windows-874", "Thai (Windows-874)", nullptr, false},
    {28605, "ISO-8859-11", "Thai (ISO-8859-11)", nullptr, false},

    // Vietnamese
    {1258, "Windows-1258", "Vietnamese (Windows-1258)", nullptr, false},

    // Baltic
    {1257, "Windows-1257", "Baltic (Windows-1257)", nullptr, false},
    {28594, "ISO-8859-4", "Baltic (ISO-8859-4)", nullptr, false},

    // Central European
    {1250, "Windows-1250", "Central European (Windows-1250)", nullptr, false},

    // System default
    {CP_ACP, "ANSI", "System Default (ANSI)", nullptr, false},
};

Vec<EncodingInfo> EncodingRegistry::GetAllEncodings() {
    Vec<EncodingInfo> result;
    for (size_t i = 0; i < dimof(gEncodings); i++) {
        result.Append(gEncodings[i]);
    }
    return result;
}

Vec<EncodingInfo> EncodingRegistry::GetCommonEncodings() {
    Vec<EncodingInfo> result;
    for (size_t i = 0; i < dimof(gEncodings); i++) {
        if (gEncodings[i].isCommon) {
            result.Append(gEncodings[i]);
        }
    }
    return result;
}

EncodingInfo* EncodingRegistry::GetEncodingInfo(uint codepage) {
    for (size_t i = 0; i < dimof(gEncodings); i++) {
        if (gEncodings[i].codepage == codepage) {
            return &gEncodings[i];
        }
    }
    return nullptr;
}

uint EncodingRegistry::GetCodepageByName(const char* name) {
    if (!name) {
        return 0;
    }

    for (size_t i = 0; i < dimof(gEncodings); i++) {
        if (str::EqI(gEncodings[i].name, name)) {
            return gEncodings[i].codepage;
        }
    }

    // Try common aliases
    if (str::EqI(name, "UTF8")) {
        return CP_UTF8;
    }
    if (str::EqI(name, "UTF-16")) {
        return 1200;
    }
    if (str::EqI(name, "UCS-2")) {
        return 1200;
    }
    if (str::EqI(name, "UNICODE")) {
        return 1200;
    }
    if (str::EqI(name, "ASCII") || str::EqI(name, "US-ASCII")) {
        return 20127; // US-ASCII
    }

    return 0;
}

uint EncodingRegistry::GetDefaultCodepage() {
    // Get system default ANSI codepage
    return GetACP();
}

const char* EncodingRegistry::GetDisplayName(uint codepage) {
    EncodingInfo* info = GetEncodingInfo(codepage);
    if (info) {
        return info->displayName;
    }

    // Fallback for unknown codepages
    static char buf[64];
    str::Format(buf, sizeof(buf), "Codepage %u", codepage);
    return buf;
}
