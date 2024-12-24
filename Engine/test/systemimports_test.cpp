//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
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

ScriptSymbolsMap sym('^', true);

TEST(SystemImports, ScriptSymbolsMap_GetIndexOf) {
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
    sym.Add("Func", 0);
    sym.Add("Function", 1);
    sym.Add("Function^1", 2);
    sym.Add("FunctionLong^1", 3);
    sym.Add("FunctionLong^3", 4);
    sym.Add("FunctionLong^5", 5);
    sym.Add("FunctionNoAppendage", 6);
    sym.Add("FunctionWithLongAppendage^123", 7);
    sym.Add("FunctionWithLongAppendage^12345", 8);

    ASSERT_EQ(sym.GetIndexOfAny("Unknown"), UINT32_MAX);
    // Exact matches always have priority
    ASSERT_EQ(sym.GetIndexOfAny("Function"), 1);
    ASSERT_EQ(sym.GetIndexOfAny("Function^1"), 2);
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong^5"), 5);
    ASSERT_EQ(sym.GetIndexOfAny("FunctionNoAppendage"), 6);
    ASSERT_EQ(sym.GetIndexOfAny("FunctionWithLongAppendage^123"), 7);
    // Request without appendage
    // - exact match is the one without appendage,
    // - otherwise, first found match of the base name
    ASSERT_EQ(sym.GetIndexOfAny("Function"), 1); // "Function" - exact match
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong"), 3); // "FunctionLong^1" - first match of the base name
    ASSERT_EQ(sym.GetIndexOfAny("FunctionNoAppendage"), 6); // "FunctionNoAppendage" - exact match
    ASSERT_EQ(sym.GetIndexOfAny("FunctionWithLongAppendage"), 7); // "FunctionWithLongAppendage^123" - first match of the base name
    // Request with appendage
    // - exact match is the one that matches appendage,
    // - otherwise, match the one without appendage
    ASSERT_EQ(sym.GetIndexOfAny("Function^1"), 2); // "Function^1" - exact match
    ASSERT_EQ(sym.GetIndexOfAny("Function^2"), 1); // "Function" - no appendage match found, best match of the base name
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong^0"), UINT32_MAX); // not matching any variant
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong^10"), UINT32_MAX); // not matching any variant
    ASSERT_EQ(sym.GetIndexOfAny("FunctionLong^3"), 4); // "FunctionLong^3" - exact match
    ASSERT_EQ(sym.GetIndexOfAny("FunctionNoAppendage^5"), 6); // "FunctionNoAppendage" - no appendage match found, best match of the base name
    ASSERT_EQ(sym.GetIndexOfAny("FunctionWithLongAppendage^1"), UINT32_MAX); // not matching any variant
    ASSERT_EQ(sym.GetIndexOfAny("FunctionWithLongAppendage^123"), 7); // "FunctionWithLongAppendage^123" - exact match
    ASSERT_EQ(sym.GetIndexOfAny("FunctionWithLongAppendage^123456"), UINT32_MAX); // not matching any variant
}
