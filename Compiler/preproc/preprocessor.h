#include "script/cs_parser_common.h"
#include "script/cc_macrotable.h"
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
        LineTooLong,
        UnknownPreprocessorDirective,
        UserDefinedError,
        EndIfWithoutIf,
        IfWithoutEndIf,
        InvalidVersionNumber,
    };

    class Preprocessor {
    private:
        bool _inMultiLineComment = false;
        MacroTable _macros = MacroTable();
        int _lineNumber;
        String _scriptName;
        Version _applicationVersion;
        std::vector<bool> _conditionalStatements = std::vector<bool>();

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