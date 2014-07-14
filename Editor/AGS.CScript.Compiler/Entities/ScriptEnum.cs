using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class ScriptEnum
    {
        public string Name;
        public Dictionary<string, int> Values = new Dictionary<string, int>();

        public ScriptEnum(string name)
        {
            Name = name;
        }
    }
}
