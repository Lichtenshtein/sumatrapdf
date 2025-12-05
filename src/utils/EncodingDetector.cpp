/* Copyright 2022 the GurupiaReader project authors (see AUTHORS file).
   License: GPLv3 */

#include "utils/BaseUtil.h"
#include "utils/StrUtil.h"
#include "EncodingDetector.h"
#include "EncodingInfo.h"

// BOM (Byte Order Mark) signatures
static const u8 kBOM_UTF8[] = {0xEF, 0xBB, 0xBF};
static const u8 kBOM_UTF16_LE[] = {0xFF, 0xFE};
static const u8 kBOM_UTF16_BE[] = {0xFE, 0xFF};
static const u8 kBOM_UTF32_LE[] = {0xFF, 0xFE, 0x00, 0x00};
static const u8 kBOM_UTF32_BE[] = {0x00, 0x00, 0xFE, 0xFF};

EncodingResult EncodingDetector::DetectFromBOM(const ByteSlice& data) {
    if (data.size() < 2) {
        return EncodingResult();
    }

    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    // Check UTF-32 first (4 bytes) before UTF-16 (2 bytes)
    if (len >= 4) {
        if (memeq(d, kBOM_UTF32_LE, 4)) {
            return EncodingResult(12000, EncodingConfidence::Certain, "UTF-32LE");
        }
        if (memeq(d, kBOM_UTF32_BE, 4)) {
            return EncodingResult(12001, EncodingConfidence::Certain, "UTF-32BE");
        }
    }

    // Check UTF-8 (3 bytes)
    if (len >= 3 && memeq(d, kBOM_UTF8, 3)) {
        return EncodingResult(CP_UTF8, EncodingConfidence::Certain, "UTF-8");
    }

    // Check UTF-16
    if (memeq(d, kBOM_UTF16_LE, 2)) {
        return EncodingResult(1200, EncodingConfidence::Certain, "UTF-16LE");
    }
    if (memeq(d, kBOM_UTF16_BE, 2)) {
        return EncodingResult(1201, EncodingConfidence::Certain, "UTF-16BE");
    }

    return EncodingResult();
}

EncodingResult EncodingDetector::DetectFromXMLPI(const ByteSlice& data) {
    // Look for <?xml encoding="..."?>
    const char* start = (const char*)data.data();
    size_t len = data.size();

    // Limit search to first 1KB
    if (len > 1024) {
        len = 1024;
    }

    const char* xmlDecl = str::Find(start, "<?xml");
    if (!xmlDecl || xmlDecl - start > 100) {
        return EncodingResult();
    }

    const char* encodingAttr = str::Find(xmlDecl, "encoding");
    if (!encodingAttr || encodingAttr - xmlDecl > 200) {
        return EncodingResult();
    }

    // Find the value in quotes
    const char* quote1 = str::FindChar(encodingAttr, '"');
    if (!quote1) {
        quote1 = str::FindChar(encodingAttr, '\'');
    }
    if (!quote1 || quote1 - encodingAttr > 20) {
        return EncodingResult();
    }

    quote1++; // Skip opening quote
    const char* quote2 = str::FindChar(quote1, quote1[-1]); // Find matching quote
    if (!quote2 || quote2 - quote1 > 50) {
        return EncodingResult();
    }

    // Extract encoding name
    AutoFree encodingName = str::Dup(quote1, quote2 - quote1);

    // Look up codepage
    uint codepage = EncodingRegistry::GetCodepageByName(encodingName);
    if (codepage != 0) {
        return EncodingResult(codepage, EncodingConfidence::High, encodingName);
    }

    return EncodingResult();
}

EncodingResult EncodingDetector::DetectFromHTMLMeta(const ByteSlice& data) {
    const char* start = (const char*)data.data();
    size_t len = data.size();

    // Limit search to first 2KB (meta tags should be in head)
    if (len > 2048) {
        len = 2048;
    }

    // Look for <meta charset="...">
    const char* metaCharset = str::Find(start, "charset");
    if (!metaCharset || metaCharset - start >= (int)len) {
        return EncodingResult();
    }

    // Try to find quoted value (charset="utf-8" or charset='utf-8')
    const char* quote1 = str::FindChar(metaCharset, '"');
    if (!quote1) {
        quote1 = str::FindChar(metaCharset, '\'');
    }

    if (quote1) {
        // Found quoted value - extract encoding name between quotes
        // Safe: quote1 is guaranteed non-NULL here
        quote1++;  // Skip opening quote
        const char* quote2 = str::FindChar(quote1, quote1[-1]);  // Find matching quote
        if (quote2 && quote2 > quote1) {
            AutoFree encodingName = str::Dup(quote1, quote2 - quote1);
            uint codepage = EncodingRegistry::GetCodepageByName(encodingName);
            if (codepage != 0) {
                return EncodingResult(codepage, EncodingConfidence::High, encodingName);
            }
        }
    } else {
        // No quotes found - try unquoted value (charset=utf-8)
        const char* eq = str::FindChar(metaCharset, '=');
        if (eq) {
            eq++;
            // Skip whitespace after '='
            while (*eq == ' ') {
                eq++;
            }
            // Find end of value
            const char* end = eq;
            while (*end && *end != ' ' && *end != '>' && *end != ';') {
                end++;
            }
            if (end > eq) {
                AutoFree encodingName = str::Dup(eq, end - eq);
                uint codepage = EncodingRegistry::GetCodepageByName(encodingName);
                if (codepage != 0) {
                    return EncodingResult(codepage, EncodingConfidence::High, encodingName);
                }
            }
        }
    }

    return EncodingResult();
}

EncodingResult EncodingDetector::DetectFromDeclaration(const ByteSlice& data) {
    // Try XML first
    EncodingResult result = DetectFromXMLPI(data);
    if (result.confidence != EncodingConfidence::None) {
        return result;
    }

    // Try HTML meta tags
    return DetectFromHTMLMeta(data);
}

bool EncodingDetector::IsValidUTF8(const ByteSlice& data) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();
    size_t i = 0;

    while (i < len) {
        u8 byte = d[i];

        // ASCII (0x00-0x7F) - single byte
        if (byte <= 0x7F) {
            i++;
            continue;
        }

        // 2-byte sequence (110xxxxx 10xxxxxx)
        // Valid range: U+0080 to U+07FF
        if ((byte & 0xE0) == 0xC0) {
            // Need 1 more byte (i+1 must be valid index)
            if (i + 1 >= len) {
                return false;  // Not enough bytes
            }
            // Second byte must be continuation byte (10xxxxxx)
            if ((d[i + 1] & 0xC0) != 0x80) {
                return false;
            }
            // Check for overlong encoding (must be >= 0xC2)
            // 0xC0 and 0xC1 would encode values < 0x80 (should be 1-byte)
            if (byte < 0xC2) {
                return false;
            }
            i += 2;
            continue;
        }

        // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        // Valid range: U+0800 to U+FFFF (excluding surrogates)
        if ((byte & 0xF0) == 0xE0) {
            // Need 2 more bytes (i+1 and i+2 must be valid indices)
            if (i + 2 >= len) {
                return false;  // Not enough bytes
            }
            // Second and third bytes must be continuation bytes
            if ((d[i + 1] & 0xC0) != 0x80 || (d[i + 2] & 0xC0) != 0x80) {
                return false;
            }
            // Check for overlong encoding
            // 0xE0 0x80-0x9F would encode values < 0x800 (should be 2-byte)
            if (byte == 0xE0 && d[i + 1] < 0xA0) {
                return false;
            }
            // Check for UTF-16 surrogates (U+D800 to U+DFFF are invalid in UTF-8)
            // 0xED 0xA0-0xBF would encode surrogates
            if (byte == 0xED && d[i + 1] >= 0xA0) {
                return false;
            }
            i += 3;
            continue;
        }

        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        // Valid range: U+10000 to U+10FFFF
        if ((byte & 0xF8) == 0xF0) {
            // Need 3 more bytes (i+1, i+2, i+3 must be valid indices)
            if (i + 3 >= len) {
                return false;  // Not enough bytes
            }
            // All continuation bytes must be valid
            if ((d[i + 1] & 0xC0) != 0x80 || (d[i + 2] & 0xC0) != 0x80 || (d[i + 3] & 0xC0) != 0x80) {
                return false;
            }
            // Check for overlong encoding
            // 0xF0 0x80-0x8F would encode values < 0x10000 (should be 3-byte)
            if (byte == 0xF0 && d[i + 1] < 0x90) {
                return false;
            }
            // Check for values > U+10FFFF (maximum valid Unicode)
            // 0xF4 0x90+ or 0xF5+ would exceed U+10FFFF
            if (byte >= 0xF4 && (byte > 0xF4 || d[i + 1] >= 0x90)) {
                return false;
            }
            i += 4;
            continue;
        }

        // Invalid UTF-8 start byte
        return false;
    }

    return true;
}

bool EncodingDetector::IsLikelyASCII(const ByteSlice& data) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    // Sample first 4KB
    if (len > 4096) {
        len = 4096;
    }

    size_t nonAsciiCount = 0;
    for (size_t i = 0; i < len; i++) {
        if (d[i] > 0x7F) {
            nonAsciiCount++;
        }
    }

    // If less than 1% non-ASCII, consider it ASCII
    return (nonAsciiCount * 100 / len) < 1;
}

// Helper: Check if byte is valid EUC-KR lead byte
static bool IsEUCKRLead(u8 byte) {
    return (byte >= 0xA1 && byte <= 0xFE);
}

// Helper: Check if byte is valid EUC-KR trail byte
static bool IsEUCKRTrail(u8 byte) {
    return (byte >= 0xA1 && byte <= 0xFE);
}

// Detect Korean EUC-KR encoding
bool EncodingDetector::IsLikelyEUCKR(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t validPairs = 0;
    size_t totalPairs = 0;
    size_t koreanSyllables = 0;  // Hangul syllables (U+AC00 to U+D7A3)

    for (size_t i = 0; i < len - 1; i++) {
        if (IsEUCKRLead(d[i])) {
            totalPairs++;
            if (IsEUCKRTrail(d[i + 1])) {
                validPairs++;

                // Check for Hangul syllable range (0xB0A1-0xC8FE in EUC-KR)
                if ((d[i] >= 0xB0 && d[i] <= 0xC8) &&
                    (d[i + 1] >= 0xA1 && d[i + 1] <= 0xFE)) {
                    koreanSyllables++;
                }
                i++;  // Skip the trail byte
            }
        }
    }

    if (totalPairs == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float validRatio = (float)validPairs / totalPairs;
    float koreanRatio = (float)koreanSyllables / totalPairs;
    float score = validRatio * 0.7f + koreanRatio * 0.3f;

    if (scoreOut) *scoreOut = score;

    // High confidence if >80% valid pairs and >30% Korean syllables
    return (validRatio > 0.8f && koreanRatio > 0.3f);
}

// Detect Japanese Shift-JIS encoding
bool EncodingDetector::IsLikelyShiftJIS(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t validPairs = 0;
    size_t totalPairs = 0;
    size_t hiraganaKatakana = 0;

    for (size_t i = 0; i < len - 1; i++) {
        u8 lead = d[i];

        // Shift-JIS lead byte ranges: 0x81-0x9F, 0xE0-0xFC
        if ((lead >= 0x81 && lead <= 0x9F) || (lead >= 0xE0 && lead <= 0xFC)) {
            totalPairs++;
            u8 trail = d[i + 1];

            // Shift-JIS trail byte ranges: 0x40-0x7E, 0x80-0xFC
            if ((trail >= 0x40 && trail <= 0x7E) || (trail >= 0x80 && trail <= 0xFC)) {
                validPairs++;

                // Hiragana: 0x829F-0x82F1, Katakana: 0x8340-0x8396
                if ((lead == 0x82 && trail >= 0x9F) ||
                    (lead == 0x83 && trail >= 0x40 && trail <= 0x96)) {
                    hiraganaKatakana++;
                }
                i++;  // Skip the trail byte
            }
        }
    }

    if (totalPairs == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float validRatio = (float)validPairs / totalPairs;
    float japaneseRatio = (float)hiraganaKatakana / totalPairs;
    float score = validRatio * 0.7f + japaneseRatio * 0.3f;

    if (scoreOut) *scoreOut = score;

    // High confidence if >80% valid pairs and >20% Hiragana/Katakana
    return (validRatio > 0.8f && japaneseRatio > 0.2f);
}

// Detect Chinese GB2312/GBK encoding
bool EncodingDetector::IsLikelyGB2312(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t validPairs = 0;
    size_t totalPairs = 0;
    size_t chineseChars = 0;

    for (size_t i = 0; i < len - 1; i++) {
        u8 lead = d[i];

        // GB2312 lead byte: 0xA1-0xFE
        if (lead >= 0xA1 && lead <= 0xFE) {
            totalPairs++;
            u8 trail = d[i + 1];

            // GB2312 trail byte: 0xA1-0xFE
            if (trail >= 0xA1 && trail <= 0xFE) {
                validPairs++;

                // Common Chinese character range (U+4E00-U+9FA5: 0xB0A1-0xF7FE in GB2312)
                if (lead >= 0xB0 && lead <= 0xF7) {
                    chineseChars++;
                }
                i++;  // Skip the trail byte
            }
        }
    }

    if (totalPairs == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float validRatio = (float)validPairs / totalPairs;
    float chineseRatio = (float)chineseChars / totalPairs;
    float score = validRatio * 0.7f + chineseRatio * 0.3f;

    if (scoreOut) *scoreOut = score;

    // High confidence if >80% valid pairs and >40% Chinese characters
    return (validRatio > 0.8f && chineseRatio > 0.4f);
}

// Detect Traditional Chinese Big5 encoding
bool EncodingDetector::IsLikelyBig5(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t validPairs = 0;
    size_t totalPairs = 0;

    for (size_t i = 0; i < len - 1; i++) {
        u8 lead = d[i];

        // Big5 lead byte: 0x81-0xFE
        if (lead >= 0x81 && lead <= 0xFE) {
            totalPairs++;
            u8 trail = d[i + 1];

            // Big5 trail byte: 0x40-0x7E, 0x80-0xFE
            if ((trail >= 0x40 && trail <= 0x7E) || (trail >= 0x80 && trail <= 0xFE)) {
                validPairs++;
                i++;  // Skip the trail byte
            }
        }
    }

    if (totalPairs == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float validRatio = (float)validPairs / totalPairs;

    if (scoreOut) *scoreOut = validRatio;

    // High confidence if >85% valid pairs
    return (validRatio > 0.85f);
}

// Detect Central European Windows-1250 encoding
bool EncodingDetector::IsLikelyCentralEuropean(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t highBytes = 0;
    size_t centralEuropeanChars = 0;

    for (size_t i = 0; i < len; i++) {
        u8 byte = d[i];
        if (byte >= 0x80) {
            highBytes++;
            // Common Central European characters in Windows-1250
            // Polish, Czech, Hungarian specific bytes
            if (byte == 0xB9 || byte == 0xE6 || byte == 0xEA || byte == 0xB3 ||
                byte == 0xF1 || byte == 0xF3 || byte == 0x9C || byte == 0x9F ||
                byte == 0xBF || byte == 0x8C || byte == 0x8F || byte == 0x9A ||
                byte == 0x9D || byte == 0x9E || byte == 0xD5 || byte == 0xDB) {
                centralEuropeanChars++;
            }
        }
    }

    if (highBytes == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float score = (float)centralEuropeanChars / highBytes;
    if (scoreOut) *scoreOut = score;

    return (score > 0.2f && highBytes * 100 / len > 5);
}

// Detect Cyrillic Windows-1251 encoding
bool EncodingDetector::IsLikelyCyrillic(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t highBytes = 0;
    size_t cyrillicChars = 0;

    for (size_t i = 0; i < len; i++) {
        u8 byte = d[i];
        if (byte >= 0x80) {
            highBytes++;
            // Cyrillic range in Windows-1251: 0xC0-0xFF
            if ((byte >= 0xC0 && byte <= 0xFF) || byte == 0xA8 || byte == 0xB8 ||
                byte == 0xAA || byte == 0xBA || byte == 0xAF || byte == 0xBF) {
                cyrillicChars++;
            }
        }
    }

    if (highBytes == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float score = (float)cyrillicChars / highBytes;
    if (scoreOut) *scoreOut = score;

    return (score > 0.6f && highBytes * 100 / len > 10);
}

// Detect Greek Windows-1253 encoding
bool EncodingDetector::IsLikelyGreek(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t highBytes = 0;
    size_t greekChars = 0;

    for (size_t i = 0; i < len; i++) {
        u8 byte = d[i];
        if (byte >= 0x80) {
            highBytes++;
            // Greek range in Windows-1253: 0xC0-0xFE
            if ((byte >= 0xC0 && byte <= 0xFE) ||
                (byte >= 0xA1 && byte <= 0xA3) ||
                (byte >= 0xB4 && byte <= 0xBE)) {
                greekChars++;
            }
        }
    }

    if (highBytes == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float score = (float)greekChars / highBytes;
    if (scoreOut) *scoreOut = score;

    return (score > 0.6f && highBytes * 100 / len > 10);
}

// Detect Turkish Windows-1254 encoding
bool EncodingDetector::IsLikelyTurkish(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t highBytes = 0;
    size_t turkishChars = 0;

    for (size_t i = 0; i < len; i++) {
        u8 byte = d[i];
        if (byte >= 0x80) {
            highBytes++;
            // Turkish-specific characters
            if (byte == 0xF0 || byte == 0xD0 || byte == 0xFD || byte == 0xDD ||
                byte == 0xF6 || byte == 0xD6 || byte == 0xFE || byte == 0xDE ||
                byte == 0xFC || byte == 0xDC || byte == 0xE7 || byte == 0xC7) {
                turkishChars++;
            }
        }
    }

    if (highBytes == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float score = (float)turkishChars / highBytes;
    if (scoreOut) *scoreOut = score;

    return (score > 0.15f && highBytes * 100 / len > 5);
}

// Detect Hebrew Windows-1255 encoding
bool EncodingDetector::IsLikelyHebrew(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t highBytes = 0;
    size_t hebrewChars = 0;

    for (size_t i = 0; i < len; i++) {
        u8 byte = d[i];
        if (byte >= 0x80) {
            highBytes++;
            // Hebrew range in Windows-1255: 0xE0-0xFA
            if (byte >= 0xE0 && byte <= 0xFA) {
                hebrewChars++;
            }
        }
    }

    if (highBytes == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float score = (float)hebrewChars / highBytes;
    if (scoreOut) *scoreOut = score;

    return (score > 0.6f && highBytes * 100 / len > 10);
}

// Detect Arabic Windows-1256 encoding
bool EncodingDetector::IsLikelyArabic(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t highBytes = 0;
    size_t arabicChars = 0;

    for (size_t i = 0; i < len; i++) {
        u8 byte = d[i];
        if (byte >= 0x80) {
            highBytes++;
            // Arabic range in Windows-1256
            if ((byte >= 0xC1 && byte <= 0xFE) ||
                (byte >= 0x8C && byte <= 0x9F)) {
                arabicChars++;
            }
        }
    }

    if (highBytes == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float score = (float)arabicChars / highBytes;
    if (scoreOut) *scoreOut = score;

    return (score > 0.6f && highBytes * 100 / len > 10);
}

// Detect Baltic Windows-1257 encoding
bool EncodingDetector::IsLikelyBaltic(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t highBytes = 0;
    size_t balticChars = 0;

    for (size_t i = 0; i < len; i++) {
        u8 byte = d[i];
        if (byte >= 0x80) {
            highBytes++;
            // Baltic-specific characters
            if (byte == 0xE0 || byte == 0xE8 || byte == 0xEA || byte == 0xEB ||
                byte == 0xEC || byte == 0xF0 || byte == 0xF8 || byte == 0xFE ||
                byte == 0xC0 || byte == 0xC8 || byte == 0xCA || byte == 0xCB ||
                byte == 0xCC || byte == 0xD0 || byte == 0xD8 || byte == 0xDE) {
                balticChars++;
            }
        }
    }

    if (highBytes == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float score = (float)balticChars / highBytes;
    if (scoreOut) *scoreOut = score;

    return (score > 0.2f && highBytes * 100 / len > 5);
}

// Detect Vietnamese Windows-1258 encoding
bool EncodingDetector::IsLikelyVietnamese(const ByteSlice& data, float* scoreOut) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    if (len < 10) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    size_t highBytes = 0;
    size_t vietnameseChars = 0;

    for (size_t i = 0; i < len; i++) {
        u8 byte = d[i];
        if (byte >= 0x80) {
            highBytes++;
            // Vietnamese tone marks
            if ((byte >= 0xC0 && byte <= 0xC3) ||  // A with tones
                (byte >= 0xC8 && byte <= 0xCA) ||  // E with tones
                (byte >= 0xCC && byte <= 0xCD) ||  // I with tones
                (byte >= 0xD2 && byte <= 0xD5) ||  // O with tones
                (byte >= 0xD9 && byte <= 0xDA) ||  // U with tones
                (byte >= 0xE0 && byte <= 0xE3) ||  // a with tones
                (byte >= 0xE8 && byte <= 0xEA) ||  // e with tones
                (byte >= 0xEC && byte <= 0xED) ||  // i with tones
                (byte >= 0xF2 && byte <= 0xF5) ||  // o with tones
                (byte >= 0xF9 && byte <= 0xFA) ||  // u with tones
                byte == 0xD0 || byte == 0xF0) {    // D with stroke
                vietnameseChars++;
            }
        }
    }

    if (highBytes == 0) {
        if (scoreOut) *scoreOut = 0.0f;
        return false;
    }

    float score = (float)vietnameseChars / highBytes;
    if (scoreOut) *scoreOut = score;

    return (score > 0.3f && highBytes * 100 / len > 10);
}

EncodingResult EncodingDetector::DetectFromContent(const ByteSlice& data) {
    // First check if it's valid UTF-8
    if (IsValidUTF8(data)) {
        // Check if it's actually ASCII
        if (IsLikelyASCII(data)) {
            return EncodingResult(20127, EncodingConfidence::High, "ASCII");
        }
        return EncodingResult(CP_UTF8, EncodingConfidence::High, "UTF-8");
    }

    // Sample the data (first 8KB)
    size_t sampleSize = data.size();
    if (sampleSize > 8192) {
        sampleSize = 8192;
    }

    const u8* d = (const u8*)data.data();

    // Count high bytes (0x80-0xFF)
    size_t highByteCount = 0;
    for (size_t i = 0; i < sampleSize; i++) {
        if (d[i] >= 0x80) {
            highByteCount++;
        }
    }

    // If very few high bytes, might be ASCII or Latin-1
    if (highByteCount * 100 / sampleSize < 5) {
        return EncodingResult(1252, EncodingConfidence::Low, "Windows-1252");
    }

    // Try East Asian encodings with scoring
    float eucKrScore = 0.0f;
    float shiftJisScore = 0.0f;
    float gb2312Score = 0.0f;
    float big5Score = 0.0f;

    bool isEucKr = IsLikelyEUCKR(data, &eucKrScore);
    bool isShiftJis = IsLikelyShiftJIS(data, &shiftJisScore);
    bool isGb2312 = IsLikelyGB2312(data, &gb2312Score);
    bool isBig5 = IsLikelyBig5(data, &big5Score);

    // Try European/Middle Eastern encodings
    float centralEuropeanScore = 0.0f;
    float cyrillicScore = 0.0f;
    float greekScore = 0.0f;
    float turkishScore = 0.0f;
    float hebrewScore = 0.0f;
    float arabicScore = 0.0f;
    float balticScore = 0.0f;
    float vietnameseScore = 0.0f;

    bool isCentralEuropean = IsLikelyCentralEuropean(data, &centralEuropeanScore);
    bool isCyrillic = IsLikelyCyrillic(data, &cyrillicScore);
    bool isGreek = IsLikelyGreek(data, &greekScore);
    bool isTurkish = IsLikelyTurkish(data, &turkishScore);
    bool isHebrew = IsLikelyHebrew(data, &hebrewScore);
    bool isArabic = IsLikelyArabic(data, &arabicScore);
    bool isBaltic = IsLikelyBaltic(data, &balticScore);
    bool isVietnamese = IsLikelyVietnamese(data, &vietnameseScore);

    // Find the best match
    float maxScore = 0.0f;
    uint bestCodepage = 0;
    const char* bestName = nullptr;

    uint systemCP = GetACP();
    float weight = 0.15f; // Boost for system default encoding

    // East Asian encodings
    if (isEucKr) {
        float score = eucKrScore + (systemCP == 949 || systemCP == 51949 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = (systemCP == 949) ? 949 : 51949; // Prefer more accurate CP if possible
            if (bestCodepage != 949 && bestCodepage != 51949) bestCodepage = 949;
            bestName = "EUC-KR";
        }
    }

    if (isShiftJis) {
        float score = shiftJisScore + (systemCP == 932 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 932;
            bestName = "Shift-JIS";
        }
    }

    if (isGb2312) {
        float score = gb2312Score + (systemCP == 936 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 936;
            bestName = "GB2312";
        }
    }

    if (isBig5) {
        float score = big5Score + (systemCP == 950 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 950;
            bestName = "Big5";
        }
    }

    // European/Middle Eastern encodings
    if (isCentralEuropean) {
        float score = centralEuropeanScore + (systemCP == 1250 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 1250;
            bestName = "Windows-1250";
        }
    }

    if (isCyrillic) {
        float score = cyrillicScore + (systemCP == 1251 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 1251;
            bestName = "Windows-1251";
        }
    }

    if (isGreek) {
        float score = greekScore + (systemCP == 1253 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 1253;
            bestName = "Windows-1253";
        }
    }

    if (isTurkish) {
        float score = turkishScore + (systemCP == 1254 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 1254;
            bestName = "Windows-1254";
        }
    }

    if (isHebrew) {
        float score = hebrewScore + (systemCP == 1255 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 1255;
            bestName = "Windows-1255";
        }
    }

    if (isArabic) {
        float score = arabicScore + (systemCP == 1256 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 1256;
            bestName = "Windows-1256";
        }
    }

    if (isBaltic) {
        float score = balticScore + (systemCP == 1257 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 1257;
            bestName = "Windows-1257";
        }
    }

    if (isVietnamese) {
        float score = vietnameseScore + (systemCP == 1258 ? weight : 0.0f);
        if (score > maxScore) {
            maxScore = score;
            bestCodepage = 1258;
            bestName = "Windows-1258";
        }
    }

    // Return best match if found
    if (bestCodepage != 0) {
        EncodingConfidence conf = EncodingConfidence::Medium;
        if (maxScore > 0.9f) {
            conf = EncodingConfidence::High;
        } else if (maxScore < 0.6f) {
            conf = EncodingConfidence::Low;
        }
        return EncodingResult(bestCodepage, conf, bestName);
    }

    // Fallback to system default with low confidence
    uint defaultCP = GetACP();
    return EncodingResult(defaultCP, EncodingConfidence::Low, "System Default");
}

EncodingResult EncodingDetector::DetectEncoding(const ByteSlice& data) {
    if (data.size() == 0) {
        return EncodingResult();
    }

    // 1. Check for BOM (highest confidence)
    EncodingResult result = DetectFromBOM(data);
    if (result.confidence == EncodingConfidence::Certain) {
        return result;
    }

    // 2. Check for encoding declaration (high confidence)
    result = DetectFromDeclaration(data);
    if (result.confidence >= EncodingConfidence::High) {
        return result;
    }

    // 3. Analyze content (medium/low confidence)
    result = DetectFromContent(data);
    return result;
}

Vec<EncodingResult> EncodingDetector::DetectMultiple(const ByteSlice& data) {
    Vec<EncodingResult> results;

    // Add BOM result if present
    EncodingResult bomResult = DetectFromBOM(data);
    if (bomResult.confidence != EncodingConfidence::None) {
        results.Append(bomResult);
        return results; // BOM is certain, no need for other candidates
    }

    // Add declaration result if present
    EncodingResult declResult = DetectFromDeclaration(data);
    if (declResult.confidence != EncodingConfidence::None) {
        results.Append(declResult);
    }

    // Add content analysis result
    EncodingResult contentResult = DetectFromContent(data);
    if (contentResult.confidence != EncodingConfidence::None) {
        // Don't add duplicate
        bool isDuplicate = false;
        for (size_t i = 0; i < results.size(); i++) {
            if (results[i].codepage == contentResult.codepage) {
                isDuplicate = true;
                break;
            }
        }
        if (!isDuplicate) {
            results.Append(contentResult);
        }
    }

    // Add common fallbacks if nothing found
    if (results.size() == 0) {
        results.Append(EncodingResult(CP_UTF8, EncodingConfidence::Low, "UTF-8"));
        results.Append(EncodingResult(GetACP(), EncodingConfidence::Low, "System Default"));
    }

    return results;
}

float EncodingDetector::ScoreCodepage(const ByteSlice& data, uint codepage) {
    // TODO: Implement scoring based on character frequency analysis
    // For now, return a simple score
    return 0.5f;
}
