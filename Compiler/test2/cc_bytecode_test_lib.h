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

struct CharRun
{
    size_t count;
    unsigned char value;
    CharRun(size_t c, unsigned char v) : count(c), value(v) {}
};

extern void CompareCode(AGS::ccCompiledScript *scrip, size_t code_size, int32_t code[]);
extern void CompareFixups(AGS::ccCompiledScript *scrip, size_t fixups_size, int32_t fixups[], char fixuptypes[]);
extern void CompareImports(AGS::ccCompiledScript *scrip, size_t non_empty_imports_count, std::string imports[]);
extern void CompareExports(AGS::ccCompiledScript *scrip, size_t exports_size, std::string exports[], int32_t export_addr[]);
extern void CompareStrings(AGS::ccCompiledScript *scrip, size_t strings_size, char strings[]);
extern void CompareGlobalRuns(AGS::ccCompiledScript *scrip, struct CharRun global_runs[]);
extern void CompareGlobalData(AGS::ccCompiledScript *scrip, size_t size, unsigned char gdata[]);

#endif
