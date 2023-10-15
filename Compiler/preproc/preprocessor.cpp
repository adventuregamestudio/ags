#include <algorithm>
#include "preproc/preprocessor.h"
#include "script/cc_common.h"
#include "util/textstreamwriter.h"
#include "util/textreader.h"

#define STRINGIFY2(X) #X
#define STRINGIFY(X) STRINGIFY2(X)

using namespace AGS::Common;

extern int currentline; // in script/script_common

namespace AGS {
namespace Preprocessor {

    static const size_t NOT_FOUND = -1;

#if AGS_PLATFORM_OS_WINDOWS
    static const char * li_end = "\r\n";
#else
    static const char * li_end = "\n";
#endif

    class StringBuilder : TextWriter {
    private:
        String _str = nullptr;
    public:
        explicit StringBuilder(size_t size) {
            _str.Reserve(size);
        }

        void WriteLine(const String& str) override { _str.Append(str); _str.Append(li_end); }
        bool IsValid() const override { return true; }
        void WriteChar(char c) override {  _str.AppendChar(c); }
        void WriteString(const String &str) override { _str.Append(str);}
        void WriteFormat(const char *fmt, ...) override { va_list argptr; va_start(argptr, fmt); _str.Append(String::FromFormatV(fmt, argptr)); va_end(argptr);}
        void WriteLineBreak() override { _str.Append(li_end); }

        String GetString () {return _str;}
    };

    class StringReader : TextReader{
    private:
        String _str = nullptr;
        size_t _idx = 0;
    public:
        StringReader(const String& str) {
            _str = str;
            _idx = 0;
        }

        bool IsValid() const override { return _idx < _str.GetLength(); }
        char ReadChar() override { return _str.GetAt(_idx++); }
        String ReadString(size_t length) override { String res = _str.Mid(_idx, length); _idx+=res.GetLength(); return res;}
        String ReadLine() override {
            if(_idx >= _str.GetLength()) return nullptr;

            size_t newPos = _str.FindChar('\n', _idx);
            if(newPos != NOT_FOUND) {
                size_t length = newPos - _idx;
                size_t idx = _idx;
                _idx = newPos+1;
                if (length == 0) return String("");
                return _str.Mid(idx,length);
            } else {
                return ReadAll();
            }
        }
        String ReadAll() override { size_t idx = _idx; _idx = _str.GetLength(); return _str.Mid(idx, _str.GetLength() - idx); }
    };

    static bool Contains(const std::vector<String> &v, const String &s) {
        return std::find(v.rbegin(), v.rend(), s) != v.rend();
    }


    void Preprocessor::LogError(ErrorCode error, const String& message) {
        if(message == nullptr) {
            switch (error) {
                case ErrorCode::MacroNameMissing:
                    cc_error("Macro name expected");
                    break;
                case ErrorCode::EndIfWithoutIf:
                    cc_error("#endif has no matching #if");
                    break;
                case ErrorCode::IfWithoutEndIf:
                    cc_error("Missing #endif");
                    break;
                case ErrorCode::ElseIfWithoutIf:
                    cc_error("#else has no matching #if");
                    break;
                case ErrorCode::MacroDoesNotExist:
                case ErrorCode::MacroAlreadyExists:
                case ErrorCode::MacroNameInvalid:
                case ErrorCode::UnknownPreprocessorDirective:
                case ErrorCode::UserDefinedError:
                case ErrorCode::InvalidVersionNumber:
                    break;
                default:
                    cc_error("Unknown preprocessor error"); // should never be reached
                    break;
            }
        } else {
            cc_error(message.GetCStr());
        }
    }

    void Preprocessor::ProcessConditionalDirective(String &directive, String &line)
    {
        String macroName = GetNextWord(line, true, true);
        if (macroName.GetLength() == 0)
        {
            LogError(ErrorCode::MacroNameMissing, String::FromFormat("Expected something after '%s'", directive.GetCStr()));
            return;
        }

        bool includeCodeBlock = true;

        if ((!_conditionalStatements.empty()) && !_conditionalStatements.top())
        {
            includeCodeBlock = false;
        }
        else if (directive.CompareRight("def") == 0)
        {
            includeCodeBlock = _macros.contains(macroName.GetCStr());
            if (directive == "ifndef")
            {
                includeCodeBlock = !includeCodeBlock;
            }
        }
        else if (directive == "ifver" || directive == "ifnver")
        {
            // Compare provided version number with the current application version
            Version macroVersion = Version(macroName);
            if(macroVersion.Major == 0) {
                LogError(ErrorCode::InvalidVersionNumber, String::FromFormat("Cannot parse version number: %s", macroName.GetCStr()));
            }
            includeCodeBlock = _applicationVersion.AsLongNumber() >= macroVersion.AsLongNumber();
            if(directive == "ifnver" )
            {
                includeCodeBlock = !includeCodeBlock;
            }
        }

        _conditionalStatements.push(includeCodeBlock);
    }

    bool Preprocessor::DeletingCurrentLine()
    {
        return ((!_conditionalStatements.empty()) && !_conditionalStatements.top());
    }

    String Preprocessor::GetNextWord(String &text, bool trimText, bool includeDots) {
        size_t i = 0;
        while ((i < text.GetLength()) &&
               (is_alphanum(text[i]) ||
                (includeDots && (text[i] == '.')))
                ) {
            i++;
        }
        String word = text.Left(i);
        text.ClipLeft(i);
        if (trimText) {
            text.Trim();
        }
        return word;
    }

    String Preprocessor::RemoveComments(String text)
    {
        if (_inMultiLineComment)
        {
            size_t commentEnd = text.FindString("*/",0);
            if (commentEnd == NOT_FOUND)
            {
                return String("");
            }
            text = text.Mid(commentEnd + 2, text.GetLength() - (commentEnd + 2));
            _inMultiLineComment = false;
        }

        StringBuilder output = StringBuilder(text.GetLength());
        for (size_t i = 0; i < text.GetLength(); i++)
        {
            if (!_inMultiLineComment)
            {
                if ((text[i] == '"') || (text[i] == '\''))
                {
                    size_t endOfString = text.FindChar(text[i],i);
                    if (endOfString == NOT_FOUND) //size_t is unsigned but it's alright
                    {
                        LogError(ErrorCode::UnterminatedString, "Unterminated string");
                        break;
                    }
                    endOfString++;
                    output.WriteString(text.Mid(i, endOfString - i));
                    text = text.Mid(endOfString, text.GetLength() - endOfString);
                    i = -1;
                }
                else if ((i < text.GetLength() - 1) && (text[i] == '/') && (text[i + 1] == '/'))
                {
                    break;
                }
                else if ((i < text.GetLength() - 1) && (text[i] == '/') && (text[i + 1] == '*'))
                {
                    _inMultiLineComment = true;
                    i++;
                }
                else
                {
                    output.WriteChar(text[i]);
                }
            }
            else if ((i < text.GetLength() - 1) && (text[i] == '*') && (text[i + 1] == '/'))
            {
                _inMultiLineComment = false;
                i++;
            }
        }

        String out = output.GetString();
        out.Trim();
        return out;
    }

    String Preprocessor::PreProcessDirective(String &line)
    {
        line.ClipLeft(1);
        String directive = GetNextWord(line);

        if ((directive == "ifdef") || (directive == "ifndef") ||
            (directive == "ifver") || (directive == "ifnver"))
        {
            ProcessConditionalDirective(directive, line);
        }
        else if (directive == "else")
        {
            if (!_conditionalStatements.empty())
            {
                // Negate previous condition
                bool prev_value = _conditionalStatements.top();
                _conditionalStatements.pop();
                _conditionalStatements.push(!prev_value);
            }
            else
            {
                LogError(ErrorCode::ElseIfWithoutIf);
            }
        }
        else if (directive == "endif")
        {
            if (!_conditionalStatements.empty())
            {
                _conditionalStatements.pop();
            }
            else
            {
                LogError(ErrorCode::EndIfWithoutIf);
            }
        }
        else if (DeletingCurrentLine())
        {
            // allow the line to be deleted, we are inside a failed #ifdef
        }
        else if (directive == "define")
        {
            String macroName = GetNextWord(line);
            if (macroName.GetLength() == 0)
            {
                LogError(ErrorCode::MacroNameMissing);
                return String("");
            }
            else if (is_digit(macroName[0]))
            {
                LogError(ErrorCode::MacroNameInvalid, String::FromFormat("Macro name '%s' cannot start with a digit", macroName.GetCStr()));
            }
            else if (_macros.contains(macroName.GetCStr()))
            {
                LogError(ErrorCode::MacroAlreadyExists, String::FromFormat("Macro '%s' is already defined", macroName.GetCStr()));
            }
            else
            {
                _macros.add(macroName, line);
            }
        }
        else if (directive == "undef")
        {
            String macroName = GetNextWord(line);
            if (macroName.GetLength() == 0)
            {
                LogError(ErrorCode::MacroNameMissing);
            }
            else if (!_macros.contains(macroName))
            {
                LogError(ErrorCode::MacroDoesNotExist, String::FromFormat("Macro '%s' is not defined", macroName.GetCStr()));
            }
            else
            {
                _macros.remove(macroName);
            }
        }
        else if (directive == "error")
        {
            LogError(ErrorCode::UserDefinedError, String::FromFormat("User error: %s", line.GetCStr()));
        }
        else if ((directive == "sectionstart") || (directive == "sectionend"))
        {
            // do nothing -- 2.72 put these as markers in the script
        }
        else if ((directive == "region") || (directive == "endregion"))
        {
            // do nothing -- scintilla can fold it, so it can be used to organize the code
        }
        else
        {
            LogError(ErrorCode::UnknownPreprocessorDirective, String::FromFormat("Unknown preprocessor directive '%s'", directive.GetCStr()));
        }

        return String("");  // replace the directive with a blank line
    }

    void Preprocessor::DefineMacro(const String& name, const String& value)
    {
        _macros.add(name, value);
    }

    String Preprocessor::PreProcessLine(const String& lineToProcess)
    {
        if (DeletingCurrentLine())
        {
            return String("");
        }

        std::vector<StringBuilder> previousOutput = std::vector<StringBuilder>();
        std::vector<String> previousLine = std::vector<String>();
        StringBuilder output = StringBuilder(lineToProcess.GetLength());
        String line = lineToProcess;
        std::vector<String> ignored = std::vector<String>();
        while (line.GetLength() > 0)
        {
            size_t i = 0;
            while ((i < line.GetLength()) && (!is_alphanum(line[i])))
            {
                if ((line[i] == '"') || (line[i] == '\''))
                {
                    i = line.FindChar(line[i],i);
                    if (i == NOT_FOUND)
                    {
                        i = line.GetLength();
                        break;
                    }
                }
                i++;
            }

            output.WriteString(line.Mid(0, i));

            if (i < line.GetLength())
            {
                bool precededByDot = false;
                if (i > 0) precededByDot = (line[i - 1] == '.');

                line = line.Mid(i, line.GetLength() - i);

                String theWord = GetNextWord(line, false, false);

                if ((!precededByDot) && (!Contains(ignored, theWord)) && (_macros.contains(theWord.GetCStr())))
                {
                    previousOutput.push_back(output);
                    previousLine.push_back(line);
                    ignored.push_back(theWord);
                    line = _macros.get_macro(theWord.GetCStr());
                    output = StringBuilder(line.GetLength());
                }
                else
                {
                    output.WriteString(theWord);
                }
            }
            else
                line = "";

            while (line.GetLength() == 0 && !previousOutput.empty())
            {
                String result = output.GetString();
                output = previousOutput.back();
                previousOutput.pop_back();
                line = previousLine.back();
                previousLine.pop_back();
                ignored.pop_back();
                output.WriteString(result);
            }

        }

        return output.GetString();
    }


    String Preprocessor::Preprocess(const String& script, const String& scriptName)
    {
        StringBuilder output = StringBuilder(script.GetLength());
        currentline = _lineNumber = 0;
        output.WriteLine(String::FromFormat("%s%s\"", NEW_SCRIPT_TOKEN_PREFIX, scriptName.GetCStr()));
        StringReader reader = StringReader(script);
        _scriptName = scriptName;
        while (reader.IsValid())
        {
            currentline = ++_lineNumber;
            String thisLine = reader.ReadLine();
            thisLine = RemoveComments(thisLine);
            if (thisLine.GetLength() > 0)
            {
                if (thisLine[0] != '#')
                {
                    thisLine = PreProcessLine(thisLine);
                }
                else
                {
                    thisLine = PreProcessDirective(thisLine);
                }
            }
            output.WriteLine(thisLine);
        }


        if (!_conditionalStatements.empty()) // count > 0
        {
            LogError(ErrorCode::IfWithoutEndIf);
        }

        return output.GetString();
    }

    void Preprocessor::MergeMacros(MacroTable &macros) {
        _macros.merge(macros);
    }

    void Preprocessor::SetAppVersion(const String &version) {
        _applicationVersion = Version(version);
    }

    } // Preprocessor
} // AGS