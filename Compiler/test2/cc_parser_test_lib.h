#ifndef __CS_PARSER_TEST_LIB_H
#define __CS_PARSER_TEST_LIB_H

#include "script2/cc_internallist.h"
#include "script2/cc_compiledscript.h"
#include "script2/cc_symboltable.h"
#include "script2/cs_parser_common.h"

extern void clear_error(void);

extern const char *last_seen_cc_error(void);

extern char g_Input_String[];

extern char g_Input_Bool[];

// Only use this function for googletests. Scan and tokenize the input.
extern int cc_scan(
    std::string const &inpl,        // preprocessed text to be tokenized
    AGS::SrcList &src,              // store for the tokenized text
    AGS::ccCompiledScript &scrip,   // repository for the strings in the text
    AGS::SymbolTable &symt,         // symbol table
    AGS::MessageHandler &mh);       // warnings and the error

// Only use this function for googletests. Parse the input
extern int cc_parse(
    AGS::SrcList &src,              // tokenized text
    AGS::FlagSet options,           // as defined in cc_options 
    AGS::ccCompiledScript &scrip,   // result of the compilation
    AGS::SymbolTable &symt,         // symbol table
    AGS::MessageHandler &mh);       // warnings and the error 

// Compile the input; in case of error cc_error() gets called
extern int cc_compile(
    std::string const &source,      // preprocessed text to be compiled
    AGS::ccCompiledScript &scrip);  // store for the compiled text
#endif
