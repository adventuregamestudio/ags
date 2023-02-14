using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public abstract class CompileMessage : ApplicationException
    {
        private int _id = 0;
        private string _scriptName = string.Empty;
        private int _lineNumber = 0;

        public CompileMessage(int id, string message, string scriptName, int lineNumber)
            : base(message)
        {
            _id = id;
            _scriptName = scriptName;
            _lineNumber = lineNumber;
        }

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

        public int Index
        {
            get { return _id; }
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
