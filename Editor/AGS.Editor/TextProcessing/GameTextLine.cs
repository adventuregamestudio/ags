using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
	public struct GameTextLine
	{
		public string Text;
        public int CharacterID;
        public string SourceRef;

        public GameTextLine(string text)
        {
            Text = text;
            CharacterID = -1;
            SourceRef = string.Empty;
        }

        public GameTextLine(string text, string sourceRef)
        {
            Text = text;
            CharacterID = -1;
            SourceRef = sourceRef;
        }

        public GameTextLine(int characterID, string text)
		{
            Text = text;
            CharacterID = characterID;
            SourceRef = string.Empty;
        }

        public GameTextLine(int characterID, string text, string sourceRef)
        {
            Text = text;
            CharacterID = characterID;
            SourceRef = sourceRef;
        }
    }
}
