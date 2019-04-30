#ifndef __CC_COMPILEDSCRIPT_H
#define __CC_COMPILEDSCRIPT_H

#include <string>

#include "script/cc_script.h"   
#include "cs_parser_common.h"   // macro definitions
#include "cc_symboltable.h"     // SymbolTableEntry


struct ccCompiledScript : public ccScript {
    long codeallocated;
    char *functions[MAX_FUNCTIONS];
    long funccodeoffs[MAX_FUNCTIONS];
    short funcnumparams[MAX_FUNCTIONS];
    long numfunctions;
    long cur_sp;
    int  next_line;  // line number of next code
    AGS::Vartype ax_vartype;  // type of value in AX, usually equiv. to type of the current expression
    AGS::SType  ax_val_scope;  // kSYM_LocalVar or kSYM_GlobalVar, needed to prevent a return of a local string

    void init();
    void shutdown();
    void free_extra();

    // Reserve siz bytes of memory for global data;
    // copy the value at vall into this new memory space if given; 
    // return the offset at which the new space begins.
    int  add_global(int siz, const char *vall);

    int  add_string(const char *);
    void add_fixup(int32_t, char);
    void fixup_previous(char);
    int  add_new_function(const char *, int *idx);
    int  add_new_import(const char *);
    int  add_new_export(const char *, int, long, int);
    void write_code(intptr_t);
    inline void set_line_number(int nlum) { next_line = nlum; }
    void flush_line_numbers();
    const char *start_new_section(const char *name);

    inline void write_cmd0(int cmdd) { write_code(cmdd); };
    inline void write_cmd1(int cmdd, int param) { write_code(cmdd); write_code(param); };
    inline void write_cmd2(int cmdd, int param1, int param2) { write_code(cmdd); write_code(param1); write_code(param2); };
    inline void write_cmd3(int cmdd, int param1, int param2, int param3) { write_code(cmdd); write_code(param1); write_code(param2); write_code(param3); };

    void push_reg(int regg);
    void pop_reg(int regg);

    int ccCompiledScript::copy_import_symbol_table_entry(AGS::Symbol idx, SymbolTableEntry *dest);
    int ccCompiledScript::just_remove_any_import(AGS::Symbol idx);

    ccCompiledScript();
    virtual ~ccCompiledScript();
};

#endif // __CC_COMPILEDSCRIPT_H
