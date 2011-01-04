using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class TextParser
    {
        public const int WORD_GROUP_ID_IGNORE = 0;
        public const int WORD_GROUP_ID_ANYWORD = 29999;
        public const int WORD_GROUP_ID_REST_OF_LINE = 30000;

        private List<TextParserWord> _words = new List<TextParserWord>();

        public TextParser()
        {
            _words.Add(new TextParserWord(0, "a", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "an", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "at", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "for", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "from", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "i", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "in", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "into", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "is", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "it", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "of", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "on", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "over", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "some", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "that", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "the", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "this", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "to", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(0, "under", TextParserWordType.Ignore));
            _words.Add(new TextParserWord(WORD_GROUP_ID_ANYWORD, "anyword", TextParserWordType.MatchAnyWord));
            _words.Add(new TextParserWord(WORD_GROUP_ID_REST_OF_LINE, "rol", TextParserWordType.MatchRestOfInput));
        }

        public List<TextParserWord> Words
        {
            get { return _words; }
        }

        public int CountWordsInWordGroup(int wordGroupID)
        {
            int count = 0;
            foreach (TextParserWord word in _words)
            {
                if (word.WordGroup == wordGroupID)
                {
                    count++;
                }
            }
            return count;
        }

        public int GetAvailableWordGroupID()
        {
            bool groupInUse;
            for (int i = 0; i < WORD_GROUP_ID_ANYWORD; i++)
            {
                groupInUse = false;
                foreach (TextParserWord word in _words)
                {
                    if (word.WordGroup == i)
                    {
                        groupInUse = true;
                        break;
                    }
                }
                if (!groupInUse)
                {
                    return i;
                }
            }
            throw new AGSEditorException("No free word groups found; already got 30000 words?");
        }

        public TextParser(XmlNode node)
        {
            foreach (XmlNode wordNode in SerializeUtils.GetChildNodes(node, "Words"))
            {
                _words.Add(new TextParserWord(wordNode));
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Words");
            foreach (TextParserWord word in _words)
            {
                word.ToXml(writer);
            }
            writer.WriteEndElement();
        }

    }
}
