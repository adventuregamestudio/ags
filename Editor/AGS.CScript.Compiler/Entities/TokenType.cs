using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal enum TokenType
    {
        Unknown,
        Constant,
        EnumType,
        StructType,
        GlobalVariable,
        LocalVariable
    }
}
