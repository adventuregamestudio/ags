using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
	public struct GameTextLine
	{
		public string Text;
        public string ScriptFileName;
        public int CharacterID;
        public int ParserWordID;
        public string Context;
		public string ContextComment;
        public int SrcLine;

        public GameTextLine(string text, int srcline = -1)
        {
            Text = text;
            ScriptFileName = string.Empty;
            CharacterID = -1;
            ParserWordID = -1;
            Context = string.Empty;
            ContextComment = string.Empty;
            SrcLine = srcline;
        }

        public GameTextLine(string text, string context, int srcline = -1)
        {
            Text = text;
            ScriptFileName = string.Empty;
            CharacterID = -1;
            ParserWordID = -1;
            Context = context;
            ContextComment = string.Empty;
            SrcLine = srcline;
        }

        public GameTextLine(string text, string context, string contextComment, int srcline = -1)
        {
            Text = text;
            ScriptFileName = string.Empty;
            CharacterID = -1;
            ParserWordID = -1;
            Context = context;
            ContextComment = contextComment;
            SrcLine = srcline;
        }

        public static GameTextLine MakeScript(string text, string scriptName)
        {
            var line = new GameTextLine(text, scriptName);
            line.ScriptFileName = scriptName;
            return line;
        }

        public static GameTextLine MakeScript(string text, string scriptName, string context)
        {
            var line = new GameTextLine(text, context);
            line.ScriptFileName = scriptName;
            return line;
        }

        public static GameTextLine MakeScript(string text, string scriptName, string context, string contextComment)
        {
            var line = new GameTextLine(text, context, contextComment);
            line.ScriptFileName = scriptName;
            return line;
        }

        public static GameTextLine MakeSpeechLine(int characterID, string text, int srcline = -1)
        {
            var line = new GameTextLine(text, string.Empty, srcline);
            line.CharacterID = characterID;
            return line;
        }

        public static GameTextLine MakeSpeechLine(int characterID, string text, string context, int srcline = -1)
        {
            var line = new GameTextLine(text, context, string.Empty, srcline);
            line.CharacterID = characterID;
            return line;
        }

        public static GameTextLine MakeSpeechLine(int characterID, string text, string context, string contextComment, int srcline = -1)
        {
            var line = new GameTextLine(text, context, contextComment, srcline);
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
