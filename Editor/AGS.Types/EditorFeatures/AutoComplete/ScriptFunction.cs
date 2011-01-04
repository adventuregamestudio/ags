using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types.AutoComplete
{
    public class ScriptFunction : ScriptToken, IComparable<ScriptFunction>
    {
        public ScriptFunction(string functionName, string type, string paramList, string ifDefOnly, string ifNDefOnly, bool returnsPointer, bool isStatic, bool isStaticOnly, bool noInherit, bool isProtected, bool isExtender, int scriptCharacterIndex)
        {
            FunctionName = functionName;
            ParamList = paramList;
            Type = type;
            IfDefOnly = ifDefOnly;
            IfNDefOnly = ifNDefOnly;
            ReturnsPointer = returnsPointer;
            IsStatic = isStatic;
            IsStaticOnly = isStaticOnly;
            NoInherit = noInherit;
            IsProtected = isProtected;
            IsExtenderMethod = isExtender;
            StartsAtCharacterIndex = scriptCharacterIndex;
            EndsAtCharacterIndex = 0;
        }

        public string FunctionName;
        public string ParamList;
        public string Type;
        public bool ReturnsPointer;
        public bool IsStatic;
        public bool IsStaticOnly;
        public bool NoInherit;
        public bool IsProtected;
        public bool IsExtenderMethod;
        public int EndsAtCharacterIndex;

        /// <summary>
        /// If this function should be hidden from the main autocomplete list.
        /// This applies to member function definitions, since the import statement
        /// will be used instead.
        /// </summary>
        public bool HideOnMainFunctionList
        {
            get { return FunctionName.IndexOf(":") >= 0; }
        }

        public override string ToString()
        {
            return "FUNC: " + (IsStatic ? "static " : "") + Type + (ReturnsPointer ? "*" : "") + " " + FunctionName + "(" + ParamList + ")";
        }

        public int CompareTo(ScriptFunction other)
        {
            return FunctionName.CompareTo(other.FunctionName);
        }
    }
}
