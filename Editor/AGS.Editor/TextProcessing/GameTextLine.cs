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
        public string Context;
		public string ContextComment;

        public GameTextLine(string text)
        {
            Text = text;
            CharacterID = -1;
            Context = string.Empty;
            ContextComment = string.Empty;
        }

        public GameTextLine(string text, string context, string contextComment)
        {
            Text = text;
            CharacterID = -1;
            Context = context;
            ContextComment = contextComment;
        }

        public GameTextLine(int characterID, string text)
		{
            Text = text;
            CharacterID = characterID;
			Context = string.Empty;
            ContextComment = string.Empty;
        }

        public GameTextLine(int characterID, string text, string context)
        {
            Text = text;
            CharacterID = characterID;
            Context = context;
            ContextComment = string.Empty;
        }

        public GameTextLine(int characterID, string text, string context, string contextComment)
        {
            Text = text;
            CharacterID = characterID;
            Context = context;
            ContextComment = contextComment;
        }
    }
}
