using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public abstract class CompileMessage : ApplicationException
    {
        private string _scriptName;
        private int _lineNumber;

        public CompileMessage(string message, string scriptName, int lineNumber)
            : base(message)
        {
            _scriptName = scriptName;
            _lineNumber = lineNumber;
        }

        public CompileMessage(string message)
            : base(message)
        {
            _scriptName = string.Empty;
            _lineNumber = 0;
        }

        public CompileMessage(string message, Exception innerException)
            : base(message, innerException)
        {
            _scriptName = string.Empty;
            _lineNumber = 0;
        }

        public CompileMessage(CompileMessage other) : base(other.Message)
        {
            _lineNumber = other.LineNumber;
            _scriptName = other.ScriptName;
        }

        public string ScriptName
        {
            get { return _scriptName; }
        }

        public int LineNumber
        {
            get { return _lineNumber; }
        }

    }
}
