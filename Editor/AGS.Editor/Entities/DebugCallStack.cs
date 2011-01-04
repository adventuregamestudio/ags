using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class DebugCallStack
    {
        private List<CallStackLine> _lines = new List<CallStackLine>();
        private string _errorMessage = null;

        internal DebugCallStack(string errorMessage)
        {
            _errorMessage = errorMessage;
        }

        public List<CallStackLine> Lines
        {
            get { return _lines; }
        }

        public string ErrorMessage
        {
            get { return _errorMessage; }
        }

        public void AddLine(string scriptName, int lineNumber)
        {
            _lines.Add(new CallStackLine(scriptName, lineNumber));
        }

    }

    public class CallStackLine
    {
        public string ScriptName;
        public int LineNumber;

        internal CallStackLine(string scriptName, int lineNumber)
        {
            this.ScriptName = scriptName;
            this.LineNumber = lineNumber;
        }
    }
}
