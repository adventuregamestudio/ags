using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types.AutoComplete
{
    public class ScriptStruct : ScriptToken
    {
        public ScriptStruct(string name, string ifDefOnly, string ifNDefOnly, int scriptCharacterIndex)
        {
            Name = name;
            IfDefOnly = ifDefOnly;
            IfNDefOnly = ifNDefOnly;
            FullDefinition = true;
			StartsAtCharacterIndex = scriptCharacterIndex;
        }

        public ScriptStruct(string name)
        {
            Name = name;
            FullDefinition = false;
        }

        public ScriptVariable FindMemberVariable(string name)
        {
            foreach (ScriptVariable var in Variables)
            {
                if (var.VariableName == name)
                {
                    return var;
                }
            }
            return null;
        }

        public ScriptFunction FindMemberFunction(string name)
        {
            foreach (ScriptFunction func in Functions)
            {
                if (func.FunctionName == name)
                {
                    return func;
                }
            }
            return null;
        }

        public string Name;
        public bool FullDefinition;
        public List<ScriptVariable> Variables = new List<ScriptVariable>();
        public List<ScriptFunction> Functions = new List<ScriptFunction>();

        public override string ToString()
        {
            return "STR: " + Name + "; " + Variables.Count + " vars, " + Functions.Count + " funcs";
        }
    }
}
