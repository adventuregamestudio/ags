using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public interface IGameTextProcessor
    {
        string ProcessText(string text, GameTextType textType);
        string ProcessText(string text, GameTextType textType, int characterID);
        // Whether the client needs to use the return value from ProcessText
        bool MakesChanges { get; } 
    }
}
