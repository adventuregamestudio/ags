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
        public string SourceRef;
        public int SrcLine;

        public GameTextLine(string text, int srcline = -1)
        {
            Text = text;
            ScriptFileName = string.Empty;
            CharacterID = -1;
            ParserWordID = -1;
            SourceRef = string.Empty;
            SrcLine = srcline;
        }

        public GameTextLine(string text, string sourceRef, int srcline = -1)
        {
            Text = text;
            ScriptFileName = string.Empty;
            CharacterID = -1;
            ParserWordID = -1;
            SourceRef = sourceRef;
            SrcLine = srcline;
        }

        public static GameTextLine MakeScript(string text, string scriptName, int srcline = -1)
        {
            var line = new GameTextLine(text, scriptName);
            line.ScriptFileName = scriptName;
            line.SrcLine = srcline;
            return line;
        }

        public static GameTextLine MakeScript(string text, string scriptName, string context, int srcline = -1)
        {
            var line = new GameTextLine(text, context);
            line.ScriptFileName = scriptName;
            line.SrcLine = srcline;
            return line;
        }

        public static GameTextLine MakeSpeechLine(int characterID, string text, int srcline = -1)
        {
            var line = new GameTextLine(text);
            line.CharacterID = characterID;
            line.SrcLine = srcline;
            return line;
        }
        public static GameTextLine MakeSpeechLine(int characterID, string text, string sourceRef, int srcline = -1)
        {
            var line = new GameTextLine(text, sourceRef);
            line.CharacterID = characterID;
            line.SrcLine = srcline;
            return line;
        }

        public static GameTextLine MakeParserWord(int wordID, string text, string sourceRef)
        {
            var line = new GameTextLine(text, sourceRef);
            line.ParserWordID = wordID;
            return line;
        }
    }
}
