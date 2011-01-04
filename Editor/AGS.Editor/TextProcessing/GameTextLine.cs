using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
	public struct GameTextLine
	{
		public int CharacterID;
		public string Text;

		public GameTextLine(int characterID, string text)
		{
			CharacterID = characterID;
			Text = text;
		}
	}
}
