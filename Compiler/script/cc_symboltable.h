#ifndef __CC_SYMBOLTABLE_H
#define __CC_SYMBOLTABLE_H

#include "cs_parser_common.h"   // macro definitions
#include "cs/cc_treemap.h"

struct symbolTable {
    int numsymbols;
    int currentscope;
    int normalIntSym;
    int normalStringSym;
    int normalFloatSym;
    int normalVoidSym;
    int nullSym;
    int stringStructSym;
    char* sname[MAXSYMBOLS];
    short stype[MAXSYMBOLS];
    long  flags[MAXSYMBOLS];
    short vartype[MAXSYMBOLS];
    int   soffs[MAXSYMBOLS];
    long  ssize[MAXSYMBOLS];  // or return type size for function
    short sscope[MAXSYMBOLS];  // or num arguments for function
    long  arrsize[MAXSYMBOLS];
    short extends[MAXSYMBOLS]; // inherits another class (classes) / owning class (member vars)
    // functions only, save types of return value and all parameters
    unsigned long funcparamtypes[MAXSYMBOLS][MAX_FUNCTION_PARAMETERS+1]; 
    short funcParamDefaultValues[MAXSYMBOLS][MAX_FUNCTION_PARAMETERS+1]; 
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
    char*get_name(int); // gets symbol name of index
    int  get_type(int ii);
    int  operatorToVCPUCmd(int opprec);
    int  is_loadable_variable(int symm);

    void set_propfuncs(int symb, int propget, int propset);
    int get_propget(int symb);
    int get_propset(int symb);
};



extern symbolTable sym;

#endif //__CC_SYMBOLTABLE_H