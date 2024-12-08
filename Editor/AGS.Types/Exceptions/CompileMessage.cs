using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public abstract class CompileMessage : ApplicationException
    {
        private string _scriptName = string.Empty;
        private int _lineNumber = 0;

        public CompileMessage(string message, string scriptName, int lineNumber)
            : base(message)
        {
            _scriptName = scriptName;
            _lineNumber = lineNumber;
        }

        public CompileMessage(string message)
            : base(message)
        {
        }

        public CompileMessage(string message, Exception innerException)
            : base(message, innerException)
        {
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

        public string AsString
        {
            get
            {
                if(_lineNumber == 0 && string.IsNullOrEmpty(_scriptName))
                {
                    return Message;
                }
                else
                {
                    return Message + " at line " + _lineNumber.ToString() + " in " + _scriptName + ".";
                }
            }
        }
    }
}
