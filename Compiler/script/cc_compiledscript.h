#ifndef __CC_COMPILEDSCRIPT_H
#define __CC_COMPILEDSCRIPT_H

#include <string>
#include "script/cc_script.h"   
#include "cs_parser_common.h"   // macro definitions and typedefs
#include "cc_symboltable.h"     // SymbolTableEntry


struct ccCompiledScript : public ccScript {
    struct FuncProps
    {
        std::string Name;
        size_t NumOfParams;
        AGS::CodeLoc CodeOffs;
    };
    std::vector<FuncProps> functions;

    long codeallocated; // [fw] Misplaced. Should be in ccScript.
    
    // Number of bytes that have been PUSHED onto the stack.
    // Local variables begin below that
    size_t cur_sp; 

    int last_emitted_lineno;  

    // Variable type of value in AX, usually equiv. to type of the current expression
    AGS::Vartype ax_vartype;

    // kSYM_LocalVar or kSYM_GlobalVar, needed to prevent a return of a local string
    AGS::SType  ax_val_scope;  

    void init();
    void shutdown();
    void free_extra();    
    
    // Reserve siz bytes of memory for global data;
    // copy the value at vall into this new memory space if given; 
    // return the offset at which the new space begins.
    AGS::GlobalLoc add_global(size_t siz, void *vall);

    // Add a string literal to the strings[] repository
    // NOTE: Return is the start of the new string, not an error value.
    AGS::StringsLoc add_string(std::string const &literal);

    // Set a fixup to the code location given. Depending on the fixup type,
    // where can be an offset to the global memory, too.
    void add_fixup(AGS::CodeLoc where, AGS::FixupType ftype);

    // Set a fixup to the last code cell written
    inline void fixup_previous(AGS::FixupType ftype) { add_fixup(codesize - 1, ftype); };

    // Add a function named 'name' to the functions repository; if set, index_allocated gets where it is stored
    // Returns the offset of the function start in the code[] space
    AGS::CodeLoc add_new_function(std::string const &name, int *index_allocated = nullptr);

    // Add an import to the import repository; return the index of the import
    int add_new_import(std::string const &name);

    // Add an exported entity to the export repository;
    // it has type vartype, resides at location; if it is a function
    // Note: This function returns -1 on error
    int add_new_export(std::string const &name, AGS::Exporttype etype, AGS::CodeLoc location, size_t num_of_arguments = 0);

    // Start a new section of the code.
    std::string start_new_section(std::string const &name);

    // Write one Bytecode byte    
    void write_code(AGS::CodeCell code);

    // Write a command
    inline void write_cmd(AGS::CodeCell op)
        { write_code(op); };
    inline void write_cmd(AGS::CodeCell op, AGS::CodeCell p1)
        { write_code(op); write_code(p1); };
    inline void write_cmd(AGS::CodeCell op, AGS::CodeCell p1, AGS::CodeCell p2)
        { write_code(op); write_code(p1); write_code(p2); };
    inline void write_cmd(AGS::CodeCell op, AGS::CodeCell p1, AGS::CodeCell p2, AGS::CodeCell p3)
        { write_code(op); write_code(p1); write_code(p2); write_code(p3); };
    // Write pseudo command that couples this point of the Bytecode
    // with the source code line lno
    void write_lineno(size_t lno);


    // write a PUSH command; track in cur_sp the number of bytes pushed to the stack
    void push_reg(AGS::CodeCell regg);
    // write a POP command; track in cur_sp the number of bytes pushed to the stack
    void pop_reg(AGS::CodeCell regg);

    // Returns the relative distance in a jump instruction
    // "here" is the location of the bytecode that will contain
    // the (relative) destination.It is not the location of the
    // start of the command but the location of its first parameter
    inline static AGS::CodeLoc RelativeJumpDist(AGS::CodeLoc here, AGS::CodeLoc dest) { return dest - here - 1; }

    ccCompiledScript();
    virtual ~ccCompiledScript();
};

#endif // __CC_COMPILEDSCRIPT_H
