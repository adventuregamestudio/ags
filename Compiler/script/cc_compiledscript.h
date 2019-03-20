#ifndef __CC_COMPILEDSCRIPT_H
#define __CC_COMPILEDSCRIPT_H

#include <string>
#include "script/cc_script.h"       // ccScript
#include "cs_parser_common.h"   // macro definitions
#include "cc_symboldef.h"       // SymbolDef


struct ccCompiledScript: public ccScript {
    long codeallocated;
    char*functions[MAX_FUNCTIONS];
    long funccodeoffs[MAX_FUNCTIONS];
    short funcnumparams[MAX_FUNCTIONS];
    long numfunctions;
    long cur_sp;
    int  next_line;  // line number of next code
    int  ax_val_type;  // type of value in AX
    int  ax_val_scope;  // SYM_LOCALVAR or SYM_GLOBALAVR

    void init();
    void shutdown();
    void free_extra();
    int  add_global(int,const char*);
    int  add_string(const char*);
    void add_fixup(int32_t,char);
    void fixup_previous(char);
    int  add_new_function(const char*, int *idx);
    int  add_new_import(const char*);
    int  add_new_export(const char*,int,long, int);
    void write_code(int32_t);
    void set_line_number(int nlum) { next_line=nlum; }
    void flush_line_numbers();
    int  remove_any_import(const char*, SymbolDef *oldSym);
    const char* start_new_section(const char *name);

    void write_cmd(int cmdd);
    void write_cmd1(int cmdd,int param);
    void write_cmd2(int cmdd,int param,int param2);
    void write_cmd3(int cmdd,int param,int param2,int param3);

    void push_reg(int regg);
    void pop_reg(int regg);

    ccCompiledScript();
    virtual ~ccCompiledScript();
};

#endif // __CC_COMPILEDSCRIPT_H