using System.Collections.Generic;

namespace AGS.Types.AutoComplete
{
    public class ScriptEnum : ScriptToken
    {
        public ScriptEnum(string name, string ifDefOnly, string ifNDefOnly)
        {
            Name = name;
            IfDefOnly = ifDefOnly;
            IfNDefOnly = ifNDefOnly;
            EnumValues = new List<string>();
        }

        public string Name;
        public List<string> EnumValues;
    }
}
