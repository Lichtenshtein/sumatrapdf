/* Copyright 2022 the GurupiaReader project authors (see AUTHORS file).
   License: GPLv3 */

#include "BaseUtil.h"
#include "EncodingDetector.h"
#include "UtAssert.h"

void EncodingDetector_UnitTests() {
    // Test UTF-8 detection
    {
        const char* utf8Data = "\xED\x95\x9C\xEA\xB8\x80"; // "한글" in UTF-8
        ByteSlice data((u8*)utf8Data, 6);
        EncodingResult res = EncodingDetector::DetectEncoding(data);
        uint codepage = res.codepage;
        utassert(codepage == CP_UTF8);
    }

    // Test EUC-KR (CP949) detection with longer text
    {
        // "안녕하세요 한글 테스트" in EUC-KR
        const char* eucKrData = "\xBE\xC8\xB3\xE7\xC7\xCF\xBC\xBC\xBF\xE4\x20"
                                "\xC7\xD1\xB1\xDB\x20\xC5\xD7\xBD\xBA\xC6\xAE";
        ByteSlice data((u8*)eucKrData, strlen(eucKrData));
        EncodingResult res = EncodingDetector::DetectEncoding(data);
        uint codepage = res.codepage;

        // Should detect as EUC-KR (949)
        printf("EUC-KR test: Detected CP%u (%s) with confidence %d\n",
               codepage, res.encodingName ? res.encodingName : "Unknown", (int)res.confidence);
        utassert(codepage == 949);
    }

    // Test Shift-JIS detection
    {
        // "こんにちは" (Konnichiwa) in Shift-JIS
        const char* shiftJisData = "\x82\xB1\x82\xF1\x82\xC9\x82\xBF\x82\xCD";
        ByteSlice data((u8*)shiftJisData, strlen(shiftJisData));
        EncodingResult res = EncodingDetector::DetectEncoding(data);
        uint codepage = res.codepage;

        printf("Shift-JIS test: Detected CP%u (%s) with confidence %d\n",
               codepage, res.encodingName ? res.encodingName : "Unknown", (int)res.confidence);
        utassert(codepage == 932);
    }

    // Test GB2312 detection
    {
        // "你好世界" (Hello World) in GB2312
        const char* gb2312Data = "\xC4\xE3\xBA\xC3\xCA\xC0\xBD\xE7";
        ByteSlice data((u8*)gb2312Data, strlen(gb2312Data));
        EncodingResult res = EncodingDetector::DetectEncoding(data);
        uint codepage = res.codepage;

        printf("GB2312 test: Detected CP%u (%s) with confidence %d\n",
               codepage, res.encodingName ? res.encodingName : "Unknown", (int)res.confidence);
        utassert(codepage == 936);
    }

    // Test Big5 detection
    {
        // "你好世界" (Hello World) in Big5
        const char* big5Data = "\xA7\x41\xA6\x6E\xA5\x40\xAC\xC9";
        ByteSlice data((u8*)big5Data, strlen(big5Data));
        EncodingResult res = EncodingDetector::DetectEncoding(data);
        uint codepage = res.codepage;

        printf("Big5 test: Detected CP%u (%s) with confidence %d\n",
               codepage, res.encodingName ? res.encodingName : "Unknown", (int)res.confidence);
        utassert(codepage == 950);
    }

    // Test ASCII detection
    {
        const char* asciiData = "Hello World";
        ByteSlice data((u8*)asciiData, 11);
        EncodingResult res = EncodingDetector::DetectEncoding(data);
        uint codepage = res.codepage;
        if (codepage != CP_UTF8 && codepage != 1252 && codepage != 437 && codepage != 20127) {
            printf("Detected codepage for ASCII: %u\n", codepage);
        }
        utassert(codepage == CP_UTF8 || codepage == 1252 || codepage == 437 || codepage == 20127);
    }

    printf("\n=== All EncodingDetector tests passed! ===\n");
}
