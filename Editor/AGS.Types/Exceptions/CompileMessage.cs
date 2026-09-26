using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    /// <summary>
    /// A helper class that defines the icon used along with the CompileMessage.
    /// Contains icon identification (currently - string).
    /// </summary>
    public struct CompileMessageIcon
    {
        public string IconName { get; private set; }

        public CompileMessageIcon(string iconName)
        {
            IconName = iconName;
        }
    }

    public abstract class CompileMessage : ApplicationException
    {
        private string _scriptName = string.Empty;
        private int _lineNumber = 0;
        private CompileMessageIcon _icon = new CompileMessageIcon(string.Empty);

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

        public CompileMessage(string message, CompileMessageIcon icon)
            : base(message)
        {
            _icon = icon;
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

        public CompileMessageIcon Icon
        {
            get { return _icon; }
        }

        public string AsString
        {
            get
            {
                string fullMessage;
                if(_lineNumber == 0 && string.IsNullOrEmpty(_scriptName))
                {
                    fullMessage = Message;
                }
                else
                {
                    fullMessage = $"{Message} at line {_lineNumber} in {_scriptName}.";
                }

                for (Exception innerException = InnerException; innerException != null; innerException = innerException.InnerException)
                {
                    fullMessage += $"{Environment.NewLine}{Environment.NewLine}{innerException.Message}";
                }
                return fullMessage;
            }
        }
    }
}
