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
#ifndef CC_BYTECODE_TEST_LIB
#define CC_BYTECODE_TEST_LIB

#include <iostream>  
#include "script2/cc_compiledscript.h"

// Should contain a path to which the tests can write copies of their checking code
constexpr const char *WRITE_PATH = "C:/TEMP/";

extern void WriteOutput(const char *fname, AGS::ccCompiledScript const &scrip);

extern void CompareCode(AGS::ccCompiledScript *scrip, size_t codesize, int32_t code[]);
extern void CompareFixups(AGS::ccCompiledScript *scrip, size_t numfixups, int32_t fixups[], char fixuptypes[]);
extern void CompareImports(AGS::ccCompiledScript *scrip, size_t numimports, std::string imports[]);
extern void CompareExports(AGS::ccCompiledScript *scrip, size_t numexports, std::string exports[], int32_t export_addr[]);
extern void CompareStrings(AGS::ccCompiledScript *scrip, size_t stringssize, char strings[]);

#endif
