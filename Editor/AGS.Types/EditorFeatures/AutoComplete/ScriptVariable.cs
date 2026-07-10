using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types.AutoComplete
{
    public class ScriptVariable : ScriptToken
    {
        public ScriptVariable(string variableName, string type, bool isArray, bool isDynamicArray, bool isPointer, int arrayDimensions,
            bool isAttribute, bool isIndexedAttribute,
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
            IsAttribute = isAttribute;
            IsIndexedAttribute = isIndexedAttribute;
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
        public bool IsAttribute;
        public bool IsIndexedAttribute;

        public override string ToString()
        {
            string str = $"VAR: {(IsStatic ? "static " : "")}{(IsReadOnly ? "readonly " : "")}{(IsAttribute ? "attribute " : "")}{Type}{(IsPointer && !IsDynamicArray ? "*" : "")} {VariableName}{(IsArray && !IsDynamicArray || IsIndexedAttribute ? "[]" : "")}";
            if (!IsDynamicArray && ArrayDimensions > 1)
            {
                for (int i = 1; i < ArrayDimensions; ++i)
                    str = str + "[]";
            }
            return str;
        }
    }
}
