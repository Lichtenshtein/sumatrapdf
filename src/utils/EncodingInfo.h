/* Copyright 2022 the GurupiaReader project authors (see AUTHORS file).
   License: GPLv3 */

// EncodingInfo provides information about character encodings/codepages
// and a registry to look up encoding information

struct EncodingInfo {
    uint codepage;
    const char* name;           // "UTF-8"
    const char* displayName;    // "Unicode (UTF-8)"
    const char* localizedName;  // Translated name (for future use)
    bool isCommon;              // Frequently used encoding
};

class EncodingRegistry {
  public:
    // Get all supported encodings
    static Vec<EncodingInfo> GetAllEncodings();

    // Get only commonly used encodings
    static Vec<EncodingInfo> GetCommonEncodings();

    // Look up encoding info by codepage
    static EncodingInfo* GetEncodingInfo(uint codepage);

    // Look up codepage by name (case-insensitive)
    static uint GetCodepageByName(const char* name);

    // Get default codepage based on system locale
    static uint GetDefaultCodepage();

    // Get display name for a codepage
    static const char* GetDisplayName(uint codepage);
};
