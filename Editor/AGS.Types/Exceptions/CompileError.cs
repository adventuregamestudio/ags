using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public class CompileError : CompileMessage
    {
        public CompileError(string message, string scriptName, int lineNumber)
            : base(message, scriptName, lineNumber)
        {
        }

        public CompileError(string message)
            : base(message)
        {
        }

        public CompileError(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        // Copy constructor required by BusyDialog
        public CompileError(CompileError cloneFrom)
            : base(cloneFrom)
        {
        }
    }
}
