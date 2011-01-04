using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class FunctionParameter
    {
        public Token Type;
        public Token Name;
        public int? DefaultValue = null;
        public Modifiers Modifiers = null;

        public FunctionParameter(Token type, Token name)
        {
            Type = type;
            Name = name;
        }

        public bool IsConst
        {
            get { return this.Modifiers.HasModifier(Modifiers.CONST); }
        }
    }
}
