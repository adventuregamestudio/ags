#ifndef CC_BYTECODE_TEST_LIB
#define CC_BYTECODE_TEST_LIB

#include <iostream>  
#include "script2/cc_compiledscript.h"

// Should contain a path to which the tests can write copies of their checking code
constexpr char *WRITE_PATH = "C:/TEMP/";

extern void WriteOutput(char *fname, AGS::ccCompiledScript const &scrip);

extern void CompareCode(AGS::ccCompiledScript *scrip, size_t codesize, int32_t code[]);
extern void CompareFixups(AGS::ccCompiledScript *scrip, size_t numfixups, int32_t fixups[], char fixuptypes[]);
extern void CompareImports(AGS::ccCompiledScript *scrip, size_t numimports, std::string imports[]);
extern void CompareExports(AGS::ccCompiledScript *scrip, size_t numexports, std::string exports[], int32_t export_addr[]);
extern void CompareStrings(AGS::ccCompiledScript *scrip, size_t stringssize, char strings[]);

#endif
