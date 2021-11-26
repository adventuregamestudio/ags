
#include <cassert>
#include <cstdarg>
#include <limits>

#include "cc_symboltable.h"
#include "cs_compile_time.h"

// For printf rendering of floats.
// 8 decimal digits don't lose precision in 32 bit IEEE 754 floats
static char const *const kFloatPrintFormat = "%#.8g";

std::string AGS::CompileTimeFunc::FloatToString(float val)
{
    char sizing_buffer[1];
    double const dval = static_cast<double>(val);

    // Reserve sufficient space for the formatted string
    char *buffer = new char[1 + snprintf(sizing_buffer, 0, kFloatPrintFormat, dval)];
    sprintf(buffer, kFloatPrintFormat, dval);
    std::string ret = std::string(buffer);
    delete[] buffer;
    return ret;
}

AGS::Symbol AGS::CompileTimeFunc::FindOrMakeLiteral(CodeCell value) const
{
    return _sym.FindOrMakeLiteral(std::to_string(value), kKW_Int, value);
}

AGS::Symbol AGS::CompileTimeFunc::FindOrMakeLiteral(float value) const
{
    // Don't use to_string(); the generated literal needs to be different from
    // int literals, and to_string() doesn't guarantee that
    // (to_string(0.0) == "0")
    return _sym.FindOrMakeLiteral(FloatToString(value), kKW_Float, InterpretFloatAsInt(value));
}

void AGS::CompileTimeFunc::UserError(char const *msg, ...)
{
    // ErrorWithPosition() can't be called with a va_list and doesn't have a variadic variant,
    // so convert all the parameters into a single C string here
    va_list vlist1, vlist2;
    va_start(vlist1, msg);
    va_copy(vlist2, vlist1);
    size_t const needed_len = vsnprintf(nullptr, 0u, msg, vlist1) + 1u;
    std::vector<char> message(needed_len);
    vsprintf(&message[0u], msg, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    throw CompileTimeError(&message[0]);
}

void AGS::CTF_IntToInt::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;
    CodeCell const res = _func(i1, i2);
    result = FindOrMakeLiteral(res);
}

AGS::CTF_IntPlus::CTF_IntPlus(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_Plus, [](CodeCell i1, CodeCell i2) { return i1 + i2; })
{ }

void AGS::CTF_IntPlus::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (((i1 > 0 && i2 > 0) || (i1 < 0 && i2 < 0)) &&
        (std::numeric_limits<CodeCell>::max() - abs(i1) < abs(i2)))
        UserError(
            "Overflow when calculating '%s + %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());

    CTF_IntToInt::Evaluate(arg1, arg2, result);
}

AGS::CTF_IntMinus::CTF_IntMinus(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_Minus, [](CodeCell i1, CodeCell i2) { return i1 - i2; })
{ }

void AGS::CTF_IntMinus::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (((i1 > 0 && i2 < 0) || (i1 < 0 && i2 > 0)) &&
        (std::numeric_limits<CodeCell>::max() - abs(i1) < abs(i2)))
        UserError(
            "Overflow when calculating '%s - %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());

    CTF_IntToInt::Evaluate(arg1, arg2, result);
}

AGS::CTF_IntMultiply::CTF_IntMultiply(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_Multiply, [](CodeCell i1, CodeCell i2) { return i1 * i2; })
{ }

void AGS::CTF_IntMultiply::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    // Overflow detection - unsigned values cannot generate overflow by definition;
    // leverage the fact that they have inherent wrap-around 
    if (i1 != 0 && i2 != 0)
    {
        uint32_t const ui1 = abs(i1);
        uint32_t const ui2 = abs(i2);
        uint32_t const product = ui1 * ui2;

        bool const wraparound_happened = product < ui1 || product < ui2;
        if (wraparound_happened || product > INT_MAX)
            UserError(
                "Overflow when calculating '%s * %s'",
                std::to_string(i1).c_str(),
                std::to_string(i2).c_str());
    }
    CTF_IntToInt::Evaluate(arg1, arg2, result);
}

AGS::CTF_IntDivide::CTF_IntDivide(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_Divide, [](CodeCell i1, CodeCell i2) { return i1 / i2; })
{ }

void AGS::CTF_IntDivide::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (0 == i2)
        UserError(
            "Division by zero when calculating '%s / %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());

    CTF_IntToInt::Evaluate(arg1, arg2, result);
}

AGS::CTF_IntShiftLeft::CTF_IntShiftLeft(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_ShiftLeft, [](CodeCell i1, CodeCell i2) { return i1 << i2; })
{ }

void AGS::CTF_IntShiftLeft::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (i2 < 0)
        UserError(
            "Negative shift amount when calculating '%s << %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());

    // The Engine calculates shifts by using signed values, so overflow is possible. 
    size_t const digitnum = std::numeric_limits<CodeCell>::digits - 1;
    if (0 != abs(i1) >> (digitnum - i2))
        UserError(
            "Overflow when calculating '%s << %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());

    CTF_IntToInt::Evaluate(arg1, arg2, result);
}

AGS::CTF_IntShiftRight::CTF_IntShiftRight(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_ShiftRight, [](CodeCell i1, CodeCell i2) { return i1 >> i2; })
{ }

void AGS::CTF_IntShiftRight::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (i2 < 0)
        UserError(
            "Negative shift amount when calculating '%s << %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());

    CTF_IntToInt::Evaluate(arg1, arg2, result);
}

AGS::CTF_IntModulo::CTF_IntModulo(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_Modulo, [](CodeCell i1, CodeCell i2) { return i1 % i2; })
{ }

void AGS::CTF_IntModulo::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (0 == i2)
        UserError(
            "Modulo zero encountered when calculating '%s %% %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());

    CTF_IntToInt::Evaluate(arg1, arg2, result);
}

void AGS::CTF_IntToBool::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;
    bool res = _func(i1, i2);
    result = FindOrMakeLiteral(static_cast<CodeCell>(res));
}

void AGS::CTF_FloatToFloat::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    float const f1 = InterpretIntAsFloat(_sym[arg1].LiteralD->Value);
    float const f2 = InterpretIntAsFloat(_sym[arg2].LiteralD->Value);
    float res = _func(f1, f2);
    if (HUGE_VAL == res || -HUGE_VAL == res || NAN == res)
        UserError(
            "Overflow on calculating '%s %s %s'",
            FloatToString(f1), _sym.GetName(_name).c_str(),  FloatToString(f2));

    result = FindOrMakeLiteral(res);
}

void AGS::CTF_FloatToBool::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    float const f1 = InterpretIntAsFloat(_sym[arg1].LiteralD->Value);
    float const f2 = InterpretIntAsFloat(_sym[arg2].LiteralD->Value);
    bool res = _func(f1, f2);
    result = FindOrMakeLiteral(static_cast<CodeCell>(res));
}

AGS::CTF_FloatDivide::CTF_FloatDivide(SymbolTable &sym)
    : CTF_FloatToFloat(sym, kKW_Divide, [](float f1, float f2) { return f1 / f2; })
{}

void AGS::CTF_FloatDivide::Evaluate(Symbol arg1, Symbol arg2, Symbol &result)
{
    float const f1 = InterpretIntAsFloat(_sym[arg1].LiteralD->Value);
    float const f2 = InterpretIntAsFloat(_sym[arg2].LiteralD->Value);

    if (0.0 == f2)
        UserError(
            "Division by zero when calculating '%s / %s'",
            FloatToString(f1), FloatToString(f2));

    CTF_FloatToFloat::Evaluate(arg1, arg2, result);
}

