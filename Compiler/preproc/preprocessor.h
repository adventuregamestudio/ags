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
#include <stack>
#include "preproc/cc_macrotable.h"
#include "util/string.h"
#include "util/version.h"

using namespace AGS::Common;

namespace AGS {
namespace Preprocessor {

    enum class ErrorCode {
        None = 0,
        MacroNameMissing = 1,
        MacroDoesNotExist,
        MacroAlreadyExists,
        MacroNameInvalid,
        UnknownPreprocessorDirective,
        UserDefinedError,
        EndIfWithoutIf,
        IfWithoutEndIf,
        ElseIfWithoutIf,
        InvalidVersionNumber,
        UnterminatedString,
        InvalidCharacter
    };

    struct Error {
        ErrorCode Type = ErrorCode::None;
        String Message = nullptr;
    };

    class Preprocessor {
    private:
        bool _inMultiLineComment = false;
        MacroTable _macros = MacroTable();
        int _lineNumber;
        String _scriptName;
        Version _applicationVersion;
        // Conditional statement stack remembers the results of all the nested conditions
        // that we have entered.
        std::stack<bool> _conditionalStatements;
        // Negative counter: it is incremented each time we enter a FALSE condition,
        // and decremented each time we exit a previous FALSE condition.
        uint32_t _negativeCounter = 0u;
        std::vector<Error> _errors;

        void LogError(ErrorCode error, const String &message = nullptr);

        void ProcessConditionalDirective(String &directive, String &line);

        bool DeletingCurrentLine();

        String GetNextWord(String &text, bool trimText = true, bool includeDots = false);

        String RemoveComments(String text);

        String PreProcessDirective(String &line);

        String PreProcessLine(const String& lineToProcess);

    public:
        static const Error NoError;
        Error GetLastError() const;
        void SetAppVersion(const String& version);
        void MergeMacros(MacroTable &macros);
        void DefineMacro(const String &name, const String &value);

        String Preprocess(const String &script, const String &scriptName);
    };

} // Preprocessor
} // AGS
