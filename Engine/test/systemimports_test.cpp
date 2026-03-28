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
#include "script/systemimports.h"

using namespace AGS::Common;

TEST(SystemImports, ScriptSymbolsMap_GetIndexOf) {
    ScriptSymbolsMap sym;
    sym.Add("Function", 0);
    sym.Add("Function^1", 1);
    sym.Add("FunctionLong^5", 2);

    ASSERT_EQ(sym.GetIndexOf("Unknown"), UINT32_MAX);
    ASSERT_EQ(sym.GetIndexOf("Function"), 0);
    ASSERT_EQ(sym.GetIndexOf("Function^1"), 1);
    ASSERT_EQ(sym.GetIndexOf("Function^2"), UINT32_MAX);
    ASSERT_EQ(sym.GetIndexOf("FunctionLong"), UINT32_MAX);
    ASSERT_EQ(sym.GetIndexOf("FunctionLong^5"), 2);
}

TEST(SystemImports, ScriptSymbolsMap_GetIndexOfAny) {
    ScriptSymbolsMap sym;
    // Import symbols
    sym.Add("Func", 0);
    sym.Add("Function", 1);
    sym.Add("Function^1", 2);
    sym.Add("Function^8", 3);
    sym.Add("FunctionLong^1", 4);
    sym.Add("FunctionLong^3", 5);
    sym.Add("FunctionLong^5", 6);
    sym.Add("FunctionLong^9", 7);
    sym.Add("FunctionNoAppendage", 8);
    sym.Add("FunctionWithLongAppendage^123", 9);
    sym.Add("FunctionWithLongAppendage^12345", 10);
    // Script export symbols
    sym.Add("Function$1", 11);
    sym.Add("Function$9", 12);
    sym.Add("FunctionLong$1", 13);
    sym.Add("FunctionLong$3", 14);
    sym.Add("FunctionLong$5", 15);
    sym.Add("FunctionLong$7", 16);
    sym.Add("FunctionWithLongAppendage$123", 17);
    sym.Add("FunctionWithLongAppendage$12345", 18);
    sym.Add("FunctionWithLongAppendage$345", 19);

    // Non-existing symbol
    ASSERT_EQ(sym.GetIndexOfAny("Unknown"), UINT32_MAX);
    ASSERT_EQ(sym.GetIndexOfAny("Functio"), UINT32_MAX);
    ASSERT_EQ(sym.GetIndexOfAny("FunctionWithLongAppe"), UINT32_MAX);
    // Exact matches always have priority
    ASSERT_EQ(sym.GetIndexOfAny("Function"), 1);
    ASSERT_EQ(sym.GetIndexOfAny("Function^1"), 2);
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong^5"), 6);
    ASSERT_EQ(sym.GetIndexOfAny("FunctionNoAppendage"), 8);
    ASSERT_EQ(sym.GetIndexOfAny("FunctionWithLongAppendage^123"), 9);
    ASSERT_EQ(sym.GetIndexOfAny("Function$1"), 11);
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong$5"), 15);
    ASSERT_EQ(sym.GetIndexOfAny("FunctionWithLongAppendage$123"), 17);
    // Request without appendage, which does not have a direct match
    // - match the first found script export matching the base name, with any args
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong"), 13); // "FunctionLong$1" - first export match of the base name
    ASSERT_EQ(sym.GetIndexOfAny("FunctionWithLongAppendage"), 17); // "FunctionWithLongAppendage$123" - first export match of the base name
    // Request import name with appendage, which does not have a direct match
    // - match the one without appendage, if not then...
    // - match the script export with the matching appendage
    ASSERT_EQ(sym.GetIndexOfAny("Function^2"), 1); // "Function" - no appendage match found, best match of the base name
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong^0"), UINT32_MAX); // not matching any variant
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong^10"), UINT32_MAX); // not matching any variant
    ASSERT_EQ(sym.GetIndexOfAny("FunctionNoAppendage^5"), 8); // "FunctionNoAppendage" - no appendage match found, best match of the base name
    ASSERT_EQ(sym.GetIndexOfAny("Function^9"), 12); // "Function$9"
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong^7"), 16); // "FunctionLong$7"
    ASSERT_EQ(sym.GetIndexOfAny("FunctionWithLongAppendage^345"), 19); // "FunctionWithLongAppendage$345"
    // Request export name with appendage, which does not have a direct match
    // - match the one without appendage
    ASSERT_EQ(sym.GetIndexOfAny("Function$2"), 1); // "Function" - no appendage match found, best match of the base name
    ASSERT_EQ(sym.GetIndexOfAny("Function$8"), 1); // "Function" - no appendage match found, best match of the base name
    ASSERT_EQ(sym.GetIndexOfAny("FunctionExtra$1"), UINT32_MAX); // not matching any variant
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong$9"), UINT32_MAX); // not matching any variant
    ASSERT_EQ(sym.GetIndexOfAny("FunctionNoAppendage$1"), 8); // "FunctionNoAppendage"
}
