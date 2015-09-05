#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions
#include "script/cc_treemap.h"

#include <map>
#include <string>
#include <vector>

struct symbolTable {
    int numsymbols;
    int currentscope;
    int normalIntSym;
    int normalStringSym;
    int normalFloatSym;
    int normalVoidSym;
    int nullSym;
    int stringStructSym;
    std::vector<short> stype;
    std::vector<long> flags;
    std::vector<short> vartype;
    std::vector<int> soffs;
    std::vector<long> ssize; // or return type size for function
    std::vector<short> sscope; // or num arguments for function
    std::vector<long> arrsize;
    std::vector<short> extends; // inherits another class (classes) / owning class (member vars)
    // functions only, save types of return value and all parameters
    std::vector<std::vector<unsigned long> > funcparamtypes;
    std::vector< std::vector<int> > funcParamDefaultValues;
    std::vector< std::vector<bool> > funcParamHasDefaultValues;
    char tempBuffer[2][MAX_SYM_LEN];
    int  usingTempBuffer;

    ccTreeMap symbolTree;

    symbolTable();
    void reset();    // clears table
    int  find(const char*);  // returns ID of symbol, or -1
    int  add_ex(char*,int,char);  // adds new symbol of type and size
    int  add_operator(char*, int priority, int vcpucmd); // adds new operator
    int  add(char*);   // adds new symbol, returns -1 if already exists
    int  get_num_args(int funcSym);
    std::string symbolTable::get_friendly_name(int idx);  // inclue ptr
    std::string symbolTable::get_name_string(int idx);
    char *get_name(int idx); // gets symbol name of index
    int  get_type(int ii);
    int  operatorToVCPUCmd(int opprec);
    int  is_loadable_variable(int symm);

    void set_propfuncs(int symb, int propget, int propset);
    int get_propget(int symb);
    int get_propset(int symb);

private:
    std::vector<std::string> sname;
    std::vector<char *> symbolTreeNames;

    std::map<int, char *> nameGenCache;
};



extern symbolTable sym;

#endif //__CC_SYMBOLTABLE_H
