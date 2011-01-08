using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types.Interfaces
{
    public interface IScript
    {
        string FileName { get; }
        string Text { get; }
        ScriptAutoCompleteData AutoCompleteData { get; }
    }
}
