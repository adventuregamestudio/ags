//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "gtest/gtest.h"
#include "data/data_helpers.h"

using namespace AGS::Common;

TEST(DataHelpers, Encrypt) {
    String s = "abcdefghijklmnopqrstuvwxyz";
    std::vector<char> buf(s.GetLength() + 1);
    std::copy(s.GetCStr(), s.GetCStr() + s.GetLength() + 1, buf.data());
    EncryptText(buf.data(), buf.size());
    const char result[] =
    { (char)('a' + 'A'), (char)('b' + 'v'), (char)('c' + 'i'), (char)('d' + 's'), (char)('e' + ' '),
      (char)('f' + 'D'), (char)('g' + 'u'), (char)('h' + 'r'), (char)('i' + 'g'), (char)('j' + 'a'), (char)('k' + 'n'),
      (char)('l' + 'A'), (char)('m' + 'v'), (char)('n' + 'i'), (char)('o' + 's'), (char)('p' + ' '),
      (char)('q' + 'D'), (char)('r' + 'u'), (char)('s' + 'r'), (char)('t' + 'g'), (char)('u' + 'a'), (char)('v' + 'n'),
      (char)('w' + 'A'), (char)('x' + 'v'), (char)('y' + 'i'), (char)('z' + 's'),  (char)(0  + ' ')};

    for (size_t i = 0; i < buf.size(); ++i)
        ASSERT_EQ(buf[i], result[i]);
}
