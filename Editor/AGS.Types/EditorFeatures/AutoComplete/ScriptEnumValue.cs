using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types.AutoComplete
{
    public class ScriptEnumValue : ScriptToken
    {
        public ScriptEnumValue(string name, string ifDefOnly, string ifNDefOnly, int scriptCharacterIndex)
        {
            Name = name;
            IfDefOnly = ifDefOnly;
            IfNDefOnly = ifNDefOnly;
            StartsAtCharacterIndex = scriptCharacterIndex;
        }

        public string Name;
    }
}
