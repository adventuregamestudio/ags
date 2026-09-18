//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Universal error class, that may be used both as a return value or
// thrown as an exception.
//
//=============================================================================
#ifndef __AGS_CN_UTIL__ERROR_H
#define __AGS_CN_UTIL__ERROR_H

#include <memory>
#include "util/string.h"

namespace AGS
{
namespace Common
{

class Error;
typedef std::shared_ptr<Error> PError;

//
// A simple struct, that provides several fields to describe an error in the program.
// If wanted, may be reworked into subclass of std::exception.
//
class Error
{
public:
    Error() = default;
    Error(PError inner_error, int code, String msg) : _code(code), _message(msg), _previousError(inner_error) {}
    Error(PError inner_error, int code) : _code(code), _previousError(inner_error) {}
    Error(PError inner_error, String msg) : _message(msg), _previousError(inner_error) {}
    Error(int code, String msg) : _code(code), _message(msg) {}
    Error(int code) : _code(code) {}
    Error(String msg) : _message(msg) {}

    Error(PError inner_error, int code, AGS_FORMAT_STRING_ARG const char* fmt, ...) AGS_FORMAT_STRING_OBJECT(3) : _code(code), _previousError(inner_error)
    {
        va_list args;
        va_start(args, fmt);
        _message.FormatV(fmt, args);
        va_end(args);
    }

    Error(PError inner_error, AGS_FORMAT_STRING_ARG const char* fmt, ...) AGS_FORMAT_STRING_OBJECT(2) : _previousError(inner_error)
    {
        va_list args;
        va_start(args, fmt);
        _message.FormatV(fmt, args);
        va_end(args);
    }

    Error(int code, AGS_FORMAT_STRING_ARG const char* fmt, ...) AGS_FORMAT_STRING_OBJECT(2) : _code(code)
    {
        va_list args;
        va_start(args, fmt);
        _message.FormatV(fmt, args);
        va_end(args);
    }

    explicit Error(AGS_FORMAT_STRING_ARG const char* fmt, ...) AGS_FORMAT_STRING_OBJECT(1)
    {
        va_list args;
        va_start(args, fmt);
        _message.FormatV(fmt, args);
        va_end(args);
    }        

    // Error code is a number, defining error subtype. It is not much use to the end-user,
    // but may be checked in the program to know more precise cause of the error.
    int    Code() const { return _code; }
    // Message description of this error type and subtype.
    String Message() const { return _message; }
    // A previous error that leads to this one. Usually empty.
    PError PreviousError() const { return _previousError; }
    // Full error message combines current error message with previous error accumulated. messages
    // NOTE: if made a child of std::exception, FullMessage may be substituted
    // or complemented with virtual const char* what().
    String FullMessage() const
    {
        String msg;
        for (const Error* err = this; err; err = err->_previousError.get())
            AppendLine(msg, err->_message);
        return msg;
    }

protected:
    void SetMessage(const String& msg) { _message = msg; }
    // Appends a line of text, separating it from the preceding one with a line break;
    // empty lines are skipped.
    static void AppendLine(String& text, const String& line)
    {
        if (line.IsEmpty())
            return;
        if (!text.IsEmpty())
            text.AppendChar('\n');
        text.Append(line);
    }

private:
    int    _code = 0; // numeric code, for specific uses
    String _message = ""; // message or description of this error
    String _comment = ""; // additional information about particular case
    PError _previousError = PError(); // previous error that caused this one
};


// ErrorHandle is a helper class that lets you have an Error object
// wrapped in a smart pointer. ErrorHandle's only data member is a
// shared_ptr, which means that it does not cause too much data copying
// when used as a function's return value.
// Note, that the reason to have distinct class instead of a shared_ptr's
// typedef is an inverted boolean comparison:
// shared_ptr converts to 'true' when it contains an object, but ErrorHandle
// returns 'true' when it *does NOT* contain an object, meaning there
// is no error.
template <class T> class ErrorHandle
{
public:
    static ErrorHandle<T> None() { return ErrorHandle(); }

    ErrorHandle() = default;
    ErrorHandle(T *err) : _error(err) {}
    ErrorHandle(std::shared_ptr<T> err) : _error(err) {}
    // Lets return a handle of a derived error type where a base one is expected;
    // an unrelated type fails to compile on the shared_ptr conversion below.
    template <class U> ErrorHandle(const ErrorHandle<U> &other) : _error(other._error) {}

    bool HasError() const { return _error.get() != nullptr; }
    explicit operator bool() const { return _error.get() == nullptr; }
    operator PError() const { return _error; }
    T *operator ->() const { return _error.operator->(); }
    T &operator *() const { return _error.operator*(); }

private:
    template <class U> friend class ErrorHandle;
    std::shared_ptr<T> _error;
};


// Basic error handle, containing Error object
typedef ErrorHandle<Error> HError;


// TypedCodeError is the Error's subclass, which only purpose is to override
// error code type in constructor and Code() getter, that may be useful if
// you'd like to restrict code values to particular enumerator.
// TODO: a type identifier as a part of template (string, or perhaps a int16
// to be placed at high-bytes in Code) to be able to distinguish error group.
template <typename CodeType, String (*GetErrorText)(CodeType)>
class TypedCodeError : public Error
{
public:
    TypedCodeError(CodeType code) : Error(PError(), code, GetErrorText(code)) {}
    TypedCodeError(CodeType code, String msg) :
        Error(code, CombineCodeText(code, msg)) {}
    TypedCodeError(PError inner_error, CodeType code) : Error(inner_error, code, GetErrorText(code)) {}
    TypedCodeError(PError inner_error, CodeType code, String msg) :
        Error(inner_error, code, CombineCodeText(code, msg)) {}

    TypedCodeError(CodeType code, AGS_FORMAT_STRING_ARG const char* fmt, ...) AGS_FORMAT_STRING_OBJECT(2) : Error(code)
    {
        String formatted;
        va_list args;
        va_start(args, fmt);
        formatted.FormatV(fmt, args);
        va_end(args);
        SetMessage(CombineCodeText(code, formatted));
    }

    TypedCodeError(PError inner_error, CodeType code, AGS_FORMAT_STRING_ARG const char* fmt, ...) AGS_FORMAT_STRING_OBJECT(3) : Error(inner_error, code)
    {
        String formatted;
        va_list args;
        va_start(args, fmt);
        formatted.FormatV(fmt, args);
        va_end(args);
        SetMessage(CombineCodeText(code, formatted));
    }

    CodeType Code() const { return (CodeType)Error::Code(); }
private:
    static String CombineCodeText(CodeType code, const String& more_text)
    {
        String text = GetErrorText(code);
        AppendLine(text, more_text);
        return text;
    }
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_UTIL__ERROR_H
