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
        public int ParserWordID;
        public string Context;
		public string ContextComment;

        public GameTextLine(string text)
        {
            Text = text;
            CharacterID = -1;
            ParserWordID = -1;
            Context = string.Empty;
            ContextComment = string.Empty;
        }

        public GameTextLine(string text, string context)
        {
            Text = text;
            CharacterID = -1;
            ParserWordID = -1;
            Context = context;
            ContextComment = string.Empty;
        }

        public GameTextLine(string text, string context, string contextComment)
        {
            Text = text;
            CharacterID = -1;
            ParserWordID = -1;
            Context = context;
            ContextComment = contextComment;
        }

        public static GameTextLine MakeSpeechLine(int characterID, string text)
        {
            var line = new GameTextLine(text);
            line.CharacterID = characterID;
            return line;
        }

        public static GameTextLine MakeSpeechLine(int characterID, string text, string context)
        {
            var line = new GameTextLine(text, context, string.Empty);
            line.CharacterID = characterID;
            return line;
        }

        public static GameTextLine MakeSpeechLine(int characterID, string text, string context, string contextComment)
        {
            var line = new GameTextLine(text, context, contextComment);
            line.CharacterID = characterID;
            return line;
        }

        public static GameTextLine MakeParserWord(int wordID, string text, string context)
        {
            var line = new GameTextLine(text, context, string.Empty);
            line.ParserWordID = wordID;
            return line;
        }
    }
}
