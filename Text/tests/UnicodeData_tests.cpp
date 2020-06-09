// Copyright 2019 Pokitec
// All rights reserved.

#include "TTauri/Text/UnicodeData.hpp"
#include "TTauri/Foundation/FileView.hpp"
#include "TTauri/Foundation/strings.hpp"
#include "data/UnicodeData.bin.inl"
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <nonstd/span>
#include <fmt/format.h>

using namespace std;
using namespace TTauri;
using namespace TTauri;

/*! A test defined in NormalizationTests.txt.
 *
 * CONFORMANCE:
 * 1. The following invariants must be true for all conformant implementations
 *
 *    NFC
 *      c2 ==  toNFC(c1) ==  toNFC(c2) ==  toNFC(c3)
 *      c4 ==  toNFC(c4) ==  toNFC(c5)
 *
 *    NFD
 *      c3 ==  toNFD(c1) ==  toNFD(c2) ==  toNFD(c3)
 *      c5 ==  toNFD(c4) ==  toNFD(c5)
 *
 *    NFKC
 *      c4 == toNFKC(c1) == toNFKC(c2) == toNFKC(c3) == toNFKC(c4) == toNFKC(c5)
 *
 *    NFKD
 *      c5 == toNFKD(c1) == toNFKD(c2) == toNFKD(c3) == toNFKD(c4) == toNFKD(c5)
 *
 * 2. For every code point X assigned in this version of Unicode that is not specifically
 *    listed in Part 1, the following invariants must be true for all conformant
 *    implementations:
 *
 *      X == toNFC(X) == toNFD(X) == toNFKC(X) == toNFKD(X)
 */
struct NormalizationTest {
    std::u32string c1;
    std::u32string c2;
    std::u32string c3;
    std::u32string c4;
    std::u32string c5;
    std::string comment;
};

std::u32string parseNormalizationTest_column(std::string_view column) {
    std::u32string r;

    auto codePointStrings = split(column, " ");
    for (let codePointString: codePointStrings) {
        auto codePoint = static_cast<char32_t>(std::stoi(std::string(codePointString), nullptr, 16));
        r += codePoint;
    }
    return r;
}

std::optional<NormalizationTest> parseNormalizationTest_line(std::string_view line)
{
    let split_line = split(line, "#");
    if (split_line.size() < 2) {
        return {};
    }
    let columns = split(split_line[0], ";");
    if (columns.size() < 6) {
        return {};
    }

    NormalizationTest r;
    r.c1 = parseNormalizationTest_column(columns[0]);
    r.c2 = parseNormalizationTest_column(columns[1]);
    r.c3 = parseNormalizationTest_column(columns[2]);
    r.c4 = parseNormalizationTest_column(columns[3]);
    r.c5 = parseNormalizationTest_column(columns[4]);
    r.comment = split_line[1];

    return r;
}

std::vector<NormalizationTest> parseNormalizationTests()
{
    let view = FileView(URL("file:NormalizationTest.txt"));
    let test_data = view.string_view();

    std::vector<NormalizationTest> r;
    for (let line: split(test_data, "\n")) {
        if (let optionalTest = parseNormalizationTest_line(line)) {   
            r.push_back(*optionalTest);
        }
    }
    return r;
}

struct GraphemeBreakTest {
    std::u32string codePoints;
    std::vector<bool> breakOpertunities;
    std::string comment;
    int lineNr;
};

std::optional<GraphemeBreakTest> parseGraphemeBreakTests_line(std::string_view line, int lineNr)
{
    GraphemeBreakTest r;

    let split_line = split(line, "\t#");
    if (split_line.size() < 2) {
        return {};
    }
    r.comment = fmt::format("{}: {}", lineNr, split_line[1]);
    r.lineNr = lineNr;

    let columns = split(split_line[0], " ");
    if (columns.size() < 2) {
        return {};
    }

    for (let column: columns) {
        if (column == "") {
            // Empty.
        } else if (column == "\xc3\xb7") {
            r.breakOpertunities.push_back(true);
        } else if (column == "\xc3\x97") {
            r.breakOpertunities.push_back(false);
        } else {
            auto codePoint = static_cast<char32_t>(std::stoi(std::string(column), nullptr, 16));
            r.codePoints += codePoint;
        }
    }

    return r;
}

std::vector<GraphemeBreakTest> parseGraphemeBreakTests()
{
    let view = FileView(URL("file:GraphemeBreakTest.txt"));
    let test_data = view.string_view();

    std::vector<GraphemeBreakTest> r;
    int lineNr = 1;
    for (let line: split(test_data, "\n")) {
        if (let optionalTest = parseGraphemeBreakTests_line(line, lineNr)) {   
            r.push_back(*optionalTest);
        }
        lineNr++;
    }
    return r;
}

class UnicodeDataTests : public ::testing::Test {
protected:
    void SetUp() override {
        normalizationTests = parseNormalizationTests();
    }

    std::vector<NormalizationTest> normalizationTests;
    UnicodeData unicodeData = UnicodeData(UnicodeData_bin_bytes);
};

// CONFORMANCE:
// 1. The following invariants must be true for all conformant implementations
//
//    NFC
//      c2 ==  toNFC(c1) ==  toNFC(c2) ==  toNFC(c3)
//      c4 ==  toNFC(c4) ==  toNFC(c5)
//
//    NFD
//      c3 ==  toNFD(c1) ==  toNFD(c2) ==  toNFD(c3)
//      c5 ==  toNFD(c4) ==  toNFD(c5)
//
//    NFKC
//      c4 == toNFKC(c1) == toNFKC(c2) == toNFKC(c3) == toNFKC(c4) == toNFKC(c5)
//
//    NFKD
//      c5 == toNFKD(c1) == toNFKD(c2) == toNFKD(c3) == toNFKD(c4) == toNFKD(c5)
//
// 2. For every code point X assigned in this version of Unicode that is not specifically
//    listed in Part 1, the following invariants must be true for all conformant
//    implementations:
//
//      X == toNFC(X) == toNFD(X) == toNFKC(X) == toNFKD(X)

TEST_F(UnicodeDataTests, toNFC_c1) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFC(test.c1), test.c2) << test.comment;
    }
}

#if defined(NDEBUG)
TEST_F(UnicodeDataTests, toNFC_c2) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFC(test.c2), test.c2) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFC_c3) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFC(test.c3), test.c2) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFC_c4) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFC(test.c4), test.c4) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFC_c5) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFC(test.c5), test.c4) << test.comment;
    }
}
#endif

TEST_F(UnicodeDataTests, toNFKC_c1) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFKC(test.c1), test.c4) << test.comment;
    }
}

#if defined(NDEBUG)
TEST_F(UnicodeDataTests, toNFKC_c2) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFKC(test.c2), test.c4) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFKC_c3) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFKC(test.c3), test.c4) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFKC_c4) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFKC(test.c4), test.c4) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFKC_c5) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFKC(test.c5), test.c4) << test.comment;
    }
}
#endif

TEST_F(UnicodeDataTests, toNFD_c1) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFD(test.c1), test.c3) << test.comment;
    }
}

#if defined(NDEBUG)
TEST_F(UnicodeDataTests, toNFD_c2) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFD(test.c2), test.c3) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFD_c3) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFD(test.c3), test.c3) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFD_c4) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFD(test.c4), test.c5) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFD_c5) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFD(test.c5), test.c5) << test.comment;
    }
}
#endif

TEST_F(UnicodeDataTests, toNFKD_c1) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFKD(test.c1), test.c5) << test.comment;
    }
}

#if defined(NDEBUG)
TEST_F(UnicodeDataTests, toNFKD_c2) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFKD(test.c2), test.c5) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFKD_c3) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFKD(test.c3), test.c5) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFKD_c4) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFKD(test.c4), test.c5) << test.comment;
    }
}

TEST_F(UnicodeDataTests, toNFKD_c5) {
    for (let &test: normalizationTests) {
        ASSERT_EQ(unicodeData.toNFKD(test.c5), test.c5) << test.comment;
    }
}
#endif

#if defined(NDEBUG)
TEST_F(UnicodeDataTests, Invariant) {
    auto previouslyTestedCodePoints = std::vector<bool>(0x11'0000, false);
    for (let &test: normalizationTests) {
        for (let &c: test.c1) {
            previouslyTestedCodePoints[c] = true;
        }
        for (let &c: test.c2) {
            previouslyTestedCodePoints[c] = true;
        }
        for (let &c: test.c3) {
            previouslyTestedCodePoints[c] = true;
        }
        for (let &c: test.c4) {
            previouslyTestedCodePoints[c] = true;
        }
        for (let &c: test.c5) {
            previouslyTestedCodePoints[c] = true;
        }
    }

    for (char32_t i = 0; i < previouslyTestedCodePoints.size(); i++) {
        if (!previouslyTestedCodePoints[i]) {
            let str = std::u32string(1, i);

            ASSERT_EQ(unicodeData.toNFD(str), str);
            ASSERT_EQ(unicodeData.toNFC(str), str);
            ASSERT_EQ(unicodeData.toNFKD(str), str);
            ASSERT_EQ(unicodeData.toNFKC(str), str);
        }
    }
}
#endif

TEST_F(UnicodeDataTests, GraphemeBreak) {
    auto tests = parseGraphemeBreakTests();

    for (let &test: tests) {
        ASSERT_EQ(test.codePoints.size() + 1, test.breakOpertunities.size());

        auto state = GraphemeBreakState{};
        
        for (size_t i = 0; i < test.codePoints.size(); i++) {
            let codePoint = test.codePoints[i];
            let breakOpertunity = test.breakOpertunities[i];

            ASSERT_EQ(unicodeData.checkGraphemeBreak(codePoint, state), breakOpertunity) << test.comment;
        }

    }
}
