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
#ifndef __CS_PARSER_TEST_LIB_H
#define __CS_PARSER_TEST_LIB_H

#include "script2/cc_internallist.h"
#include "script2/cc_compiledscript.h"
#include "script2/cc_symboltable.h"
#include "script2/cs_parser_common.h"

extern void clear_error(void);

extern const char *last_seen_cc_error(void);

extern char kAgsHeaderString[];

extern char kAgsHeaderBool[];

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


#endif
