using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class ScriptVariable
    {
        public const int POINTER_SIZE_IN_BYTES = 4;
        public const int ENUM_SIZE_IN_BYTES = 4;

        public Token VariableTypeToken;
        public int Size;
        public int Offset;
        public bool IsPointer;
        public bool IsAccessed = false;

    }
}
