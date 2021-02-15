
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

void AGS::CompileTimeFunc::Error(MessageHandler &mh, std::string const &section, size_t const line, char const *msg, ...)
{
    // ErrorWithPosition() can't be called with a va_list and doesn't have a variadic variant,
    // so convert all the parameters into a single C string here
    va_list vlist1, vlist2;
    va_start(vlist1, msg);
    va_copy(vlist2, vlist1);
    char sizing_buffer[1];
    // Allocate enough memory for the message
    char *message = new char[1 + vsnprintf(sizing_buffer, 0, msg, vlist1)];
    vsprintf(message, msg, vlist2);
    va_end(vlist2);
    va_end(vlist1);

    mh.AddMessage(MessageHandler::kSV_Error, section, line, message);
    delete[] message;
}

AGS::ErrorType AGS::CTF_IntToInt::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol arg1, Symbol arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;
    CodeCell const res = _func(i1, i2);
    result = FindOrMakeLiteral(res);
    return kERR_None;
}

AGS::CTF_IntPlus::CTF_IntPlus(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_Plus, [](CodeCell i1, CodeCell i2) { return i1 + i2; })
{ }

AGS::ErrorType AGS::CTF_IntPlus::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (((i1 > 0 && i2 > 0) || (i1 < 0 && i2 < 0)) &&
        (std::numeric_limits<CodeCell>::max() - abs(i1) < abs(i2)))
    {
        Error(mh, section, line,
            "Overflow when calculating '%s + %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());
        return kERR_UserError;
    }
    return CTF_IntToInt::Evaluate(mh, section, line, arg1, arg2, result);
}

AGS::CTF_IntMinus::CTF_IntMinus(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_Minus, [](CodeCell i1, CodeCell i2) { return i1 - i2; })
{ }

AGS::ErrorType AGS::CTF_IntMinus::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (((i1 > 0 && i2 < 0) || (i1 < 0 && i2 > 0)) &&
        (std::numeric_limits<CodeCell>::max() - abs(i1) > abs(i2)))
    {
        Error(mh, section, line,
            "Overflow when calculating '%s - %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());
        return kERR_UserError;
    }
    return CTF_IntToInt::Evaluate(mh, section, line, arg1, arg2, result);
}

AGS::CTF_IntMultiply::CTF_IntMultiply(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_Multiply, [](CodeCell i1, CodeCell i2) { return i1 * i2; })
{ }

AGS::ErrorType AGS::CTF_IntMultiply::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    // Overflow detection - unsigned values cannot generate overflow by definition
    if (i1 != 0 && i2 != 0)
    {
        uint32_t const ui1 = abs(i1);
        uint32_t const ui2 = abs(i2);
        uint32_t const product = ui1 * ui2;

        if (product < ui1 || product < ui2)
        {
            Error(mh, section, line,
                "Overflow when calculating '%s * %s'",
                std::to_string(i1).c_str(),
                std::to_string(i2).c_str());
            return kERR_UserError;
        }
    }
    return CTF_IntToInt::Evaluate(mh, section, line, arg1, arg2, result);
}

AGS::CTF_IntDivide::CTF_IntDivide(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_Divide, [](CodeCell i1, CodeCell i2) { return i1 / i2; })
{ }

AGS::ErrorType AGS::CTF_IntDivide::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (0 == i2)
    {
        Error(mh, section, line,
            "Division by zero when calculating '%s / %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());
        return kERR_UserError;
    }
    return CTF_IntToInt::Evaluate(mh, section, line, arg1, arg2, result);
}

AGS::CTF_IntShiftLeft::CTF_IntShiftLeft(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_ShiftLeft, [](CodeCell i1, CodeCell i2) { return i1 << i2; })
{ }

AGS::ErrorType AGS::CTF_IntShiftLeft::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (i2 < 0)
    {
        Error(mh, section, line,
            "Negative shift amount when calculating '%s << %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());
        return kERR_UserError;
    }

    size_t const digitnum = std::numeric_limits<CodeCell>::digits - 1;
    if (0 != i1 >> (digitnum - i2))
    {
        Error(mh, section, line,
            "Overflow when calculating '%s << %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());
        return kERR_UserError;
    }
    return CTF_IntToInt::Evaluate(mh, section, line, arg1, arg2, result);
}

AGS::CTF_IntShiftRight::CTF_IntShiftRight(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_ShiftRight, [](CodeCell i1, CodeCell i2) { return i1 >> i2; })
{ }

AGS::ErrorType AGS::CTF_IntShiftRight::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (i2 < 0)
    {
        Error(mh, section, line,
            "Negative shift amount when calculating '%s << %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());
        return kERR_UserError;
    }

    return CTF_IntToInt::Evaluate(mh, section, line, arg1, arg2, result);
}

AGS::CTF_IntModulo::CTF_IntModulo(SymbolTable &sym)
    : CTF_IntToInt(sym, kKW_Modulo, [](CodeCell i1, CodeCell i2) { return i1 % i2; })
{ }

AGS::ErrorType AGS::CTF_IntModulo::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;

    if (0 == i2)
    {
        Error(mh, section, line,
            "Modulo is zero when calculating '%s %% %s'",
            std::to_string(i1).c_str(),
            std::to_string(i2).c_str());
        return kERR_UserError;
    }
    return CTF_IntToInt::Evaluate(mh, section, line, arg1, arg2, result);
}

AGS::ErrorType AGS::CTF_IntToBool::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    CodeCell const i1 = _sym[arg1].LiteralD->Value;
    CodeCell const i2 = _sym[arg2].LiteralD->Value;
    bool res = _func(i1, i2);
    result = FindOrMakeLiteral(static_cast<CodeCell>(res));
    return kERR_None;
}

AGS::ErrorType AGS::CTF_FloatToFloat::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    float const f1 = InterpretIntAsFloat(_sym[arg1].LiteralD->Value);
    float const f2 = InterpretIntAsFloat(_sym[arg2].LiteralD->Value);
    float res = _func(f1, f2);
    if (HUGE_VAL == res || -HUGE_VAL == res || NAN == res)
    {
        Error(mh, section, line,
            "Overflow on calculating '%s %s %s'",
            FloatToString(f1), _sym.GetName(_name).c_str(),  FloatToString(f2));
        return kERR_UserError;
    }
    result = FindOrMakeLiteral(res);
    return kERR_None;
}

AGS::ErrorType AGS::CTF_FloatToBool::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    float const f1 = InterpretIntAsFloat(_sym[arg1].LiteralD->Value);
    float const f2 = InterpretIntAsFloat(_sym[arg2].LiteralD->Value);
    bool res = _func(f1, f2);
    result = FindOrMakeLiteral(static_cast<CodeCell>(res));
    return kERR_None;
}

AGS::CTF_FloatDivide::CTF_FloatDivide(SymbolTable &sym)
    : CTF_FloatToFloat(sym, kKW_Divide, [](float f1, float f2) { return f1 / f2; })
{}

AGS::ErrorType AGS::CTF_FloatDivide::Evaluate(MessageHandler &mh, std::string const &section, size_t const line, Symbol const arg1, Symbol const arg2, Symbol &result)
{
    float const f1 = InterpretIntAsFloat(_sym[arg1].LiteralD->Value);
    float const f2 = InterpretIntAsFloat(_sym[arg2].LiteralD->Value);

    if (0.0 == f2)
    {
        Error(mh, section, line,
            "Division by zero when calculating '%s / %s'",
            FloatToString(f1), FloatToString(f2));
        return kERR_UserError;
    }

    return CTF_FloatToFloat::Evaluate(mh, section, line, arg1, arg2, result);
}

