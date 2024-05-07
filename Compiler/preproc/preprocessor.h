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
#include <stack>
#include "preproc/cc_macrotable.h"
#include "util/string.h"
#include "util/version.h"

using namespace AGS::Common;

namespace AGS {
namespace Preprocessor {

    enum class ErrorCode {
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
        UnterminatedString
    };

    class Preprocessor {
    private:
        bool _inMultiLineComment = false;
        MacroTable _macros = MacroTable();
        int _lineNumber;
        String _scriptName;
        Version _applicationVersion;
        std::stack<bool> _conditionalStatements;

        static void LogError(ErrorCode error, const String &message = nullptr);

        void ProcessConditionalDirective(String &directive, String &line);

        bool DeletingCurrentLine();

        static String GetNextWord(String &text, bool trimText = true, bool includeDots = false);

        String RemoveComments(String text);

        String PreProcessDirective(String &line);

        String PreProcessLine(const String& lineToProcess);

    public:
        void SetAppVersion(const String& version);
        void MergeMacros(MacroTable &macros);
        void DefineMacro(const String &name, const String &value);

        String Preprocess(const String &script, const String &scriptName);
    };

} // Preprocessor
} // AGS