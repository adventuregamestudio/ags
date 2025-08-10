using System;

namespace AGS.Types
{
    public class CompileInformation : CompileMessage
    {
        public CompileInformation(int id, string message, string scriptName, int lineNumber)
            : base(id, message, scriptName, lineNumber)
        {
        }

        public CompileInformation(string message, string scriptName, int lineNumber)
            : base(message, scriptName, lineNumber)
        {
        }

        public CompileInformation(string message)
            : base(message)
        {
        }

        public CompileInformation(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        // Copy constructor required by BusyDialog
        public CompileInformation(CompileInformation cloneFrom)
            : base(cloneFrom)
        {
        }
    }
}
