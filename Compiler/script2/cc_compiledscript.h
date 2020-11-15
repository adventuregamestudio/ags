#ifndef __CC_COMPILEDSCRIPT_H
#define __CC_COMPILEDSCRIPT_H

#include <string>
#include <unordered_map>
#include "script/cc_script.h"
#include "cs_parser_common.h"   // macro definitions and typedefs
#include "cc_symboltable.h"     // ScopeType

namespace AGS {
class ccCompiledScript : public ccScript {

    // These track how much space has been allocated for the different memory chunks.
    size_t _globaldataAllocated = 0u;
    size_t _codeAllocated = 0u;
    size_t _stringsAllocated = 0u;
    size_t _fixupsAllocated = 0u;
    size_t _fixupTypesAllocated = 0u;
    size_t _importsAllocated = 0u;
    size_t _exportsAllocated = 0u;
    size_t _exportAddrAllocated = 0u;
    size_t _sectionNamesAllocated = 0u;
    size_t _sectionOffsetsAllocated = 0u;

    // Resize the memory chunk at 'start' that currently has 'allocated' bytes allocated
    // so that it has at least 'MAX(needed, min_size)' bytes allocated.
    // The chunk is allocated 'malloc' style, so it needs to be 'free'd after use.
    // Returns kERR_InternalError if reallocation fails.
    ErrorType ResizeMemory(size_t needed, size_t min_size, void *&start, size_t &allocated);

    inline ErrorType ResizeChunk(size_t needed, size_t min_size, void *&start, size_t &allocated)
        { return allocated >= needed ? kERR_None : ResizeMemory(needed, min_size, start, allocated); }

public:
    struct FuncProps
    {
        std::string Name;
        size_t NumOfParams;
        CodeLoc CodeOffs;
    };
    std::vector<FuncProps> Functions;

    std::unordered_map<std::string, int> ImportIdx;
    std::unordered_map<std::string, int> ExportIdx;

    // Number of bytes that have been PUSHED onto the stack. Local variables begin below that
    size_t OffsetToLocalVarBlock;

    int LastEmittedLineno;
    bool EmitLineNumbers;

    // Variable type of value in AX, usually equiv. to type of the current expression
    Vartype AX_Vartype;

    // Needed to prevent a return of a local string
    ScopeType AX_ScopeType;

    void FreeExtra();

    // Reserve siz bytes of memory for global data;
    // copy the value at vall into this new memory space if given; 
    // return the offset at which the new space begins.
    GlobalLoc AddGlobal(size_t value_size, void *value_ptr);

    // Add a string literal to the strings[] repository
    // NOTE: Return is the start of the new string, not an error value.
    StringsLoc AddString(std::string const &literal);

    // Set a fixup to the code location given. Depending on the fixup type,
    // where can be an offset to the global memory, too.
    // Returns -1 on allocation error.
    int AddFixup(CodeLoc where, FixupType ftype);

    // Set a fixup to the last code cell written
    inline void FixupPrevious(FixupType ftype) { AddFixup(codesize - 1, ftype); };

    // Add a function named 'func_name' to the functions repository
    CodeLoc AddNewFunction(std::string const &func_name, size_t num_of_parameters);

    inline bool IsImport(std::string const &name) const { return 0 < ImportIdx.count(name); }

    // Find or add an import to the import repository; return the index of the import
    int FindOrAddImport(std::string const &import_name);

    // Add an exported entity to the export repository;
    // it has type vartype, resides at location; if it is a function
    // Note: This function returns -1 on error
    int AddExport(std::string const &name, CodeLoc location, size_t num_of_arguments = INT_MAX);

    // Start a new section of the code.
    int StartNewSection(std::string const &name);

    // Write one Bytecode byte    
    void WriteCode(CodeCell cell);

    // Write a command
    inline void WriteCmd(CodeCell op) { WriteCode(op); }

    inline void WriteCmd(CodeCell op, CodeCell p1)
        { WriteCode(op); WriteCode(p1); }
    inline void WriteCmd(CodeCell op, CodeCell p1,CodeCell p2)
        { WriteCode(op); WriteCode(p1); WriteCode(p2); };
    inline void WriteCmd(CodeCell op, CodeCell p1, CodeCell p2, CodeCell p3)
        { WriteCode(op); WriteCode(p1); WriteCode(p2); WriteCode(p3); };

    // Write Bytecode that couples this point of the Bytecode with the source code line lno
    void WriteLineno(size_t lno);
    // Only write Bytecode for source line no if it differs from the last emitted
    inline void refresh_lineno(size_t lno) { if (LastEmittedLineno != lno) WriteLineno(lno); }

    // write a PUSH command; track in OffsetToLocalVarBlock the number of bytes pushed to the stack
    void push_reg(CodeCell regg);
    // write a POP command; track in OffsetToLocalVarBlock the number of bytes pushed to the stack
    void pop_reg(CodeCell regg);

    // Returns the relative distance in a jump instruction
    // "here" is the location of the bytecode that will contain
    // the (relative) destination.It is not the location of the
    // start of the command but the location of its first parameter
    inline static CodeLoc RelativeJumpDist(CodeLoc here, CodeLoc dest) { return dest - here - 1; }

    ccCompiledScript(bool emit_line_numbers = true);
    virtual ~ccCompiledScript();
};
}
#endif // __CC_COMPILEDSCRIPT_H
