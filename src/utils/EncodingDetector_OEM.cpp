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
