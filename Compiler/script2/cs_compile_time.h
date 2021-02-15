#ifndef __CS_COMPILE_TIME_H
#define __CS_COMPILE_TIME_H

#include <functional>
#include "cs_parser_common.h" 

namespace AGS
{
struct SymbolTable;

// Encapsules a function that will be called at compile time 
// (when both its arguments are known at compile time)
class CompileTimeFunc
{
protected:
    SymbolTable &_sym;

    static float InterpretIntAsFloat(CodeCell val) { return *reinterpret_cast<float *>(&val); }
    static CodeCell InterpretFloatAsInt(float val) { return *reinterpret_cast<CodeCell *>(&val); }

    // converts the float to a string
    static std::string FloatToString(float val);

    Symbol FindOrMakeLiteral(CodeCell value) const;

    Symbol FindOrMakeLiteral(float value) const;

    void Error(MessageHandler &mh, std::string const &section, size_t const line, char const *msg, ...);

public:

    CompileTimeFunc(SymbolTable &sym)
        : _sym(sym)
    {}

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result)
        = 0;
};

// A compile time func that operates on integers and yields an integer
class CTF_IntToInt : public CompileTimeFunc
{
protected:
    CodeCell (*_func) (CodeCell, CodeCell); // The compile time function to execute
    Symbol _name; // of the operator symbol

public:
    CTF_IntToInt(SymbolTable &sym, Symbol name, CodeCell (*func)(CodeCell, CodeCell))
        : CompileTimeFunc(sym)
        , _func(func)
    {}

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// '+' on integers
class CTF_IntPlus : public CTF_IntToInt
{
public:
    CTF_IntPlus(SymbolTable &sym);

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// '-' on integers
class CTF_IntMinus : public CTF_IntToInt
{
public:
    CTF_IntMinus(SymbolTable &sym);

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// '*' on integers
class CTF_IntMultiply : public CTF_IntToInt
{
public:
    CTF_IntMultiply(SymbolTable &sym);

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// '/' on integers
class CTF_IntDivide : public CTF_IntToInt
{
public:
    CTF_IntDivide(SymbolTable &sym);

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// '%' (modulo) on integers
class CTF_IntModulo : public CTF_IntToInt
{
public:
    CTF_IntModulo(SymbolTable &sym);

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// '<<' on (signed) integers
class CTF_IntShiftLeft : public CTF_IntToInt
{
public:
    CTF_IntShiftLeft(SymbolTable &sym);

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// '>>' on (signed) integers
class CTF_IntShiftRight : public CTF_IntToInt
{
public:
    CTF_IntShiftRight(SymbolTable &sym);

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// Encapsulates a function on integers that yields a bool
class CTF_IntToBool : public CompileTimeFunc
{
private:
    // The compile time function
    bool (*_func)(CodeCell, CodeCell);
    Symbol _name;

public:
    CTF_IntToBool(SymbolTable &sym, Symbol name, bool (*func)(CodeCell, CodeCell))
        : CompileTimeFunc(sym)
        , _func(func)
        , _name(name)
    {}

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// Encapsulate a function on floats that yields a float
class CTF_FloatToFloat : public CompileTimeFunc
{
protected:
    // The compile time function
    float (*_func)(float, float);
    Symbol _name;

public:
    CTF_FloatToFloat(SymbolTable &sym, Symbol name, float(*func)(float, float))
        : CompileTimeFunc(sym)
        , _func(func)
        , _name(name)
    {}

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// '/' on floats
class CTF_FloatDivide : public CTF_FloatToFloat
{

public:
    CTF_FloatDivide(SymbolTable &sym);

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

// Encapsulates a function on floats that yields a bool
class CTF_FloatToBool : public CompileTimeFunc
{
private:
    // The compile time function
    bool (*_func)(float, float);
    Symbol _name;

public:
    CTF_FloatToBool(SymbolTable &sym, Symbol name, bool (*func)(float, float))
        : CompileTimeFunc(sym)
        , _func(func)
        , _name(name)
    {}

    virtual ErrorType Evaluate(MessageHandler &mh, std::string const &section, size_t line, Symbol arg1, Symbol arg2, Symbol &result) override;
};

} // namespace AGS
#endif // __CS_COMPILE_TIME_H
