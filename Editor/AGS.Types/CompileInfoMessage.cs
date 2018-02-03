namespace AGS.Types
{
    public class CompileInfoMessage : ICompileMessage
    {
        public CompileInfoMessage(string message, string scriptName, int lineNumber)
        {
            Message = message;
            ScriptName = scriptName;
            LineNumber = lineNumber;
            ShowIcon = true;
        }

        public CompileInfoMessage(string message) :
            this(message, string.Empty, 0)
        {
        }

        public CompileInfoMessage(CompileInfoMessage other) :
            this(other.Message, other.ScriptName, other.LineNumber)
        {
        }

        public string Message
        {
            get;
            private set;
        }

        public string ScriptName
        {
            get;
            private set;
        }

        public int LineNumber
        {
            get;
            private set;
        }

        public bool ShowIcon
        {
            get;
            set;
        }
    }
}
