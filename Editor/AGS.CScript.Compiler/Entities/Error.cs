using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    public class Error : CompilerMessage
    {
        public Error(ErrorCode errorCode, string message, int lineNumber, string scriptName)
            : base(errorCode, message, lineNumber, scriptName)
        {
        }
    }
}
