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
#ifndef __AGS_EE_SCRIPT__RUNTIMESCRIPT_H
#define __AGS_EE_SCRIPT__RUNTIMESCRIPT_H
#include <memory>
#include <vector>
#include "script/cc_internal.h" // FIXME: should not be included into this header
#include "script/cc_reflecthelper.h"
#include "script/cc_script.h"
#include "script/runtimescriptvalue.h"
#include "script/systemimports.h"
#include "util/string.h"

namespace AGS
{
namespace Engine
{

// We use 10 bits to hold instance IDs ORed with op-code
#define INSTANCE_ID_SHIFT       22LL
#define INSTANCE_ID_MASK        0x00000000000003FFLL
#define INSTANCE_ID_REMOVEMASK  0x00000000003FFFFFLL

enum ScriptOpArgIsReg
{
    kScOpNoArgIsReg     = 0,
    kScOpArg1IsReg      = 0x0001,
    kScOpArg2IsReg      = 0x0002,
    kScOpArg3IsReg      = 0x0004,
    kScOpOneArgIsReg    = kScOpArg1IsReg,
    kScOpTwoArgsAreReg  = kScOpArg1IsReg | kScOpArg2IsReg,
    kScOpTreeArgsAreReg = kScOpArg1IsReg | kScOpArg2IsReg | kScOpArg3IsReg
};

struct ScriptCommandInfo
{
    ScriptCommandInfo(const int32_t code, const char *cmdname, const int arg_count, const ScriptOpArgIsReg arg_is_reg)
        : Code(code), CmdName(cmdname), ArgCount(arg_count)
        , ArgIsReg {
            (arg_is_reg & kScOpArg1IsReg) != 0, 
            (arg_is_reg & kScOpArg2IsReg) != 0, 
            (arg_is_reg & kScOpArg3IsReg) != 0
        }
    {}

    const int32_t   Code = 0;
    const char     *CmdName = nullptr;
    const int       ArgCount = 0;
    const bool      ArgIsReg[3]{};
};

// FIXME: move this elsewhere, make available for both RuntimeScript and ccInstance (script runner)
extern const ScriptCommandInfo sccmd_info[CC_NUM_SCCMDS];


struct ScriptVariable
{
    ScriptVariable() = default;

    // original 32-bit relative data address, written in compiled script;
    // if we are to use Map or HashMap, this could be used as Key
    int32_t             ScAddress = -1;
    RuntimeScriptValue  RValue;
};

// Runtime variant of script data, fixups and imports,
// resolved after loading all the game scripts,
// and possibly shared among multiple script instance forks.
class RuntimeScript
{
    using String = AGS::Common::String;
    static const String _noname;
    static const String _unknownSectionName;
public:
    RuntimeScript();
    RuntimeScript(const String &tag);
    ~RuntimeScript();

    static std::unique_ptr<RuntimeScript> Create(const ccScript *script, const String &tag);
    // Joins custom provided RTTI into the global collection;
    // fills in maps for locid and typeid remap which may be used to know
    // which *global* ids were assigned to this particular rtti's entries.
    // Updates RTTIHelper correspondingly.
    static void JoinRTTI(const RTTI &rtti,
        std::unordered_map<uint32_t, uint32_t> &loc_l2g,
        std::unordered_map<uint32_t, uint32_t> &type_l2g);

    struct ResolvedExport
    {
        int Type = EXPORT_NONE; // EXPORT_* value
        int Offset = 0; // bytecode offset
        RuntimeScriptValue Value; // real pointer to data or executable code

        ResolvedExport() = default;
        ResolvedExport(int type, int offset, const RuntimeScriptValue &value)
            : Type(type), Offset(offset), Value(value) {}
    };

    // Get this script's name
    const String &GetScriptName() const;
    // Get an optional tag
    // TODO: this is for debugging purposes, review later
    const String &GetTag() const { return _tag; }
    // Returns the index that this script is assigned on linking stage
    int32_t       GetLinkIndex() const { return _linkIndex; }
    // Get a readonly access to the script's bytecode
    const std::vector<intptr_t> &GetCode() const { return _code; }
    // Get a readonly access to the bytecode fixups
    const std::vector<uint8_t> &GetCodeFixups() const { return _codeFixups; }
    // Get a readonly access to the global script data
    const std::vector<uint8_t> &GetGlobalData() const { return _globaldata; }
    // Get a readonly access to the script's literal strings
    const std::vector<char> &GetStrings() const { return _strings; }
    // Get a readonly access to the script's export names
    const std::vector<String> &GetExports() const { return _exports; }
    // Get a readonly access to the resolved exports
    const std::vector<ResolvedExport> &GetResolvedExports() const { return _resolvedExports; }
    // Get script section's name at a given bytecode offset
    const String &GetSectionName(int32_t offset) const;
    // Get an optional table of contents for this script
    const ScriptTOC *GetTOC() const { return _toc.get(); }
    const RTTI *GetRTTI() const { return _rtti.get(); }

    // Get the address of an exported symbol (function or variable) in the script
    RuntimeScriptValue GetSymbolAddress(const String &symname) const;
    // Searches for the function among this script's exports,
    // on success returns its starting position in bytecode, and number of arguments
    bool    FindExportedFunction(const String &fn_name, int32_t &start_at, int32_t &num_args) const;

    // Copies global data values over to this instance;
    // copies not more than the allocated size of global data
    void    CopyGlobalData(const std::vector<uint8_t> &data);

    // Adds this script's exports to the symbol table
    void    RegisterExports(SystemImports &simp);
    // Removes this script's exports from the symbol table
    void    UnRegisterExports(SystemImports &simp);
    // Links this script to others, resolving import/exports
    bool    ResolveImports(const SystemImports &simp);


    // TODO: move the "static" contents to a separate class that links RuntimeScripts
    // together (something like ScriptProgram?)
    static const JointRTTI *GetJointRTTI() { return _jointRtti.get(); }
    static const Engine::RTTIHelper *GetRTTIHelper() { return _rttiHelper.get(); }
    // Returns a dictionary that maps local script's typeid to global typeid (in joint RTTI)
    // Requires RTTI
    const std::unordered_map<uint32_t, uint32_t> &
        GetLocal2GlobalTypeMap() const { return _typeidLocal2Global; }
    const std::unordered_map<Common::String, uint32_t> &
        GetGlobalVariableLookup() const { return _globalVarLookup; }

private:
    bool    Create(const ccScript *script);

    bool    CreateGlobalVars();
    bool    AddGlobalVar(const ScriptVariable &glvar);
    ScriptVariable *FindGlobalVar(int32_t var_addr);
    bool    CreateRuntimeCodeFixups();
    bool    ResolveExports();

    // Using resolved imports array, resolve the IMPORT fixups
    // Also change CALLEXT op-codes to CALLAS when they pertain to a runtime script 
    bool    ResolveImportFixups();

    // 1024 because we use 10 bits to hold instance number
    static const int32_t MaxLinkedScripts = 1024;
    static RuntimeScript *  _linkedScripts[MaxLinkedScripts];

    // This script's name
    String                  _scriptname;
    String                  _tag;
    int32_t                 _linkIndex = -1;
    // Script's global data (for global variables)
    std::vector<uint8_t>    _globaldata;
    // Executed byte-code. Unlike ccScript's code array which is int32_t, the one
    // in RuntimeScript must be intptr_t to accomodate real pointers placed after
    // performing fixups.
    std::vector<intptr_t>   _code;
    std::vector<int32_t>    _fixups; // code array index to fixup (in ints)
    std::vector<uint8_t>    _fixupTypes; // FIXUP_* type per fixup index
    std::vector<int32_t>    _fixupValues; // fixup values per fixup index
    std::vector<uint8_t>    _codeFixups; // fixup type per each code entry
    // Resolved global variables
    std::unordered_map<int32_t, ScriptVariable> _globalvars;
    // Literal string data
    std::vector<char>       _strings;
    // This script's import names
    std::vector<String>     _imports;
    // Array of real import indexes used in script
    std::vector<uint32_t>   _resolvedImports;
    // This script's export names
    std::vector<String>     _exports;
    // Export addresses: high byte is type; low 24-bits are offset in bytecode
    std::vector<int32_t>    _exportAddr;
    std::vector<ResolvedExport> _resolvedExports;
    ScriptSymbolsMap        _exportLookup;
    // "Sections" allow the interpreter to find out which bit
    // of the code came from header files, and which from the main file
    std::vector<String>     _sectionNames;
    // Section offsets (relative to bytecode)
    std::vector<int32_t>    _sectionOffsets;

    // Extended information (optional)
    // RTTI, runtime type info: used for garbage collection, and debugging
    std::unique_ptr<RTTI>   _rtti;
    // Script table of contents (for debugging purposes)
    std::unique_ptr<ScriptTOC> _toc;

    // RTTI tables
    static std::unique_ptr<JointRTTI> _jointRtti;
    // Full name to global id (global id is an actual index in the joint rtti table)
    static std::unordered_map<Common::String, uint32_t> _rttiLookup;
    // Helper data for quicker RTTI analyzis
    static std::unique_ptr<Engine::RTTIHelper> _rttiHelper;

    // Map local script's location id to global (program-wide)
    std::unordered_map<uint32_t, uint32_t> _locidLocal2Global;
    // Map local script's type id to global (program-wide)
    std::unordered_map<uint32_t, uint32_t> _typeidLocal2Global;
    // Global variables name-to-index lookup (in script's TOC)
    std::unordered_map<Common::String, uint32_t> _globalVarLookup;
};

typedef std::shared_ptr<RuntimeScript> PRuntimeScript;

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_SCRIPT__RUNTIMESCRIPT_H
