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
#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions
#include "script/cc_treemap.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

// So there's another symbol definition in cc_symboldef.h
struct SymbolTableEntry {
	std::string sname;
	int16_t stype;
	int32_t flags;
	int16_t vartype;
	int soffs;
	int32_t ssize; // or return type size for function
	int16_t sscope; // or num arguments for function
	int32_t arrsize;
	int16_t extends; // inherits another class (classes) / owning class (member vars)
    // functions only, save types of return value and all parameters
    std::vector<uint32_t> funcparamtypes;
    std::vector<int32_t> funcParamDefaultValues;
    std::vector<bool> funcParamHasDefaultValues;

	int get_num_args();

	int is_loadable_variable();

    // Set indexes of get/set property handlers; 0xffff for no entry
	void set_propfuncs(int propget, int propset);
    // Returns an index of get property handler; -1 means no entry
	int get_propget();
    // Returns an index of set property handler; -1 means no entry
	int get_propset();

	int operatorToVCPUCmd();
};

struct symbolTable {
	// index for predefined symbols
    int normalIntSym;
    int normalStringSym;
    int normalFloatSym;
    int normalVoidSym;
    int nullSym;
    int stringStructSym;  // can get overwritten with new String symbol defined in agsdefns.sh

	// properties for symbols, size is numsymbols
	std::vector<SymbolTableEntry> entries;

    symbolTable();
    void reset();    // clears table
    int  find(const char*);  // returns ID of symbol, or -1
    int  add_ex(const char*,int,char);  // adds new symbol of type and size
    int  add(const char*);   // adds new symbol, returns -1 if already exists

    // TODO: why is there "friendly name" and "name", and what's the difference?
    std::string get_friendly_name(int idx);  // inclue ptr
    const char *get_name(int idx); // gets symbol name of index

    int  get_type(int ii);


private:

    std::map<int, char *> nameGenCache;

    ccTreeMap symbolTree;
    std::vector<char *> symbolTreeNames;

    int  add_operator(const char*, int priority, int vcpucmd); // adds new operator
    std::string get_name_string(int idx);
};


extern symbolTable sym;

#endif //__CC_SYMBOLTABLE_H
