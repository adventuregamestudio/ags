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
    ErrorType ResizeMemory(size_t el_size, size_t needed, size_t min_allocated, void *&start, size_t &allocated);

    inline ErrorType ResizeChunk(size_t el_size, size_t needed, size_t min_allocated, void *&start, size_t &allocated)
        { return allocated >= needed ? kERR_None : ResizeMemory(el_size, needed, min_allocated, start, allocated); }

public:
    struct FuncProps
    {
        std::string Name;
        size_t NumOfParams;
        CodeLoc CodeOffs;
    };
    std::vector<FuncProps> Functions = {};

    std::unordered_map<std::string, int> ImportIdx = {};
    std::unordered_map<std::string, int> ExportIdx = {};

    // Number of bytes that have been PUSHED onto the stack. Local variables begin below that
    size_t OffsetToLocalVarBlock = 0u;

    int LastEmittedLineno = INT_MAX;
    bool EmitLineNumbers;

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
    ErrorType StartNewSection(std::string const &name);

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
    inline void RefreshLineno(size_t lno) { if (LastEmittedLineno != lno) WriteLineno(lno); }

    // write a PUSH command; track in OffsetToLocalVarBlock the number of bytes pushed to the stack
    void PushReg(CodeCell regg);
    // write a POP command; track in OffsetToLocalVarBlock the number of bytes pushed to the stack
    void PopReg(CodeCell regg);

    // Returns the relative distance in a jump instruction
    // "here" is the location of the bytecode that will contain
    // the (relative) destination.It is not the location of the
    // start of the command but the location of its first parameter
    inline static CodeLoc RelativeJumpDist(CodeLoc here, CodeLoc dest) { return dest - here - 1; }

    ccCompiledScript(bool emit_line_numbers = true);
    virtual ~ccCompiledScript();
};

// Encapusulates a point of generated code.
class RestorePoint
{
private:
    ccCompiledScript &_scrip;
    size_t _rememberedCodeLocation;

public:
    RestorePoint(ccCompiledScript &scrip)
        : _scrip(scrip)
        , _rememberedCodeLocation(scrip.codesize)
    { }

    // Discard all code that has been generated since the object was created
    // Also discard the corresponding fixups
    // This is useful if code is generated and it turns out later that it was in vain.
    void Restore();

    inline bool IsEmpty() const { return _scrip.codesize <= static_cast<int>(_rememberedCodeLocation); }
};

// A storage for parameters of forward jumps. When at some later time,
// Patch() is called, then all the jumps will be patched to point to the current
// point in code. If appropriate, the last emitted src line will be invalidated, too.
class ForwardJump
{
private:
    ccCompiledScript &_scrip;
    std::vector<CodeLoc> _jumpDestParamLocs;
    size_t  _lastEmittedSrcLineno;

public:
    ForwardJump(ccCompiledScript &scrip)
        : _scrip(scrip)
        , _lastEmittedSrcLineno(INT_MAX)
    {}

    // Add the parameter of a forward jump 
    void AddParam(int offset = -1);

    // Patch all the forward jump parameters to point to _scrip.codesize
    void Patch(size_t cur_line);
};

// Remember a point of the bytecode that is going to be the destination
// of a backward jump. When at some later time, WriteJump() is called,
// then the appropriate instruction(s) for a backward jump are generated.
// This may entail a SCMD_LINENUM command.
// This may make the last emitted src line invalid.
class BackwardJumpDest
{
private:
    ccCompiledScript &_scrip;
    CodeLoc _dest;
    size_t  _lastEmittedSrcLineno;
public:
    BackwardJumpDest(ccCompiledScript &scrip)
        : _scrip(scrip)
        , _dest(-1)
        , _lastEmittedSrcLineno(INT_MAX)
    { }

    // Set the destination to the location given; default to current location in code
    void Set(CodeLoc cl = -1);
    inline CodeLoc Get() const { return _dest; }
    // Write a jump to the code location that I represent
    void WriteJump(CodeCell jump_op, size_t cur_line);
};

} // namespace AGS
#endif // __CC_COMPILEDSCRIPT_H
