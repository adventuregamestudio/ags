//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <string.h>
#include "gtest/gtest.h"
#include "util/utf8.h"

void TestGetCharBack(const char *utf8_str, const char *first_char_bytes)
{
    const char *read_ptr = utf8_str + strlen(utf8_str);
    for (size_t c = strlen(first_char_bytes) - 1;
         read_ptr > utf8_str; --c)
    {
        read_ptr = Utf8::BackOneChar(read_ptr, utf8_str);
        ASSERT_EQ(*read_ptr, first_char_bytes[c]);
    }
}

TEST(UTF8, GetCharBack) {
    const char *test_str1 = "latin";
    const char first_char_bytes1[] = { 'l', 'a', 't', 'i', 'n', 0 };
    const uint8_t test_str2[] =
    // latin symbols surrounded by unicode
    // 0xE2, 0x80, 0x80 - unicode symbol 0x2000 (non printable)
    { 0xE2, 0x80, 0x80, 0xE2, 0x80, 0x80, 'l', 'a', 't', 'i', 'n', 0xE2, 0x80, 0x80, 0xE2, 0x80, 0x80, 0 };
    const uint8_t first_char_bytes2[] = { 0xE2, 0xE2, 'l', 'a', 't', 'i', 'n', 0xE2, 0xE2, 0 };
    const uint8_t test_str3[] =
    // unicode surrounded by spaces
    // kon ni chi ha
    { ' ', ' ', 0xE3, 0x81, 0x93, 0xE3, 0x82, 0x93, 0xE3, 0x81, 0xAB, 0xE3, 0x81, 0xA1, 0xE3, 0x81, 0xAF, ' ', ' ', 0 };
    const uint8_t first_char_bytes3[] = { ' ', ' ', 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, ' ', ' ', 0 };
    // unicode only
    const uint8_t test_str4[] =
    { 0xE2, 0x80, 0x80, 0xE2, 0x80, 0x80, 0xE3, 0x81, 0x93, 0xE3, 0x82, 0x93, 0xE3, 0x81, 0xAB, 0xE3, 0x81, 0xA1, 0xE3, 0x81, 0xAF, 0xE2, 0x80, 0x80, 0xE2, 0x80, 0x80, 0 };
    const uint8_t first_char_bytes4[] = { 0xE2, 0xE2, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE2, 0xE2, 0 };

    TestGetCharBack(test_str1, first_char_bytes1);
    TestGetCharBack(reinterpret_cast<const char*>(test_str2), reinterpret_cast<const char*>(first_char_bytes2));
    TestGetCharBack(reinterpret_cast<const char*>(test_str3), reinterpret_cast<const char*>(first_char_bytes3));
    TestGetCharBack(reinterpret_cast<const char*>(test_str4), reinterpret_cast<const char*>(first_char_bytes4));
}
