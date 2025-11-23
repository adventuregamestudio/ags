using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types.AutoComplete
{
    public class ScriptVariable : ScriptToken
    {
        public ScriptVariable(string variableName, string type, bool isArray, bool isDynamicArray, bool isPointer, int arrayDimensions,
            string ifDefOnly, string ifNDefOnly, bool isStatic, bool isStaticOnly, bool noInherit, bool isProtected, bool isReadOnly, int scriptCharacterIndex)
        {
            VariableName = variableName;
            Type = type;
            IsArray = isArray;
            IsDynamicArray = isDynamicArray;
            IsPointer = isPointer;
            IfDefOnly = ifDefOnly;
            IfNDefOnly = ifNDefOnly;
            IsStatic = isStatic;
            IsStaticOnly = isStaticOnly;
            NoInherit = noInherit;
            IsProtected = isProtected;
            IsReadOnly = isReadOnly;
            ArrayDimensions = arrayDimensions;
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
        public bool IsDynamicArray;
        public bool IsReadOnly;
        public int ArrayDimensions;

        public override string ToString()
        {
            string str = $"VAR: {(IsStatic ? "static " : "")}{Type}{(IsPointer && !IsDynamicArray ? "*" : "")} {VariableName}{(IsArray && !IsDynamicArray ? "[]" : "")}";
            if (!IsDynamicArray && ArrayDimensions > 1)
            {
                for (int i = 1; i < ArrayDimensions; ++i)
                    str = str + "[]";
            }
            return str;
        }
    }
}
