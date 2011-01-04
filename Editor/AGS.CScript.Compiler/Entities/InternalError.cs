using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    public class InternalError : CompilerMessage
    {
        internal InternalError(string message) : base(ErrorCode.InternalError, "Internal compiler error: " + message)
        {
        }
    }
}
