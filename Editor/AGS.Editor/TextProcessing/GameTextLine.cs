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
        public string SourceRef;

        public GameTextLine(string text)
        {
            Text = text;
            CharacterID = -1;
            ParserWordID = -1;
            SourceRef = string.Empty;
        }

        public GameTextLine(string text, string sourceRef)
        {
            Text = text;
            CharacterID = -1;
            ParserWordID = -1;
            SourceRef = sourceRef;
        }

        public static GameTextLine MakeSpeechLine(int characterID, string text)
        {
            var line = new GameTextLine(text);
            line.CharacterID = characterID;
            return line;
        }
        public static GameTextLine MakeSpeechLine(int characterID, string text, string sourceRef)
        {
            var line = new GameTextLine(text, sourceRef);
            line.CharacterID = characterID;
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
