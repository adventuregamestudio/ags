using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types.Interfaces;

namespace AGS.Types.AutoComplete
{
    public class ScriptTokenReference
    {
        public IScript Script { get; set; }
        public string CurrentLine { get; set; }
        public int LineIndex { get; set; }
        public int CharacterIndex { get; set; }
        public string Token { get; set; }
    }
}
