/* Test program for EncodingDetector */

#include "utils/BaseUtil.h"
#include "utils/FileUtil.h"
#include "utils/EncodingDetector.h"
#include <stdio.h>

static void TestFile(const char* filePath, const char* expectedEncoding) {
    printf("\n========================================\n");
    printf("Testing: %s\n", filePath);
    printf("Expected: %s\n", expectedEncoding);
    printf("========================================\n");

    ByteSlice data = file::ReadFile(filePath);
    if (data.empty()) {
        printf("ERROR: Failed to read file!\n");
        return;
    }

    printf("File size: %zu bytes\n", data.size());

    // Test single detection
    EncodingResult result = EncodingDetector::DetectEncoding(data);

    const char* confidenceName = "Unknown";
    switch (result.confidence) {
        case EncodingConfidence::None: confidenceName = "None"; break;
        case EncodingConfidence::Low: confidenceName = "Low"; break;
        case EncodingConfidence::Medium: confidenceName = "Medium"; break;
        case EncodingConfidence::High: confidenceName = "High"; break;
        case EncodingConfidence::Certain: confidenceName = "Certain"; break;
    }

    printf("\nDetected Encoding:\n");
    printf("  Codepage: %u\n", result.codepage);
    printf("  Name: %s\n", result.encodingName ? result.encodingName : "Unknown");
    printf("  Confidence: %s\n", confidenceName);

    // Test multiple candidates
    Vec<EncodingResult> candidates = EncodingDetector::DetectMultiple(data);
    printf("\nAll Candidates (%d):\n", candidates.Size());
    for (int i = 0; i < candidates.Size(); i++) {
        EncodingResult& r = candidates.At(i);
        const char* conf = "Unknown";
        switch (r.confidence) {
            case EncodingConfidence::None: conf = "None"; break;
            case EncodingConfidence::Low: conf = "Low"; break;
            case EncodingConfidence::Medium: conf = "Medium"; break;
            case EncodingConfidence::High: conf = "High"; break;
            case EncodingConfidence::Certain: conf = "Certain"; break;
        }
        printf("  %d. CP%u (%s) - %s\n", i+1, r.codepage,
               r.encodingName ? r.encodingName : "Unknown", conf);
    }

    data.Free();
}

int main(int argc, char* argv[]) {
    printf("===========================================\n");
    printf("  EncodingDetector Test Suite\n");
    printf("===========================================\n");

    // Test UTF-8
    TestFile("test_files\\utf8.txt", "UTF-8");

    // Test Korean EUC-KR
    TestFile("test_files\\korean_euckr.txt", "EUC-KR");

    // Test Japanese Shift-JIS
    TestFile("test_files\\japanese_shiftjis.txt", "Shift-JIS");

    // Test Chinese GB2312
    TestFile("test_files\\chinese_gb2312.txt", "GB2312");

    // Test Chinese Big5
    TestFile("test_files\\chinese_big5.txt", "Big5");

    printf("\n===========================================\n");
    printf("  Test Complete!\n");
    printf("===========================================\n");

    return 0;
}
