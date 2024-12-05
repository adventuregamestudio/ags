using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// CompileWarningWithFunction - stores a function name in addition to regular parameters.
    /// </summary>
    public class CompileWarningWithFunction : CompileWarning
    {
        private string _funcName = string.Empty;

        public CompileWarningWithFunction(string message, string scriptName, string funcName, int lineNumber)
			: base(message, scriptName, lineNumber)
		{
            _funcName = funcName;
        }

        public string FunctionName
        {
            get { return _funcName; }
        }
    }
}
