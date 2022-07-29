using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types.AutoComplete
{
    public class ScriptDefine : ScriptToken
    {
        public ScriptDefine(string name, string ifDefOnly, string ifNDefOnly, int scriptCharacterIndex)
        {
            Name = name;
            IfDefOnly = ifDefOnly;
            IfNDefOnly = ifNDefOnly;
            StartsAtCharacterIndex = scriptCharacterIndex;
        }

        public string Name;
    }
}
