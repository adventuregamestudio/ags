using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class EndOfStreamToken : Token
    {
        public EndOfStreamToken()
            : base(null, true)
        {
        }
    }
}
