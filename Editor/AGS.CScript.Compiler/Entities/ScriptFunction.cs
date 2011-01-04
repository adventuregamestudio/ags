using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class ScriptFunction
    {
        public Token ReturnType;
        public Token Name;
        public List<FunctionParameter> Parameters;
        public bool? IsPrototypeOnly = null;
        public bool VariableArguments = false;

        public ScriptFunction(Token returnType, Token name)
        {
            ReturnType = returnType;
            Name = name;
            Parameters = new List<FunctionParameter>();
        }
    }
}
