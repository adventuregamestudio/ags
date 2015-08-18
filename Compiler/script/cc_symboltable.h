#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions
#include "script/cc_treemap.h"
#include <vector>
#include <map>
#include <string>


// So there's another symbol definition in cc_symboldef.h
struct SymbolTableEntry {
	std::string sname;
	short stype;
	long flags;
	short vartype;
	int soffs;
	long ssize; // or return type size for function
	short sscope; // or num arguments for function
	long arrsize;
	short extends; // inherits another class (classes) / owning class (member vars)
    // functions only, save types of return value and all parameters
    std::vector<unsigned long> funcparamtypes;
    std::vector<short> funcParamDefaultValues;

	int get_num_args();

	int is_loadable_variable();

	void set_propfuncs(int propget, int propset);
	int get_propget();
	int get_propset();

	int operatorToVCPUCmd();
};

struct symbolTable {
    int numsymbols;

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
    int  add_ex(char*,int,char);  // adds new symbol of type and size
    int  add(char*);   // adds new symbol, returns -1 if already exists
    char *get_name(int); // gets symbol name of index
	std::string get_name_string(int idx);
    int  get_type(int ii);


private: 

    std::map<int, char *> nameGenCache;

    ccTreeMap symbolTree;

    int  add_operator(char*, int priority, int vcpucmd); // adds new operator
};


extern symbolTable sym;

#endif //__CC_SYMBOLTABLE_H