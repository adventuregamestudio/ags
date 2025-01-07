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
#include "script/runtimescript.h"
#include "script/cc_common.h"
#include "script/script_runtime.h"
#include "util/memory_compat.h"

namespace AGS
{
namespace Engine
{

using namespace AGS::Common;

const ScriptCommandInfo sccmd_info[CC_NUM_SCCMDS] =
{
    ScriptCommandInfo( 0                    , "NULL"              , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_ADD             , "addi"              , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_SUB             , "subi"              , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_REGTOREG        , "mov"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_WRITELIT        , "memwritelit"       , 2, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_RET             , "ret"               , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_LITTOREG        , "movl"              , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMREAD         , "memread4"          , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMWRITE        , "memwrite4"         , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MULREG          , "mul"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_DIVREG          , "div"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_ADDREG          , "add"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_SUBREG          , "sub"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_BITAND          , "and"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_BITOR           , "or"                , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_ISEQUAL         , "cmpeq"             , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_NOTEQUAL        , "cmpne"             , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_GREATER         , "gt"                , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_LESSTHAN        , "lt"                , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_GTE             , "gte"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_LTE             , "lte"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_AND             , "land"              , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_OR              , "lor"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_CALL            , "call"              , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMREADB        , "memread1"          , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMREADW        , "memread2"          , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMWRITEB       , "memwrite1"         , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMWRITEW       , "memwrite2"         , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_JZ              , "jzi"               , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_PUSHREG         , "push"              , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_POPREG          , "pop"               , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_JMP             , "jmpi"              , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_MUL             , "muli"              , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_CALLEXT         , "farcall"           , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_PUSHREAL        , "farpush"           , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_SUBREALSTACK    , "farsubsp"          , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_LINENUM         , "sourceline"        , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_CALLAS          , "callscr"           , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_THISBASE        , "thisaddr"          , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_NUMFUNCARGS     , "setfuncargs"       , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_MODREG          , "mod"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_XORREG          , "xor"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_NOTREG          , "not"               , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_SHIFTLEFT       , "shl"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_SHIFTRIGHT      , "shr"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_CALLOBJ         , "callobj"           , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_CHECKBOUNDS     , "checkbounds"       , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMWRITEPTR     , "memwrite.ptr"      , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMREADPTR      , "memread.ptr"       , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_MEMZEROPTR      , "memwrite.ptr.0"    , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_MEMINITPTR      , "meminit.ptr"       , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_LOADSPOFFS      , "load.sp.offs"      , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_CHECKNULL       , "checknull.ptr"     , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_FADD            , "faddi"             , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_FSUB            , "fsubi"             , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_FMULREG         , "fmul"              , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FDIVREG         , "fdiv"              , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FADDREG         , "fadd"              , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FSUBREG         , "fsub"              , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FGREATER        , "fgt"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FLESSTHAN       , "flt"               , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FGTE            , "fgte"              , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_FLTE            , "flte"              , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_ZEROMEMORY      , "zeromem"           , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_CREATESTRING    , "newstring"         , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_STRINGSEQUAL    , "streq"             , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_STRINGSNOTEQ    , "strne"             , 2, kScOpTwoArgsAreReg ),
    ScriptCommandInfo( SCMD_CHECKNULLREG    , "checknull"         , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_LOOPCHECKOFF    , "loopcheckoff"      , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_MEMZEROPTRND    , "memwrite.ptr.0.nd" , 0, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_JNZ             , "jnzi"              , 1, kScOpNoArgIsReg ),
    ScriptCommandInfo( SCMD_DYNAMICBOUNDS   , "dynamicbounds"     , 1, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_NEWARRAY        , "newarray"          , 3, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_NEWUSEROBJECT   , "newuserobject"     , 2, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_NEWUSEROBJECT2  , "newuserobject2"    , 3, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_NEWARRAY2       , "newarray2"         , 3, kScOpOneArgIsReg ),
    ScriptCommandInfo( SCMD_DYNAMICCAST     , "dynamiccast"       , 1, kScOpNoArgIsReg),
};

// FIXME: move to some "script helpers" module
static int DetermineScriptLine(const std::vector<intptr_t> &code, const int32_t at_pc)
{
    if (at_pc < 0)
        return 0;

    int line = -1;
    for (uint32_t pc = 0; (pc <= static_cast<uint32_t>(at_pc)) && (pc < code.size()); ++pc)
    {
        const int op = code[pc] & INSTANCE_ID_REMOVEMASK;
        if (op < 0 || op >= CC_NUM_SCCMDS) return -1;
        if (pc + sccmd_info[op].ArgCount >= code.size()) return -1;
        if (op == SCMD_LINENUM)
            line = code[pc + 1];
        pc += sccmd_info[op].ArgCount;
    }
    return line;
}

static void cc_error_fixups(const RuntimeScript *scri, const int32_t pc, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const String displbuf = String::FromFormatV(fmt, ap);
    va_end(ap);
    if (pc == INT32_MAX)
    {
        cc_error("in script %s: %s", scri->GetScriptName().GetCStr(), displbuf.GetCStr());
    }
    else
    {
        const int line = DetermineScriptLine(scri->GetCode(), pc);
        cc_error("in script %s around line %d: %s", scri->GetScriptName().GetCStr(), line, displbuf.GetCStr());
    }
}


const String RuntimeScript::_noname = "(unknown)";
const String RuntimeScript::_unknownSectionName = "(unknown section)";
RuntimeScript *RuntimeScript::_linkedScripts[RuntimeScript::MaxLinkedScripts] = { nullptr };

std::unique_ptr<JointRTTI> RuntimeScript::_jointRtti;
std::unique_ptr<RTTIHelper> RuntimeScript::_rttiHelper;

RuntimeScript::RuntimeScript()
    : _exportLookup('$', true /* allow to match symbols with more appendages */)
{
}

RuntimeScript::RuntimeScript(const String &tag)
    : _exportLookup('$', true /* allow to match symbols with more appendages */)
    , _tag(tag)
{
}

/* static */ std::unique_ptr<RuntimeScript> RuntimeScript::Create(const ccScript *script, const String &tag)
{
    if (!script)
        return nullptr;

    auto run_script = std::make_unique<RuntimeScript>(tag);
    if (!run_script->Create(script))
        return nullptr;

    // Find a linking slot for the new script
    for (int i = 0; i < MaxLinkedScripts; i++)
    {
        if (_linkedScripts[i] == nullptr)
        {
            run_script->_linkIndex = static_cast<uint8_t>(i);
            _linkedScripts[i] = run_script.get();
            break;
        }
    }

    if (run_script->_linkIndex < 0)
    {
        cc_error("Too many linked scripts");
        return nullptr;
    }

    // Join RTTI
    if (!RuntimeScript::_jointRtti)
        RuntimeScript::_jointRtti.reset(new JointRTTI());
    if (!RuntimeScript::_rttiHelper)
        RuntimeScript::_rttiHelper.reset(new RTTIHelper());
    if (run_script->GetRTTI() && !run_script->GetRTTI()->IsEmpty())
    {
        JoinRTTI(*run_script->GetRTTI(), run_script->_locidLocal2Global, run_script->_typeidLocal2Global);
    }

    return run_script;
}

/* static */ RuntimeScript *RuntimeScript::GetLinkedScript(int linkid)
{
    return _linkedScripts[linkid];
}

/* static */ void RuntimeScript::JoinRTTI(const RTTI &rtti,
    std::unordered_map<uint32_t, uint32_t> &loc_l2g,
    std::unordered_map<uint32_t, uint32_t> &type_l2g)
{
    _jointRtti->Join(rtti, loc_l2g, type_l2g);
    // TODO: optimize by updating only newly joint types
    _rttiHelper->Generate(_jointRtti->AsConstRTTI());
}

RuntimeScript::~RuntimeScript()
{
    if ((_linkIndex > 0u) && (_linkedScripts[_linkIndex] == this))
        _linkedScripts[_linkIndex] = nullptr;
}

const String &RuntimeScript::GetScriptName() const
{
    if (!_scriptname.IsEmpty())
        return _scriptname;
    // In a regular script sections contain an optional list of headers in an order
    // they were included, and the script body's own name as the last element.
    if (_sectionNames.size() > 0)
        return _sectionNames.back();
    return _noname;
}

const String &RuntimeScript::GetSectionName(int32_t offset) const
{
    size_t sect_idx = 0;
    for (; sect_idx < _sectionOffsets.size(); ++sect_idx)
    {
        if (_sectionOffsets[sect_idx] < offset)
            continue;
        break;
    }

    // if no sections in script, return unknown
    if (sect_idx == 0)
        return _unknownSectionName;

    return _sectionNames[sect_idx - 1];
}

RuntimeScriptValue RuntimeScript::GetSymbolAddress(const String &symname) const
{
    uint32_t exp_index = _exportLookup.GetIndexOfAny(symname);
    return (exp_index < UINT32_MAX) ? _resolvedExports[exp_index].Value : RuntimeScriptValue();
}

bool RuntimeScript::FindExportedFunction(const String &fn_name, int32_t &start_at, int32_t &num_args) const
{
    const uint32_t exp_index = _exportLookup.GetIndexOfAny(fn_name);
    if (exp_index == UINT32_MAX)
        return false;

    if (_resolvedExports[exp_index].Type != EXPORT_FUNCTION)
        return false; // not a function

    start_at = _resolvedExports[exp_index].Offset;
    const String &exp_name = _exports[exp_index];
    assert(exp_name.GetLength() >= fn_name.GetLength());
    if (exp_name.GetLength() <= fn_name.GetLength())
    {
        num_args = -1; // unknown, registered without args info
    }
    else
    {
        num_args = atoi(&exp_name[fn_name.GetLength() + 1]);
    }
    return true;
}

void RuntimeScript::CopyGlobalData(const std::vector<uint8_t> &data)
{
    const size_t copy_sz = std::min(data.size(), _globaldata.size());
    std::copy(data.begin(), data.begin() + copy_sz, _globaldata.begin());
}

bool RuntimeScript::Create(const ccScript *script)
{
    // Copy loaded script data over
    _scriptname = script->scriptname;
    _globaldata = script->globaldata;
    // Read code into array of intptr_t, necessary for being able to
    // store direct memory addresses at runtime
    _code.resize(script->code.size());
    for (size_t i = 0; i < script->code.size(); ++i)
        _code[i] = script->code[i];
    _fixups = script->fixups;
    _fixupTypes = script->fixuptypes;
    _fixupValues.resize(script->fixups.size());
    _codeFixups.resize(script->code.size());
    _strings = script->strings;
    for (const auto &i : script->imports)
        _imports.emplace_back(i);
    for (const auto &e : script->exports)
        _exports.emplace_back(e);
    _exportAddr = script->export_addr;
    for (const auto &sn : script->sectionNames)
        _sectionNames.emplace_back(sn);
    _sectionOffsets = script->sectionOffsets;
    // Auxiliary script data
    if (script->rtti)
    {
        _rtti = std::make_unique<RTTI>(*script->rtti);
    }
    if (script->sctoc)
    {
        _toc = std::make_unique<ScriptTOC>(*script->sctoc);
        _toc->RebindRTTI(_rtti.get());
    }

    // Generate runtime data
    if (!CreateGlobalVars())
        return false;
    if (!CreateRuntimeCodeFixups())
        return false;
    if (!ResolveExports())
        return false;

    // Generate lookup tables for script TOC (if available)
    if (_toc)
    {
        const auto &glvars = _toc->GetGlobalVariables();
        for (size_t i = 0; i < glvars.size(); ++i)
        {
            _globalVarLookup.insert(
                std::make_pair(String(glvars[i].name), static_cast<uint32_t>(i)));
        }
    }

    return true;
}

void RuntimeScript::RegisterExports(SystemImports &simp)
{
    // Do not register exports if SCOPT_AUTOIMPORT is not set
    if (ccGetOption(SCOPT_AUTOIMPORT) == 0)
        return;

    for (size_t i = 0; i < _exports.size(); i++)
    {
        String name = _exports[i];
        name.Replace('$', '^'); // replace exported name separator with imported name separator
        ccAddExternalScriptSymbol(name, _resolvedExports[i].Value, this);
    }
}

void RuntimeScript::UnRegisterExports(SystemImports &simp)
{
    simp.RemoveScriptExports(this);
}

bool RuntimeScript::ResolveImports(const SystemImports &simp)
{
    // Script keeps the information of what imports are used as an array of names.
    // When an import is referenced in the code, it's addressed by its index in this
    // array. Different scripts have differing arrays of imports; indexes
    // into 'imports[]' are NOT unique and relative to the respective script only.
    // To allow real-time import use, the sequence of imports in 'imports[]'
    // and 'resolved_imports[]' should not be modified.

    const size_t numimports = _imports.size();
    if (numimports == 0)
    {
        // [PGB] AFAICS there's nothing wrong with not having any imports, and
        // it doesn't lead to trouble. However, if it turns out that we do need
        // to return 'false' here, we should also report why with a 'Debug::Printf()' call.
        return true;
    }

    auto &resolved_imports = _resolvedImports;
    resolved_imports.resize(numimports);
    size_t errors = 0, last_err_idx = 0;
    for (size_t import_idx = 0; import_idx < _imports.size(); ++import_idx)
    {
        if (_imports[import_idx].IsEmpty())
        {
            resolved_imports[import_idx] = UINT32_MAX;
            continue;
        }

        resolved_imports[import_idx] = simp.GetIndexOfAny(_imports[import_idx]);
        if (resolved_imports[import_idx] == UINT32_MAX)
        {
            Debug::Printf(kDbgMsg_Error, "unresolved import '%s' in '%s'", _imports[import_idx].GetCStr(), GetScriptName().GetCStr());
            errors++;
            last_err_idx = import_idx;
        }
    }

    if (errors > 0)
    {
        cc_error("in %s: %d unresolved imports (last: %s)",
            GetScriptName().GetCStr(),
            errors,
            _imports[last_err_idx].GetCStr());
        return false;
    }

    ResolveImportFixups();
    return true;
}

bool RuntimeScript::ResolveImportFixups()
{
    const auto &resolved_imports = _resolvedImports;
    for (size_t fixup_idx = 0; fixup_idx < _fixups.size(); ++fixup_idx)
    {
        if (_fixupTypes[fixup_idx] != FIXUP_IMPORT)
            continue;

        uint32_t const fixup = _fixups[fixup_idx];
        int32_t const fixupValue = _fixupValues[fixup_idx];
        uint32_t const import_index = resolved_imports[fixupValue];
        ScriptImport const *import = simp.GetByIndex(import_index);
        if (!import)
        {
            cc_error("cannot resolve import, key = %d", import_index);
            cc_error_fixups(this, fixup, "cannot resolve import (bytecode pos %d, key %d)", fixup, import_index);
            return false;
        }
        _code[fixup] = import_index;
        // If the call is to another script function, then CALLEXT must be replaced with CALLAS
        if ((import->ScriptID >= 0) && ((_code[fixup + 1] & INSTANCE_ID_REMOVEMASK) == SCMD_CALLEXT))
            _code[fixup + 1] = SCMD_CALLAS | (import->ScriptID << INSTANCE_ID_SHIFT);
    }
    return true;
}

// TODO: it is possible to deduce global var's size at start with
// certain accuracy after all global vars are registered. Each
// global var's size would be limited by closest next var's ScAddress
// and globaldatasize.
bool RuntimeScript::CreateGlobalVars()
{
    ScriptVariable glvar;
    uint8_t *globaldata = _globaldata.data();

    // Step One: deduce global variables from fixups
    for (size_t i = 0; i < _fixupTypes.size(); ++i)
    {
        switch (_fixupTypes[i])
        {
        case FIXUP_GLOBALDATA:
            // GLOBALDATA fixup takes relative address of global data element from code array;
            // this is the address of actual data
            glvar.ScAddress = (int32_t)_code[_fixups[i]];
            glvar.RValue.SetData(globaldata + glvar.ScAddress, 0);
            break;
        case FIXUP_DATADATA:
            {
            // DATADATA fixup takes relative address of global data element from fixups array;
            // this is the address of element, which stores address of actual data
            glvar.ScAddress = _fixups[i];
            const int32_t data_addr = BBOp::Int32FromLE(*(int32_t*)&globaldata[glvar.ScAddress]);
            if (glvar.ScAddress - data_addr != 200 /* size of old AGS string */)
            {
                // CHECKME: probably replace with mere warning in the log?
                cc_error("unexpected old-style string's alignment");
                return false;
            }
            // TODO: register this explicitly as a string instead (can do this later)
            glvar.RValue.SetScriptObject(globaldata + data_addr, &GlobalStaticManager);
            }
            break;
        default:
            // other fixups are of no use here
            continue;
        }

        AddGlobalVar(glvar);
    }

    // Step Two: deduce global variables from exports
    for (size_t i = 0; i < _exports.size(); ++i)
    {
        const int32_t etype = (_exportAddr[i] >> 24L) & 0x000ff;
        const int32_t eaddr = (_exportAddr[i] & 0x00ffffff);
        if (etype == EXPORT_DATA)
        {
            // NOTE: old-style strings could not be exported in AGS,
            // no need to worry about these here
            glvar.ScAddress = eaddr;
            glvar.RValue.SetData(globaldata + glvar.ScAddress, 0);
            AddGlobalVar(glvar);
        }
    }

    return true;
}

bool RuntimeScript::AddGlobalVar(const ScriptVariable &glvar)
{
    // NOTE:
    // We suppress the error here, because unfortunately at least one existing
    // game ("Metal Dead", built with AGS 3.21.1115) fails to pass this check.
    // It has been found that this may be caused by a global variable of zero
    // size (an instance of empty struct) placed in the end of the script.
    // TODO: invent some workaround?
    // TODO: enable the error back in AGS 4, as this is not a normal behavior.
    const int32_t globaldatasize = static_cast<int32_t>(_globaldata.size());
    if (glvar.ScAddress < 0 || glvar.ScAddress >= globaldatasize)
    {
        /* return false; */
        Debug::Printf(kDbgMsg_Warn, "WARNING: global variable refers to data beyond allocated buffer (%d, %d)", glvar.ScAddress, globaldatasize);
    }
    _globalvars.insert(std::make_pair(glvar.ScAddress, glvar));
    return true;
}

ScriptVariable *RuntimeScript::FindGlobalVar(const int32_t var_addr)
{
    // NOTE: see comment for AddGlobalVar()
    const int32_t globaldatasize = static_cast<int32_t>(_globaldata.size());
    if (var_addr < 0 || var_addr >= globaldatasize)
    {
        /*
        return nullptr;
        */
        Debug::Printf(kDbgMsg_Warn, "WARNING: looking up for global variable beyond allocated buffer (%d, %d)", var_addr, globaldatasize);
    }
    const auto it = _globalvars.find(var_addr);
    return it != _globalvars.end() ? &it->second : nullptr;
}

bool RuntimeScript::CreateRuntimeCodeFixups()
{
    for (size_t i = 0; i < _fixups.size(); ++i)
    {
        if (_fixupTypes[i] == FIXUP_NOFIXUP || _fixupTypes[i] == FIXUP_DATADATA)
        {
            continue;
        }

        const int32_t fixup = _fixups[i];
        if (fixup < 0 || static_cast<size_t>(fixup) >= _code.size())
        {
            cc_error_fixups(this, INT32_MAX, "bad fixup at %d (fixup type %d, bytecode pos %d, bytecode range is 0..%d)",
                i,  _fixupTypes[i], fixup, static_cast<int32_t>(_code.size()));
            return false;
        }

        const int32_t fixup_value = static_cast<int32_t>(_code[fixup]);
        const uint8_t fixup_type = _fixupTypes[i];
        _fixupValues[i] = fixup_value; // save original code value for this fixup
        _codeFixups[fixup] = fixup_type; // save fixup type for the code cell

        switch (fixup_type)
        {
        case FIXUP_GLOBALDATA:
            {
                ScriptVariable *gl_var = FindGlobalVar(fixup_value);
                if (!gl_var)
                {
                    cc_error_fixups(this, fixup, "cannot resolve global variable (bytecode pos %d, key %d)", fixup, fixup_value);
                    return false;
                }
                _code[fixup] = (intptr_t)gl_var;
            }
            break;
        case FIXUP_FUNCTION:
        case FIXUP_STRING:
        case FIXUP_STACK:
        case FIXUP_IMPORT:
            break; // do nothing yet
        default:
            cc_error_fixups(this, INT32_MAX, "unknown fixup type: %d (fixup num %d)", fixup_type, i);
            return false;
        }
    }
    return true;
}

bool RuntimeScript::ResolveExports()
{
    // Find the real addresses of the exports
    for (size_t i = 0; i < _exports.size(); i++)
    {
        const int32_t etype = (_exportAddr[i] >> 24L) & 0x000ff;
        const int32_t eaddr = (_exportAddr[i] & 0x00ffffff);
        ResolvedExport res_exp;
        res_exp.Type = etype;
        res_exp.Offset = eaddr;
        if (etype == EXPORT_FUNCTION)
        {
            // NOTE: unfortunately, there seems to be no way to know if
            // that's an extender function that expects object pointer
            res_exp.Value.SetCodePtr(_code.data() + static_cast<uintptr_t>(eaddr));
        }
        else if (etype == EXPORT_DATA)
        {
            ScriptVariable *gl_var = FindGlobalVar(eaddr);
            if (gl_var)
            {
                res_exp.Value.SetGlobalVar(&gl_var->RValue);
            }
            else
            {
                cc_error("cannot resolve global variable, key = %d", eaddr);
                return false;
            }
        }
        else
        {
            cc_error("internal export fixup error");
            return false;
        }

        _resolvedExports.push_back(res_exp);
        _exportLookup.Add(_exports[i], i);
    }

    return true;
}

} // namespace Engine
} // namespace AGS
