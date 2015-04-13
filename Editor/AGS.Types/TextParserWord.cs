using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    [DefaultProperty("Word")]
    public class TextParserWord
    {
        private int _wordGroup;
        private string _word = string.Empty;
        private TextParserWordType _type = TextParserWordType.Normal;

        public TextParserWord()
        {
        }

        public TextParserWord(int wordGroup, string word, TextParserWordType type)
        {
            _wordGroup = wordGroup;
            _word = word;
            _type = type;
        }

        [Description("The word group that this word belongs to")]
        [Category("Design")]
        [ReadOnly(true)]
        public int WordGroup
        {
            get { return _wordGroup; }
            set { _wordGroup = value; }
        }

        [Description("The word!")]
        [Category("Design")]
        public string Word
        {
            get { return _word; }
            set { _word = value.ToLower(); }
        }

        [Description("Which type of word is this?")]
        [Category("Design")]
        [ReadOnly(true)]
        public TextParserWordType Type
        {
            get { return _type; }
            set { _type = value; }
        }

        public void SetWordTypeFromGroup()
        {
            if (_wordGroup == TextParser.WORD_GROUP_ID_ANYWORD)
            {
                _type = TextParserWordType.MatchAnyWord;
            }
            else if (_wordGroup == TextParser.WORD_GROUP_ID_IGNORE)
            {
                _type = TextParserWordType.Ignore;
            }
            else if (_wordGroup == TextParser.WORD_GROUP_ID_REST_OF_LINE)
            {
                _type = TextParserWordType.MatchRestOfInput;
            }
            else
            {
                _type = TextParserWordType.Normal;
            }
        }

        public TextParserWord(XmlNode node)
        {
            SerializeUtils.DeserializeFromXML(this, node);
        }

        public void ToXml(XmlTextWriter writer)
        {
            SerializeUtils.SerializeToXML(this, writer);
        }

    }
}
