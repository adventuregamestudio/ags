using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types.AutoComplete
{
    public class ScriptVariable : ScriptToken
    {
        public ScriptVariable(string variableName, string type, bool isArray, bool isPointer, string ifDefOnly, string ifNDefOnly, bool isStatic, bool isStaticOnly, bool noInherit, bool isProtected, int scriptCharacterIndex)
        {
            VariableName = variableName;
            Type = type;
            IsArray = isArray;
            IsPointer = isPointer;
            IfDefOnly = ifDefOnly;
            IfNDefOnly = ifNDefOnly;
            IsStatic = isStatic;
            IsStaticOnly = isStaticOnly;
            NoInherit = noInherit;
            IsProtected = isProtected;
			StartsAtCharacterIndex = scriptCharacterIndex;
        }

        public string VariableName;
        public string Type;
        public bool IsArray;
        public bool IsPointer;
        public bool IsStatic;
        public bool IsStaticOnly;
        public bool NoInherit;
        public bool IsProtected;

        public override string ToString()
        {
            return "VAR: " + (IsStatic ? "static " : "") + Type + (IsPointer ? "*" : "") + (IsArray ? "[]" : "") + " " + VariableName;
        }
    }
}
