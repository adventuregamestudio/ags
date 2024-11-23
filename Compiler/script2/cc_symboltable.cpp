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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cc_symboltable.h"
#include "script/cc_internal.h"

const size_t AGS::SymbolTableConstant::kParameterScope = 1u;
const size_t AGS::SymbolTableConstant::kFunctionScope = 2u;
const size_t AGS::SymbolTableConstant::kNoSrcLocation = INT_MAX;
const int AGS::SymbolTableConstant::kNoPrio = -1;
const int AGS::SymbolTableConstant::kNoOpcode = -1;
const int AGS::SymbolTableConstant::kSpecialLogic = -2;

AGS::SymbolTableEntry::~SymbolTableEntry()
{
    // (note that null pointers may be safely 'delete'd, in contrast to 'free'd)
    delete this->AttributeD;
    this->AttributeD = nullptr;
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
    this->LifeScope = orig.LifeScope;
    this->Accessed = orig.Accessed;

    // Deep copy semantics.
    this->AttributeD = (orig.AttributeD) ? new SymbolTableEntry::AttributeDesc{ *(orig.AttributeD) } : nullptr;
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
        { TQ::kAutoptr,         kKW_Autoptr, },
        { TQ::kBuiltin,         kKW_Builtin, },
        { TQ::kImport,          kKW_ImportStd, },
        { TQ::kManaged,         kKW_Managed,  },
        { TQ::kProtected,       kKW_Protected,  },
        { TQ::kReadonly,        kKW_Readonly, },
        { TQ::kStatic,          kKW_Static, },
        { TQ::kInternalstring,  kKW_Internalstring, },
        { TQ::kWriteprotected,  kKW_Writeprotected, },
    };

    return tq2sym;
}

AGS::TypeQualifierSet AGS::TypeQualifierSet::WithoutTypedefQualifiers()
{
    TypeQualifierSet ret{ *this };
    ret[TQ::kAutoptr] =
        ret[TQ::kBuiltin] =
        ret[TQ::kInternalstring] =
        ret[TQ::kManaged] = false;
    return ret;
}

AGS::TypeQualifierSet AGS::TypeQualifierSet::WithouttVarFuncDefQualifiers()
{
    TypeQualifierSet ret{ *this };
    ret[TQ::kImport] =
        ret[TQ::kProtected] =
        ret[TQ::kReadonly] =
        ret[TQ::kStatic] =
        ret[TQ::kWriteprotected] = false;
    return ret;
}

void AGS::SymbolTableEntry::Clear()
{
    // Leave the name so that symbols in the main phase equate to symbols in the pre phase.
    Declared = SymbolTableConstant::kNoSrcLocation;
    Scope = 0u;
    LifeScope = {};
    // Don't clear Accessed so when a function is first used and then declared, this doesn't clobber the use.

    delete AttributeD;
    AttributeD = nullptr;
    delete ComponentD;
    ComponentD = nullptr;
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
    delete VariableD;
    VariableD = nullptr;
    delete VartypeD;
    VartypeD = nullptr;
}

AGS::SymbolTableEntry::SymbolTableEntry(SymbolTableEntry const &orig)
{
    *this = orig;
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
    AddOperator(kKW_And, "&&", 118, kNoPrio, true, SCMD_AND);
    // No compile time functions defined here; those are done with special logic.

    AddOperator(kKW_BitAnd, "&", 109, kNoPrio, false, SCMD_BITAND);
    OperatorCtFunctions(
        kKW_BitAnd,
        new CTF_IntToInt(*this, kKW_BitAnd, [](CodeCell i1, CodeCell i2) { return i1 & i2; }),
        nullptr);

    AddOperator(kKW_BitNeg, "~", kNoPrio, 101, false, kSpecialLogic); // bitwise NOT
    OperatorCtFunctions(
        kKW_BitNeg,
        new CTF_IntToInt(*this, kKW_BitNeg, [](CodeCell i1, CodeCell i2) { return ~i2; }),
        nullptr);

    AddOperator(kKW_BitOr, "|", 110, kNoPrio, false, SCMD_BITOR);
    OperatorCtFunctions(
        kKW_BitOr,
        new CTF_IntToInt(*this, kKW_BitOr, [](CodeCell i1, CodeCell i2) { return i1 | i2; }),
        nullptr);

    AddOperator(kKW_BitXor, "^", 110, kNoPrio, false, SCMD_XORREG);
    OperatorCtFunctions(
        kKW_BitXor,
        new CTF_IntToInt(*this, kKW_BitXor, [](CodeCell i1, CodeCell i2) { return i1 ^ i2; }),
        nullptr);

    AddOperator(kKW_Decrement, "--", 101, 101, false, SCMD_SUB);
    // No compile time functions defined here; those are done with special logic.

    AddOperator(kKW_Divide, "/", 103, kNoPrio, false, SCMD_DIVREG, SCMD_FDIVREG);
    OperatorCtFunctions(
        kKW_Divide,
        new CTF_IntDivide(*this),
        new CTF_FloatDivide(*this));

    AddOperator(kKW_Equal, "==", 112, kNoPrio, true, SCMD_ISEQUAL, SCMD_ISEQUAL, SCMD_ISEQUAL, SCMD_STRINGSEQUAL);
    OperatorCtFunctions(
        kKW_Equal,
        new CTF_IntToBool(*this, kKW_Equal, [](CodeCell i1, CodeCell i2) { return i1 == i2; }),
        new CTF_FloatToBool(*this, kKW_Equal, [](float f1, float f2) { return f1 == f2; }));

    AddOperator(kKW_Greater, ">", 112, kNoPrio, true, SCMD_GREATER, SCMD_FGREATER);
    OperatorCtFunctions(
        kKW_Greater,
        new CTF_IntToBool(*this, kKW_Greater, [](CodeCell i1, CodeCell i2) { return i1 > i2; }),
        new CTF_FloatToBool(*this, kKW_Greater, [](float f1, float f2) { return f1 > f2; }));

    AddOperator(kKW_GreaterEqual, ">=", 112, kNoPrio, true, SCMD_GTE, SCMD_FGTE);
    OperatorCtFunctions(
        kKW_GreaterEqual,
        new CTF_IntToBool(*this, kKW_GreaterEqual, [](CodeCell i1, CodeCell i2) { return i1 >= i2; }),
        new CTF_FloatToBool(*this, kKW_GreaterEqual, [](float f1, float f2) { return f1 >= f2; }));

    AddOperator(kKW_Increment, "++", 101, 101, false, SCMD_ADD);
    // No compile time functions defined here; those are done with special logic.

    AddOperator(kKW_Less, "<", 112, kNoPrio, true, SCMD_LESSTHAN, SCMD_FLESSTHAN);
    OperatorCtFunctions(
        kKW_Less,
        new CTF_IntToBool(*this, kKW_Less, [](CodeCell i1, CodeCell i2) { return i1 < i2; }),
        new CTF_FloatToBool(*this, kKW_Less, [](float f1, float f2) { return f1 < f2; }));

    AddOperator(kKW_LessEqual, "<=", 112, kNoPrio, true, SCMD_LTE, SCMD_FLTE);
    OperatorCtFunctions(
        kKW_LessEqual,
        new CTF_IntToBool(*this, kKW_LessEqual, [](CodeCell i1, CodeCell i2) { return i1 <= i2; }),
        new CTF_FloatToBool(*this, kKW_LessEqual, [](float f1, float f2) { return f1 <= f2; }));

    AddOperator(kKW_Minus, "-", 105, 101, false, SCMD_SUBREG, SCMD_FSUBREG);
    OperatorCtFunctions(
        kKW_Minus,
        new CTF_IntMinus(*this),
        new CTF_FloatToFloat(*this, kKW_Minus, [](float f1, float f2) { return f1 - f2; }));

    AddOperator(kKW_Modulo, "%", 103, kNoPrio, false, SCMD_MODREG);
    OperatorCtFunctions(
        kKW_Modulo,
        new CTF_IntModulo(*this),
        nullptr);

    AddOperator(kKW_Multiply, "*", 103, kNoPrio, false, SCMD_MULREG, SCMD_FMULREG);
    OperatorCtFunctions(
        kKW_Multiply,
        new CTF_IntMultiply(*this),
        new CTF_FloatToFloat(*this, kKW_Multiply, [](float f1, float f2) { return f1 * f2; }));

    AddOperator(kKW_Not, "!", kNoPrio, 101, true, SCMD_NOTREG); // boolean NOT
    OperatorCtFunctions(
        kKW_Not,
        new CTF_IntToBool(*this, kKW_Not, [](int i1, int i2) { return !i2; }),
        nullptr);

    AddOperator(kKW_New, "new", kNoPrio, 101, false, kSpecialLogic);
    // No compile time functions defined here, will be handled by special logic

    AddOperator(kKW_NotEqual, "!=", 112, kNoPrio, true, SCMD_NOTEQUAL, SCMD_NOTEQUAL, SCMD_NOTEQUAL, SCMD_STRINGSNOTEQ);
    OperatorCtFunctions(
        kKW_NotEqual,
        new CTF_IntToBool(*this, kKW_NotEqual, [](int i1, int i2) { return i1 != i2; }),
        new CTF_FloatToBool(*this, kKW_NotEqual, [](float f1, float f2) { return f1 != f2; }));

    AddOperator(kKW_Or, "||", 119, kNoPrio, true, SCMD_OR);
    // No compile time functions defined here; those are handled with special logic.

    AddOperator(kKW_Plus, "+", 105, 101, false, SCMD_ADDREG, SCMD_FADDREG);
    OperatorCtFunctions(
        kKW_Plus,
        new CTF_IntPlus(*this),
        new CTF_FloatToFloat(*this, kKW_Plus, [](float f1, float f2) { return f1 + f2; }));
        
    AddOperator(kKW_ShiftLeft, "<<", 107, kNoPrio, false, SCMD_SHIFTLEFT);
    OperatorCtFunctions(
        kKW_ShiftLeft,
        new CTF_IntShiftLeft(*this),
        nullptr);

    AddOperator(kKW_ShiftRight, ">>", 107, kNoPrio, false, SCMD_SHIFTRIGHT);
    OperatorCtFunctions(
        kKW_ShiftRight,
        new CTF_IntShiftRight(*this),
        nullptr);

    AddOperator(kKW_Tern, "?", 120, kNoPrio, false, kSpecialLogic);    // note, operator and keyword
    // No compile time functions defined here; those are handled with special logic.

    // Assignments
    AddAssign(kKW_Assign, "=", 120, kSpecialLogic, kSpecialLogic, kSpecialLogic, kSpecialLogic);

    AddAssign(kKW_AssignBitAnd, "&=", 120, SCMD_BITAND);
    AddAssign(kKW_AssignBitOr, "|=", 120, SCMD_BITOR);
    AddAssign(kKW_AssignBitXor, "^=", 120, SCMD_XORREG);
    AddAssign(kKW_AssignDivide, "/=", 120, SCMD_DIVREG, SCMD_FDIVREG);
    AddAssign(kKW_AssignMinus, "-=", 120, SCMD_SUBREG, SCMD_FSUBREG);
    AddAssign(kKW_AssignMultiply, "*=", 120, SCMD_MULREG, SCMD_FMULREG);
    AddAssign(kKW_AssignPlus, "+=", 120, SCMD_ADDREG, SCMD_FADDREG);
    AddAssign(kKW_AssignShiftLeft, "<<=", 120, SCMD_SHIFTLEFT);
    AddAssign(kKW_AssignShiftRight, ">>=", 120, SCMD_SHIFTRIGHT);

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
    AddKeyword(kKW_FallThrough, "fallthrough");
    AddKeyword(kKW_For, "for");
    AddKeyword(kKW_If, "if");
    AddKeyword(kKW_ImportStd, "import");
    AddKeyword(kKW_ImportTry, "_tryimport");
    AddKeyword(kKW_Internalstring, "internalstring");
    AddKeyword(kKW_Managed, "managed");
    AddKeyword(kKW_Noloopcheck, "noloopcheck");
    AddKeyword(kKW_Null, "null");
    MakeEntryLiteral(kKW_Null);
    entries[kKW_Null].LiteralD->Vartype = kKW_Null;
    entries[kKW_Null].LiteralD->Value = 0u;
    AddKeyword(kKW_OnePastLongMax, "2147483648");
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
        Symbol const int_zero_sym = Add("0");
        MakeEntryLiteral(int_zero_sym);
        entries[int_zero_sym].LiteralD->Value = 0;
        entries[int_zero_sym].LiteralD->Vartype = kKW_Int;
    }
    {
        Symbol const one_sym = Add("1");
        MakeEntryLiteral(one_sym);
        entries[one_sym].LiteralD->Value = 1;
        entries[one_sym].LiteralD->Vartype = kKW_Int;
    }
    {
        Symbol const long_min_sym = Add("-2147483648");
        MakeEntryLiteral(long_min_sym);
        entries[long_min_sym].LiteralD->Value = INT32_MIN;
        entries[long_min_sym].LiteralD->Vartype = kKW_Int;
    }
    {
        Symbol const float_zero_sym = Add("0.0");
        MakeEntryLiteral(float_zero_sym);
        entries[float_zero_sym].LiteralD->Value = 0;
        entries[float_zero_sym].LiteralD->Vartype = kKW_Float;
    }
    _lastAllocated = VartypeWithConst(kKW_String);
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
    _stringStructPtrSym = VartypeWithDynpointer(s);
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
    for (size_t idx = 0u; idx < name.size(); ++idx)
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
        IsVariable(s) ||
        (s == kKW_OnePastLongMax);
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

size_t AGS::SymbolTable::ArrayElementsCount(Symbol s) const
{
    if (IsVariable(s))
        s = entries.at(s).VariableD->Vartype;
    if (!IsVartype(s))
        return 0u;

    SymbolTableEntry::VartypeDesc const *vdesc = entries.at(s).VartypeD;
    size_t const dims_size = vdesc->Dims.size();
    if (0u == dims_size)
        return 0u;

    size_t count = 1u;
    for (size_t dims_idx = 0u; dims_idx < dims_size; ++dims_idx)
        count *= vdesc->Dims[dims_idx];
    return count;
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
    if (IsAutoptrVartype(symbl))
    {
        std::string name = entries[symbl].Name;
        int const pos_of_last_ch = name.length() - 1u;
        if (pos_of_last_ch >= 0 && ' ' == name[pos_of_last_ch])
            name.resize(pos_of_last_ch); // cut off trailing ' '
        return name;
    }
    return entries[symbl].Name;
}

AGS::Vartype AGS::SymbolTable::VartypeWithArray(std::vector<size_t> const &dims, AGS::Vartype vartype)
{
    // If 'vartype' is an array, the new array index must be spliced into the vartype name
    // in front of the first '['
    std::string pre = GetName(vartype);
    size_t first_bracket_pos = pre.find_first_of('[');
    if (std::string::npos == first_bracket_pos)
        first_bracket_pos = pre.length();
    std::string post = pre.substr(first_bracket_pos);
    pre = pre.substr(0u, first_bracket_pos);

    bool const is_array_of_array = IsVTT(vartype, VTT::kArray);
    std::vector<size_t> aoa_dims;
    if (is_array_of_array)
    {
        // Classic array of classic array: Make the vartype of one joint array of them both
        std::vector<size_t> const &old_dims = entries[vartype].VartypeD->Dims;
        aoa_dims.reserve(old_dims.size() + dims.size());
        aoa_dims = dims;
        aoa_dims.insert(aoa_dims.end(), old_dims.begin(), old_dims.end());
        // Cut off the first '[...]', it will be replaced by the index of the joint array
        first_bracket_pos = post.find_first_of('[', 1u);
        if (std::string::npos == first_bracket_pos)
            first_bracket_pos = post.length();
        post = post.substr(first_bracket_pos);
        vartype = entries[vartype].VartypeD->BaseVartype;
    }
    std::vector<size_t> const &dims_to_use = is_array_of_array ? aoa_dims : dims;

    std::string insert = "";
    size_t element_count = 1u;
    for (auto it = dims_to_use.cbegin(); it != dims_to_use.cend(); ++it)
    {
        element_count *= *it;
        insert += std::to_string(*it) + ", ";
    }
    insert = "[" + insert.substr(0u, insert.length() - 2u) + "]";
    std::string const vartype_name = pre + insert + post;
    Vartype const array_vartype = FindOrAdd(vartype_name);
    if (!IsVartype(array_vartype))
    {
        // Initialise the new vartype
        entries[array_vartype].VartypeD = new SymbolTableEntry::VartypeDesc;
        entries[array_vartype].VartypeD->Type = VTT::kArray;
        entries[array_vartype].VartypeD->BaseVartype = vartype;
        entries[array_vartype].VartypeD->Size = element_count * GetSize(vartype);
        entries[array_vartype].VartypeD->Dims = dims_to_use;
    }
    return array_vartype;
}

AGS::Vartype AGS::SymbolTable::VartypeWithConst(AGS::Vartype vartype)
{
    if (IsVTT(vartype, VTT::kConst))
        return vartype; // Nothing to be done

    // Return cached result if existent 
    std::pair<Vartype, VartypeType> const arg = { vartype, VTT::kConst };
    Vartype &valref(_vartypesCache[arg]);
    if (valref)
        return valref;

    std::string const new_typename = "const " + entries[vartype].Name;
    Vartype new_type = valref = FindOrAdd(new_typename);
    if (!IsVartype(new_type))
    {
        // Initialise entry for new type
        SymbolTableEntry &entry = entries[new_type];
        entry.VartypeD = new SymbolTableEntry::VartypeDesc;
        entry.VartypeD->Type = VTT::kConst;
        entry.VartypeD->BaseVartype = vartype;
        entry.VartypeD->Size = GetSize(vartype);
    }
    return new_type;
}

AGS::Vartype AGS::SymbolTable::VartypeWithDynarray(AGS::Vartype vartype)
{
    // Return cached result if it exists
    std::pair<Vartype, VartypeType> const arg = { vartype, VTT::kDynarray };
    Vartype &valref(_vartypesCache[arg]);
    if (valref)
        return valref;

    // For the new typename, insert '[}' in front of the first bracket of the actual typename
    std::string &act_typename = entries[vartype].Name;
    size_t pos_of_first_openbracket = act_typename.find_first_of('[');
    if (std::string::npos == pos_of_first_openbracket)
        pos_of_first_openbracket = act_typename.length();
    std::string const pre = act_typename.substr(0u, pos_of_first_openbracket);
    std::string const post = act_typename.substr(pos_of_first_openbracket);
    std::string const new_typename = pre + "[]" + post;

    Vartype new_type = valref = FindOrAdd(new_typename);
    if (!IsVartype(new_type))
    {
        // Initialise entry for new type
        SymbolTableEntry &entry = entries[new_type];
        entry.VartypeD = new SymbolTableEntry::VartypeDesc;
        entry.VartypeD->Type = VTT::kDynarray;
        entry.VartypeD->BaseVartype = vartype;
        entry.VartypeD->Size = SIZE_OF_DYNPOINTER;
    }
    return new_type;
}

AGS::Vartype AGS::SymbolTable::VartypeWithDynpointer(Vartype vartype)
{
    if (IsVTT(vartype, VTT::kDynpointer))
        return vartype; // Nothing to be done

    // Return cached result if it exists
    std::pair<Vartype, VartypeType> const arg = { vartype, VTT::kDynpointer };
    Vartype &valref(_vartypesCache[arg]);
    if (valref)
        return valref;

    std::string post = " *";
    // Note: Different types must have different names. So even if we have an
    // autoptr and suppress the '*', we still need to add _something_ to the new name.
    // (If this turns out to be too ugly for users to see, then we need two
    // fields per vartype: one field that is output to the user, another field for
    // internal use that is guaranteed to have different values for different vartypes.)
    if (IsVartype(vartype) && entries[vartype].VartypeD->Flags[VTF::kAutoptr])
        post = " ";
    std::string const new_typename = entries[vartype].Name + post;
    Vartype new_type = valref = FindOrAdd(new_typename);
    if (!IsVartype(new_type))
    {
        // Initialise entry for new type
        SymbolTableEntry &entry = entries[new_type];
        entry.VartypeD = new SymbolTableEntry::VartypeDesc;
        entry.VartypeD->Type = VTT::kDynpointer;
        entry.VartypeD->BaseVartype = vartype;
        entry.VartypeD->Size = SIZE_OF_DYNPOINTER;
    }
    return new_type;
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
        strct = entries.at(strct).VartypeD->Parent;
    }
    return kKW_NoSymbol;
}

AGS::Symbol AGS::SymbolTable::FindConstructorOfTypeOrParent(Symbol strct) const
{
    if (!IsVartype(strct) || !entries.at(strct).VartypeD->Flags[VTF::kStruct])
        return kKW_NoSymbol; // not a struct

    // Check for the constructor in the current struct and in all its parents
    while (strct)
    {
        if (entries.at(strct).VartypeD->Constructor != kKW_NoSymbol)
            return entries.at(strct).VartypeD->Constructor;
        strct = entries.at(strct).VartypeD->Parent;
    }
    return kKW_NoSymbol;
}

AGS::Vartype AGS::SymbolTable::GetFirstBaseVartype(AGS::Vartype vartype) const
{
    // TODO: extra safety checks?
    for (const auto *ste = &entries[vartype]; ste->VartypeD && (ste->VartypeD->BaseVartype > 0);
        vartype = ste->VartypeD->BaseVartype, ste = &entries[vartype]) { }
    return vartype;
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
        size_t const new_size1 = entries.capacity() * 2u;
        size_t const new_size2 = entries.capacity() + 1000u;
        entries.reserve((new_size1 < new_size2) ? new_size1 : new_size2);
    }
    int const idx_of_new_entry = entries.size();
    entries.emplace_back();
    entries.at(idx_of_new_entry).Name = name;
    _findCache[name] = idx_of_new_entry;
    return idx_of_new_entry;
}

AGS::Symbol AGS::SymbolTable::FindOrMakeLiteral(std::string const &name, Vartype vartype, CodeCell value)
{
    Symbol lit = Find(name);
    if (kKW_NoSymbol == lit)
    {
        lit = Add(name);
        MakeEntryLiteral(lit);
        entries[lit].LiteralD->Vartype = vartype;
        entries[lit].LiteralD->Value = value;
    }
    return lit;
}

AGS::Symbol AGS::SymbolTable::AddNoSymbol(Predefined kw, std::string const &name)
{
    SymbolTableEntry &entry = entries[kw];
    entry.Name = name;

    _findCache[name] = kw;
    return kw;
}

AGS::Symbol AGS::SymbolTable::AddAssign(Predefined kw, std::string const &name, size_t prio, CodeCell int_opcode, CodeCell float_opcode, CodeCell dyn_opcode, CodeCell string_opcode)
{
    SymbolTableEntry &entry = entries.at(kw);
    entry.Name = name;
    entry.OperatorD = new SymbolTableEntry::OperatorDesc;
    entry.OperatorD->BinaryPrio = prio;
    entry.OperatorD->UnaryPrio = kNoPrio;
    entry.OperatorD->IntOpcode = int_opcode;
    entry.OperatorD->FloatOpcode = float_opcode;
    entry.OperatorD->DynOpcode = dyn_opcode;

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

AGS::Symbol AGS::SymbolTable::AddOperator(Predefined kw, std::string const &name, size_t binary_prio, size_t unary_prio, bool boolean, CodeCell int_opcode, CodeCell float_opcode, CodeCell dyn_opcode, CodeCell string_opcode)
{
    SymbolTableEntry &entry = entries.at(kw);
    entry.Name = name;
    entry.OperatorD = new SymbolTableEntry::OperatorDesc;
    entry.OperatorD->BinaryPrio = binary_prio;
    entry.OperatorD->UnaryPrio = unary_prio;
    entry.OperatorD->Boolean = boolean;
    entry.OperatorD->IntOpcode = int_opcode;
    entry.OperatorD->FloatOpcode = float_opcode;
    entry.OperatorD->DynOpcode = dyn_opcode;
    entry.OperatorD->StringOpcode = string_opcode;
    entry.OperatorD->CanBePartOfAnExpression = true;

    _findCache[name] = kw;
    return kw;
}

void AGS::SymbolTable::OperatorCtFunctions(Predefined kw, CompileTimeFunc * int_ct_func, CompileTimeFunc * float_ct_func)
{
    SymbolTableEntry &entry = entries.at(kw);
    entry.OperatorD->IntCTFunc = int_ct_func;
    entry.OperatorD->FloatCTFunc = float_ct_func;
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
