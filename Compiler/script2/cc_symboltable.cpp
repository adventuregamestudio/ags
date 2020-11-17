
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_symboltable.h"
#include "script/script_common.h"       // macro definitions
#include "script/cc_error.h"            // error processing

AGS::SymbolTableEntry::~SymbolTableEntry()
{
    // (note that null pointers may be safely 'delete'd, in contrast to 'free'd)
    delete this->ConstantD;
    this->ConstantD = nullptr;
    delete this->ComponentD;
    this->ComponentD = nullptr;
    delete this->DelimeterD;
    this->DelimeterD = nullptr;
    delete this->FunctionD;
    this->FunctionD = nullptr;
    delete this->LiteralD;
    this->LiteralD = nullptr;
    delete this->OperatorD;
    this->OperatorD = nullptr;
    delete this->VariableD;
    this->VariableD = nullptr;
    delete this->VartypeD;
    this->VartypeD = nullptr;
}

AGS::SymbolTableEntry &AGS::SymbolTableEntry::operator=(const SymbolTableEntry &orig)
{
    this->Name = orig.Name;
    this->Declared = orig.Declared;
    this->Scope = orig.Scope;
    this->Accessed = orig.Accessed;

    // Deep copy semantics.
    this->ConstantD = (orig.ConstantD) ? new SymbolTableEntry::ConstantDesc{ *(orig.ConstantD) } : nullptr;
    this->DelimeterD = (orig.DelimeterD) ? new SymbolTableEntry::DelimeterDesc{ *(orig.DelimeterD) } : nullptr;
    this->FunctionD = (orig.FunctionD) ? new SymbolTableEntry::FunctionDesc{ *(orig.FunctionD) } : nullptr;
    this->LiteralD = (orig.LiteralD) ? new SymbolTableEntry::LiteralDesc{ *(orig.LiteralD) } : nullptr;
    this->OperatorD = (orig.OperatorD) ? new SymbolTableEntry::OperatorDesc{ *(orig.OperatorD) } : nullptr;
    this->ComponentD = (orig.ComponentD) ? new SymbolTableEntry::ComponentDesc{ *(orig.ComponentD) } : nullptr;
    this->VariableD = (orig.VariableD) ? new SymbolTableEntry::VariableDesc{ *(orig.VariableD) } : nullptr;
    this->VartypeD = (orig.VartypeD) ? new SymbolTableEntry::VartypeDesc{ *(orig.VartypeD) } : nullptr;

    return *this;
}

std::map<AGS::TypeQualifier, AGS::Symbol> const &AGS::TypeQualifierSet::TQToSymbolMap() const
{
    // "static" so that we get a singleton that will only be initialized once, at first use
    static std::map<TypeQualifier, Symbol> const tq2sym =
    {
        { TQ::kAttribute,       kKW_Attribute, },
        { TQ::kAutoptr,         kKW_Autoptr, },
        { TQ::kBuiltin,         kKW_Builtin, },
        { TQ::kConst,           kKW_Const, },
        { TQ::kImport,          kKW_ImportStd, },
        { TQ::kManaged,         kKW_Managed,  },
        { TQ::kProtected,       kKW_Protected,  },
        { TQ::kReadonly,        kKW_Readonly, },
        { TQ::kStatic,          kKW_Static, },
        { TQ::kStringstruct,    kKW_Internalstring, },
        { TQ::kWriteprotected,  kKW_Writeprotected, },
    };

    return tq2sym;
}

void AGS::SymbolTableEntry::Clear()
{
    // Leave the name so that symbols in the main phase equate to symbols in the pre phase.
    Declared = SymbolTableConstant::kNoSrcLocation;
    Scope = 0u;
    // Don't clear Accessed so when a function is first used and then declared, this doesn't clobber the use.

    delete ConstantD;
    ConstantD = nullptr;
    delete DelimeterD;
    DelimeterD = nullptr;
    delete FunctionD;
    FunctionD = nullptr;
    delete LiteralD;
    LiteralD = nullptr;
    delete OperatorD;
    OperatorD = nullptr;
    delete ComponentD;
    ComponentD = nullptr;
    delete VariableD;
    VariableD = nullptr;
    delete VartypeD;
    VartypeD = nullptr;
}

AGS::SymbolTableEntry::SymbolTableEntry(SymbolTableEntry const &orig)
    : Name(orig.Name)
    , Declared(orig.Declared)
    , Scope(orig.Scope)
    , Accessed(orig.Accessed)
{
    // Deep copy semantics.
    this->ConstantD = (orig.ConstantD) ? new SymbolTableEntry::ConstantDesc{ *(orig.ConstantD) } : nullptr;
    this->DelimeterD = (orig.DelimeterD) ? new SymbolTableEntry::DelimeterDesc{ *(orig.DelimeterD) } : nullptr;
    this->FunctionD = (orig.FunctionD) ? new SymbolTableEntry::FunctionDesc{ *(orig.FunctionD) } : nullptr;
    this->LiteralD = (orig.LiteralD) ? new SymbolTableEntry::LiteralDesc{ *(orig.LiteralD) } : nullptr;
    this->OperatorD = (orig.OperatorD) ? new SymbolTableEntry::OperatorDesc{ *(orig.OperatorD) } : nullptr;
    this->ComponentD = (orig.ComponentD) ? new SymbolTableEntry::ComponentDesc{ *(orig.ComponentD) } : nullptr;
    this->VariableD = (orig.VariableD) ? new SymbolTableEntry::VariableDesc{ *(orig.VariableD) } : nullptr;
    this->VartypeD = (orig.VartypeD) ? new SymbolTableEntry::VartypeDesc{ *(orig.VartypeD) } : nullptr;
}

AGS::SymbolTable::SymbolTable()
    : _stringStructSym (kKW_NoSymbol)
    , _stringStructPtrSym(kKW_NoSymbol)
{
   _findCache.clear();
    _vartypesCache.clear();

    entries.clear();
    entries.reserve(300);
    entries.resize(kKW_LastPredefined + 1);

    AddNoSymbol(kKW_NoSymbol, "__no_symbol__");

    // Primitive types
    AddVartype(kKW_Char, "char", SIZE_OF_CHAR, true);
    AddVartype(kKW_Float, "float", SIZE_OF_FLOAT);
    AddVartype(kKW_Int, "int", SIZE_OF_INT, true);
    AddVartype(kKW_Long, "long", SIZE_OF_LONG, true);
    AddVartype(kKW_Short, "short", SIZE_OF_SHORT, true);
    AddVartype(kKW_String, "string", STRINGBUFFER_LENGTH);
    AddVartype(kKW_Void, "void", 0u);

    // Delimeters
    AddDelimeter(kKW_CloseBracket, "]", false, kKW_OpenBracket, true);
    AddDelimeter(kKW_OpenBracket, "[", true, kKW_CloseBracket, true);
    AddDelimeter(kKW_CloseBrace, "}", false, kKW_OpenBrace, false);    
    AddDelimeter(kKW_OpenBrace, "{", true, kKW_CloseBrace, false);    
    AddDelimeter(kKW_CloseParenthesis, ")", false, kKW_OpenParenthesis, true);
    AddDelimeter(kKW_OpenParenthesis, "(", true, kKW_CloseParenthesis, true);

    // Operators
    AddOperator(kKW_And, "&&", SCMD_AND, 118, SymbolTable::kNoPrio);
    AddOperator(kKW_BitAnd, "&", SCMD_BITAND, 109, SymbolTable::kNoPrio);
    AddOperator(kKW_BitNeg, "~", SCMD_NOTREG, SymbolTable::kNoPrio, 101); // bitwise NOT
    AddOperator(kKW_BitOr, "|", SCMD_BITOR, 110, SymbolTable::kNoPrio);
    AddOperator(kKW_BitXor, "^", SCMD_XORREG, 110, SymbolTable::kNoPrio);
    AddOperator(kKW_Divide, "/", SCMD_DIVREG, 103, SymbolTable::kNoPrio);
    AddOperator(kKW_Equal, "==", SCMD_ISEQUAL, 112, SymbolTable::kNoPrio);
    AddOperator(kKW_Greater, ">", SCMD_GREATER, 112, SymbolTable::kNoPrio);
    AddOperator(kKW_GreaterEqual, ">=", SCMD_GTE, 112, SymbolTable::kNoPrio);
    AddOperator(kKW_Less, "<", SCMD_LESSTHAN, 112, SymbolTable::kNoPrio);
    AddOperator(kKW_LessEqual, "<=", SCMD_LTE, 112, SymbolTable::kNoPrio);
    AddOperator(kKW_Minus, "-", SCMD_SUBREG, 105, 101);
    AddOperator(kKW_Modulo, "%", SCMD_MODREG, 103, SymbolTable::kNoPrio);
    AddOperator(kKW_Multiply, "*", SCMD_MULREG, 103, SymbolTable::kNoPrio);
    AddOperator(kKW_Not, "!", SCMD_NOTREG, SymbolTable::kNoPrio, 101); // boolean NOT
    AddOperator(kKW_New, "new", SCMD_NEWUSEROBJECT, SymbolTable::kNoPrio, 101);
    AddOperator(kKW_NotEqual, "!=", SCMD_NOTEQUAL, 112, SymbolTable::kNoPrio);
    AddOperator(kKW_Or, "||", SCMD_OR, 119, SymbolTable::kNoPrio);
    AddOperator(kKW_Plus, "+", SCMD_ADDREG, 105, 101);
    AddOperator(kKW_ShiftLeft, "<<", SCMD_SHIFTLEFT, 107, SymbolTable::kNoPrio);
    AddOperator(kKW_ShiftRight, ">>", SCMD_SHIFTRIGHT, 107, SymbolTable::kNoPrio);
    AddOperator(kKW_Tern, "?", 0, 120, SymbolTable::kNoPrio);    // note, operator and keyword

    // Assignments
    AddAssign(kKW_Assign, "=", 120);

    AddAssignMod(kKW_AssignBitAnd, "&=", SCMD_BITAND, 120);
    AddAssignMod(kKW_AssignBitOr, "|=", SCMD_BITOR, 120);
    AddAssignMod(kKW_AssignBitXor, "^=", SCMD_XORREG, 120);
    AddAssignMod(kKW_AssignDivide, "/=", SCMD_DIVREG, 120);
    AddAssignMod(kKW_AssignMinus, "-=", SCMD_SUBREG, 120);
    AddAssignMod(kKW_AssignMultiply, "*=", SCMD_MULREG, 120);
    AddAssignMod(kKW_AssignPlus, "+=", SCMD_ADDREG, 120);
    AddAssignMod(kKW_AssignShiftLeft, "<<=", SCMD_SHIFTLEFT, 120);
    AddAssignMod(kKW_AssignShiftRight, ">>=",SCMD_SHIFTRIGHT, 120);

    // Modifiers
    AddModifier(kKW_Increment, "++", SCMD_ADD, 101, 101);
    AddModifier(kKW_Decrement, "--", SCMD_SUB, 101, 101);

    // other keywords and symbols
    AddKeyword(kKW_Dot, ".");
    AddKeyword(kKW_This, "this");
    MakeEntryVariable(kKW_This);
    AddKeyword(kKW_Attribute, "attribute");
    AddKeyword(kKW_Autoptr, "autoptr");
    AddKeyword(kKW_Break, "break");
    AddKeyword(kKW_Builtin, "builtin");
    AddKeyword(kKW_Case, "case");
    AddKeyword(kKW_Colon, ":");
    AddKeyword(kKW_Comma, ",");
    AddKeyword(kKW_Const, "const");
    AddKeyword(kKW_Continue, "continue");
    AddKeyword(kKW_Default, "default");
    AddKeyword(kKW_Do, "do");
    AddKeyword(kKW_Else, "else");
    AddKeyword(kKW_Enum, "enum");
    AddKeyword(kKW_Export, "export");
    AddKeyword(kKW_Extends, "extends");
    AddKeyword(kKW_For, "for");
    AddKeyword(kKW_If, "if");
    AddKeyword(kKW_ImportStd, "import");
    AddKeyword(kKW_ImportTry, "_tryimport");
    AddKeyword(kKW_Internalstring, "internalstring");
    AddKeyword(kKW_Managed, "managed");
    AddKeyword(kKW_Noloopcheck, "noloopcheck");
    AddKeyword(kKW_Null, "null");
    MakeEntryLiteral(kKW_Null);
    entries[kKW_Null].LiteralD->Vartype = kKW_NoSymbol;
    entries[kKW_Null].LiteralD->Value = 0u;
    AddKeyword(kKW_Protected, "protected");
    AddKeyword(kKW_Readonly, "readonly");
    AddKeyword(kKW_Return, "return");
    AddKeyword(kKW_ScopeRes, "::");
    AddKeyword(kKW_Semicolon, ";");
    AddKeyword(kKW_Static, "static");
    AddKeyword(kKW_Struct, "struct");
    AddKeyword(kKW_Switch, "switch");
    AddKeyword(kKW_DotDotDot, "...");
    AddKeyword(kKW_While, "while");
    AddKeyword(kKW_Writeprotected, "writeprotected");

    // Add some additional symbols that the compiler or scanner will need
    {
        Symbol const zero_sym = Add("0");
        MakeEntryLiteral(zero_sym);
        entries[zero_sym].LiteralD->Value = 0u;
        entries[zero_sym].LiteralD->Vartype = kKW_Int;
    }
    _lastAllocated = VartypeWith(VTT::kConst, kKW_String);
}

bool AGS::SymbolTable::IsVTT(Symbol s, VartypeType vtt) const
{
    if (IsVariable(s))
        s = entries.at(s).VariableD->Vartype;
    if (!IsVartype(s))
        return false;

    // ignore 'const', must be outermost if present
    if (VTT::kConst != vtt && VTT::kConst == entries.at(s).VartypeD->Type)
        s = entries.at(s).VartypeD->BaseVartype;
    return vtt == entries.at(s).VartypeD->Type;
}

bool AGS::SymbolTable::IsVTF(Symbol s, VartypeFlag flag) const
{
    if (IsVariable(s))
        s = entries.at(s).VariableD->Vartype;
    if (!IsVartype(s))
        return false;

    // Get to the innermost symbol; read that symbol's flags
    while (VTT::kAtomic != entries.at(s).VartypeD->Type)
        s = entries.at(s).VartypeD->BaseVartype;
    return entries.at(s).VartypeD->Flags[flag];
}

void AGS::SymbolTable::SetStringStructSym(Symbol const s)
{
    _stringStructSym = s;
    _stringStructPtrSym = VartypeWith(VTT::kDynpointer, s);
}

bool AGS::SymbolTable::IsInUse(Symbol s) const
{
    if (s <= kKW_LastPredefined)
        return true;

    // Don't check for ComponentD. This record is subsidiary
    // and only makes sense if one of the other records is set.
    auto const &entry = entries.at(s);
    return
        entry.ConstantD ||
        entry.FunctionD ||
        entry.LiteralD ||
        entry.VariableD ||
        entry.VartypeD;
}

bool AGS::SymbolTable::IsIdentifier(Symbol s) const
{
    if (s <= kKW_LastPredefined || s > static_cast<decltype(s)>(entries.size()))
        return false;
    std::string const name = GetName(s);
    if (0u == name.size())
        return false;
    if ('0' <= name[0] && name[0] <= '9')
        return false;
    for (size_t idx = 0; idx < name.size(); ++idx)
    {
        char const &ch = name[idx];
        if ('0' <= ch && ch <= '9') continue;
        if ('A' <= ch && ch <= 'Z') continue;
        if ('a' <= ch && ch <= 'z') continue;
        if ('_' == ch) continue;
        return false;
    }
    return true;
}

bool AGS::SymbolTable::CanBePartOfAnExpression(Symbol s)
{
    // Note: Whatever is within delimeters will be completely skipped automatically,
    // it doesn't need to be considered here.
    return
        IsConstant(s) ||
        (IsDelimeter(s) && entries.at(s).DelimeterD->CanBePartOfAnExpression) ||
        IsFunction(s) ||
        IsLiteral(s) ||
        (IsOperator(s) && entries.at(s).OperatorD->CanBePartOfAnExpression) ||
        IsVariable(s);
}

bool AGS::SymbolTable::IsAnyIntegerVartype(Symbol s) const
{
    if (IsVariable(s))
        s = entries.at(s).VariableD->Vartype;
    if (!IsVartype(s) || !IsAtomicVartype(s))
        return false;
    if (kKW_NoSymbol == entries.at(s).VartypeD->BaseVartype)
        return entries.at(s).VartypeD->Flags[VTF::kIntegerVartype];
    return IsAnyIntegerVartype(entries.at(s).VartypeD->BaseVartype);
}

size_t AGS::SymbolTable::GetSize(Symbol s) const
{
    if (IsVariable(s))
        s = entries.at(s).VariableD->Vartype;
    if (!IsVartype(s))
        return (0u);
    return entries.at(s).VartypeD->Size;    
}

bool AGS::SymbolTable::IsPrimitiveVartype(Symbol s) const
{
    if (IsVariable(s))
        s = entries.at(s).VariableD->Vartype;
    if (!IsVartype(s))
        return false;
    if (VTT::kConst == entries.at(s).VartypeD->Type)
        s = entries.at(s).VartypeD->BaseVartype;
    if (!IsPredefined(s))
        return false;
    if (VTT::kAtomic != entries.at(s).VartypeD->Type)
        return false;
    return kKW_NoSymbol == entries.at(s).VartypeD->BaseVartype;
}

size_t AGS::SymbolTable::NumArrayElements(Symbol s) const
{
    if (IsVariable(s))
        s = entries.at(s).VariableD->Vartype;
    if (!IsVartype(s))
        return 0u;

    SymbolTableEntry::VartypeDesc const *vdesc = entries.at(s).VartypeD;
    size_t const dims_size = vdesc->Dims.size();
    if (0u == dims_size)
        return 0u;

    size_t num = 1;
    for (size_t dims_idx = 0; dims_idx < dims_size; ++dims_idx)
        num *= vdesc->Dims[dims_idx];
    return num;
}

bool AGS::SymbolTable::IsManagedVartype(Symbol s) const
{
    if (IsVariable(s))
        s = entries.at(s).VariableD->Vartype;
    if (!IsVartype(s))
        return false;
    
    while (VTT::kAtomic != entries.at(s).VartypeD->Type)
        s = entries.at(s).VartypeD->BaseVartype;
       
    return entries.at(s).VartypeD->Flags[VTF::kManaged];
}

std::string const AGS::SymbolTable::GetName(AGS::Symbol symbl) const
{
    if (symbl <= 0)
        return std::string("(end of input)");
    if (static_cast<size_t>(symbl) >= entries.size())
        return std::string("(invalid symbol)");
    return entries[symbl].Name;
}

AGS::Vartype AGS::SymbolTable::VartypeWithArray(std::vector<size_t> const &dims, AGS::Vartype vartype)
{
    // Can't have classic arrays of classic arrays
    if (IsVTT(vartype, VTT::kArray))
        return vartype;

    std::string conv_name = entries[vartype].Name + "[";
    size_t const last_idx = dims.size() - 1;
    size_t num_elements = 1;
    for (size_t dims_idx = 0; dims_idx <= last_idx; ++dims_idx)
    {
        num_elements *= dims[dims_idx];
        conv_name += std::to_string(dims[dims_idx]);
        conv_name += (dims_idx == last_idx) ? "]" : ", ";
    }
    Vartype const array_vartype = FindOrAdd(conv_name);
    if (IsVartype(array_vartype))
        return array_vartype;

    entries[array_vartype].VartypeD = new SymbolTableEntry::VartypeDesc;
    entries[array_vartype].VartypeD->Type = VTT::kArray;
    entries[array_vartype].VartypeD->BaseVartype = vartype;
    entries[array_vartype].VartypeD->Size = num_elements * GetSize(vartype);
    entries[array_vartype].VartypeD->Dims = dims;
    return array_vartype;
}

AGS::Vartype AGS::SymbolTable::VartypeWith(VartypeType vtt, AGS::Vartype vartype)
{
    // Return cached result if existent 
    std::pair<Vartype, VartypeType> const arg = { vartype, vtt };
    Vartype &valref(_vartypesCache[arg]);
    if (valref)
        return valref;

    if (IsVTT(vartype, vtt))
        return (valref = vartype); // Nothing to be done

    std::string pre = "";
    std::string post = "";
    switch (vtt)
    {
    default: pre = "QUAL" + std::to_string(static_cast<int>(vtt)) + " "; break;
    case VTT::kConst: pre = "const "; break;
        // Note: Even if we have an autoptr and suppress the '*', we still need to add _something_ to the name.
        // The name for the pointered type must be different from the name of the unpointered type.
        // (If this turns out to be too ugly, then we need two fields for vartypes:
        // one field that is output to the user, another field that is guaranteed to have different values
        // for different vartypes.)
    case VTT::kDynpointer: post = (IsVartype(vartype) && entries[vartype].VartypeD->Flags[VTF::kAutoptr]) ? " " : " *"; break;
    case VTT::kDynarray: post = "[]"; break;
    }
    std::string const conv_name = (pre + entries[vartype].Name) + post;
    valref = FindOrAdd(conv_name);
    if (IsVartype(valref))
        return valref;

    SymbolTableEntry &entry = entries[valref];
    entry.VartypeD = new SymbolTableEntry::VartypeDesc;
    entry.VartypeD->Type = vtt;
    entry.VartypeD->BaseVartype = vartype;
    entry.VartypeD->Size = (VTT::kConst == vtt) ? GetSize(vartype) : SIZE_OF_DYNPOINTER;
    return valref;
}

AGS::Vartype AGS::SymbolTable::VartypeWithout(VartypeType const vtt, AGS::Vartype vartype) const
{
    if (IsVartype(vartype) && vtt == entries[vartype].VartypeD->Type)
        vartype = entries[vartype].VartypeD->BaseVartype;
    return vartype;
}

void AGS::SymbolTable::GetComponentsOfStruct(Symbol strct, std::vector<Symbol> &compo_list) const
{
    if (!IsVartype(strct) || !entries.at(strct).VartypeD->Flags[VTF::kStruct])
        return;

    // Collect the ancestors of 'struct', i.e., those structs that 'struct' extends
    std::vector<Symbol> ancestors = {};
    while(kKW_NoSymbol != strct)
    {
        ancestors.push_back(strct);
        strct = entries.at(strct).VartypeD->Parent;
    }
   
    for (auto ancestors_it = ancestors.crbegin(); ancestors_it != ancestors.crend(); ancestors_it++)
    {
        auto const &components = entries.at(*ancestors_it).VartypeD->Components;
        for (auto components_it = components.cbegin(); components_it != components.cend(); components_it++)
            compo_list.push_back(components_it->second);
    }
}

AGS::Symbol AGS::SymbolTable::FindStructComponent(Symbol strct, Symbol const component, Symbol const ancestor) const
{
    // Find the ancestor
    while (strct != ancestor && strct != kKW_NoSymbol)
        strct = entries.at(strct).VartypeD->Parent;

    // Start looking for the components
    while (strct)
    {
        auto const &components = entries.at(strct).VartypeD->Components;
        for (auto components_it = components.begin(); components_it != components.end(); components_it++)
            if (component == components_it->first)
                return components_it->second;
    }
    return kKW_NoSymbol;
}

AGS::ScopeType AGS::SymbolTable::GetScopeType(Symbol s) const
{
    if (0 < entries.at(s).Scope)
        return ScT::kLocal;
    if (entries.at(s).VariableD->TypeQualifiers[TQ::kImport])
        return ScT::kImport;
    return ScT::kGlobal;
}

bool AGS::SymbolTable::IsAnyStringVartype(Symbol s) const
{
    if (IsVariable(s))
        s = entries[s].VariableD->Vartype;

    if (!IsVartype(s))
        return false;

    // Oldstrings and String * are strings
    Vartype const s_without_const = VartypeWithout(VTT::kConst, s);

    return
        kKW_String == s_without_const ||
        GetStringStructSym() == VartypeWithout(VTT::kDynpointer, s_without_const);
}

bool AGS::SymbolTable::IsOldstring(Symbol s) const
{
    if (!IsInBounds(s))
        return false;

    // Convert a var to its vartype
    if (IsVariable(s))
        s = entries[s].VariableD->Vartype;

    if (!IsVartype(s))
            return false;

    Vartype const s_without_const =
        VartypeWithout(VTT::kConst, s);
    // string and const string are oldstrings
    if (kKW_String == s_without_const)
        return true;

    // const char[..] and char[..] are considered oldstrings, too
    return (IsArrayVartype(s) && kKW_Char == VartypeWithout(VTT::kArray, s_without_const));
}

AGS::Symbol AGS::SymbolTable::Add(std::string const &name)
{
    // Note: A very significant portion of computing is spent in this function.
    if (0 != _findCache.count(name))
        return kKW_NoSymbol;

    // Extend the entries in chunks instead of one-by-one: Experiments show that this saves time
    if (entries.size() == entries.capacity())
    {
        size_t const new_size1 = entries.capacity() * 2;
        size_t const new_size2 = entries.capacity() + 1000;
        entries.reserve((new_size1 < new_size2) ? new_size1 : new_size2);
    }
    int const idx_of_new_entry = entries.size();
    entries.emplace_back();
    entries.at(idx_of_new_entry).Name = name;
    _findCache[name] = idx_of_new_entry;
    return idx_of_new_entry;
}

AGS::Symbol AGS::SymbolTable::AddNoSymbol(Predefined kw, std::string const &name)
{
    SymbolTableEntry &entry = entries[kw];
    entry.Name = name;

    _findCache[name] = kw;
    return kw;
}

AGS::Symbol AGS::SymbolTable::AddAssign(Predefined kw, std::string const &name, size_t prio)
{
    SymbolTableEntry &entry = entries.at(kw);
    entry.Name = name;
    entry.OperatorD = new SymbolTableEntry::OperatorDesc;
    entry.OperatorD->BinaryPrio = prio;
    entry.OperatorD->UnaryPrio = SymbolTable::kNoPrio;
    entry.OperatorD->Opcode = 0;

    _findCache[name] = kw;
    return kw;
}

AGS::Symbol AGS::SymbolTable::AddAssignMod(Predefined kw, std::string const &name, CodeCell opcode, size_t prio)
{
    SymbolTableEntry &entry = entries.at(kw);
    entry.Name = name;
    entry.OperatorD = new SymbolTableEntry::OperatorDesc;
    entry.OperatorD->BinaryPrio = prio;
    entry.OperatorD->UnaryPrio = SymbolTable::kNoPrio;
    entry.OperatorD->Opcode = opcode;

    _findCache[name] = kw;
    return kw;
}

AGS::Symbol AGS::SymbolTable::AddDelimeter(Predefined kw, std::string const &name, bool is_opener, Symbol partner, bool can_be_expression)
{
    SymbolTableEntry &entry = entries.at(kw);
    entry.Name = name;
    entry.DelimeterD = new SymbolTableEntry::DelimeterDesc;
    entry.DelimeterD->Opening = is_opener;
    entry.DelimeterD->Partner = partner;
    entry.DelimeterD->CanBePartOfAnExpression = can_be_expression;

    _findCache[name] = kw;
    return kw;
}

AGS::Symbol AGS::SymbolTable::AddKeyword(Predefined kw, std::string const &name)
{
    SymbolTableEntry &entry = entries.at(kw);
    entry.Name = name;
    
    _findCache[name] = kw;
    return kw;
}

AGS::Symbol AGS::SymbolTable::AddModifier(Predefined kw, std::string const &name, CodeCell opcode, size_t prefix_prio, size_t postfix_prio)
{
    
    SymbolTableEntry &entry = entries.at(kw);
    entry.Name = name;
    entry.OperatorD = new SymbolTableEntry::OperatorDesc;
    entry.OperatorD->BinaryPrio = prefix_prio;
    entry.OperatorD->UnaryPrio = postfix_prio;
    entry.OperatorD->Opcode = opcode;

    _findCache[name] = kw;
    return kw;
}

AGS::Symbol AGS::SymbolTable::AddOperator(Predefined kw, std::string const & name, CodeCell opcode, size_t binary_prio, size_t unary_prio)
{
    SymbolTableEntry &entry = entries.at(kw);
    entry.Name = name;
    entry.OperatorD = new SymbolTableEntry::OperatorDesc;
    entry.OperatorD->BinaryPrio = binary_prio;
    entry.OperatorD->UnaryPrio = unary_prio;
    entry.OperatorD->Opcode = opcode;
    entry.OperatorD->CanBePartOfAnExpression = true;

    _findCache[name] = kw;
    return kw;
}

AGS::Symbol AGS::SymbolTable::AddVartype(Predefined kw, std::string const &name, size_t size, bool is_integer_vartype)
{
    SymbolTableEntry &entry = entries.at(kw);
    entry.Name = name;
    entry.VartypeD = new SymbolTableEntry::VartypeDesc;
    entry.VartypeD->Size = size;
    entry.VartypeD->Type = VTT::kAtomic;
    entry.VartypeD->Flags[VTF::kIntegerVartype] = is_integer_vartype;

    _findCache[name] = kw;
    return kw;
}
