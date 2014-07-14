using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class Translation
    {
        public const string TRANSLATION_SOURCE_FILE_EXTENSION = ".trs";
        public const string TRANSLATION_COMPILED_FILE_EXTENSION = ".tra";

        private const string NORMAL_FONT_TAG = "//#NormalFont=";
        private const string SPEECH_FONT_TAG = "//#SpeechFont=";
        private const string TEXT_DIRECTION_TAG = "//#TextDirection=";
        private const string TAG_DEFAULT = "DEFAULT";
        private const string TAG_DIRECTION_LEFT = "LEFT";
        private const string TAG_DIRECTION_RIGHT = "RIGHT";

        private string _name;
        private string _fileName;
        private bool _modified;
        private int? _normalFont;
        private int? _speechFont;
        private bool? _rightToLeftText;
        private Dictionary<string, string> _translatedLines;

        public Translation(string name)
        {
            this.Name = name;
            _modified = false;
            _normalFont = null;
            _speechFont = null;
            _rightToLeftText = null;
        }

        public string Name
        {
            get { return _name; }
            set { _name = value; _fileName = _name + TRANSLATION_SOURCE_FILE_EXTENSION; }
        }

        public string FileName
        {
            get { return _fileName; }
        }

        public string CompiledFileName
        {
            get { return _name + TRANSLATION_COMPILED_FILE_EXTENSION; }
        }

        public Dictionary<string, string> TranslatedLines
        {
            get { return _translatedLines; }
            set { _translatedLines = value; }
        }

        public int? NormalFont
        {
            get { return _normalFont; }
        }

        public int? SpeechFont
        {
            get { return _speechFont; }
        }

        public bool? RightToLeftText
        {
            get { return _rightToLeftText; }
        }

        public bool Modified
        {
            get { return _modified; }
            set { _modified = value; }
        }

        public Translation(XmlNode node)
        {
            this.Name = SerializeUtils.GetElementString(node, "Name");
            LoadData();
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Translation");
            writer.WriteElementString("Name", _name);
            writer.WriteEndElement();
        }

        public void SaveData()
        {
            using (StreamWriter sw = new StreamWriter(FileName, false, Encoding.Default))
            {
                sw.WriteLine("// AGS TRANSLATION SOURCE FILE");
                sw.WriteLine("// Format is alternating lines with original game text and replacement");
                sw.WriteLine("// text. If you don't want to translate a line, just leave the following");
                sw.WriteLine("// line blank. Lines starting with '//' are comments - DO NOT translate");
                sw.WriteLine("// them. Special characters such as [ and %%s symbolise things within the");
                sw.WriteLine("// game, so should be left in an appropriate place in the message.");
                sw.WriteLine("// ");
                sw.WriteLine("// ** Translation settings are below");
                sw.WriteLine("// ** Leave them as \"DEFAULT\" to use the game settings");
                sw.WriteLine("// The normal font to use - DEFAULT or font number");
                sw.WriteLine("//#NormalFont=" + WriteOptionalInt(_normalFont));
                sw.WriteLine("// The speech font to use - DEFAULT or font number");
                sw.WriteLine("//#SpeechFont=" + WriteOptionalInt(_speechFont));
                sw.WriteLine("// Text direction - DEFAULT, LEFT or RIGHT");
                sw.WriteLine("//#TextDirection=" + ((_rightToLeftText == true) ? TAG_DIRECTION_RIGHT : ((_rightToLeftText == null) ? TAG_DEFAULT : TAG_DIRECTION_LEFT)));
                sw.WriteLine("//  ");
                sw.WriteLine("// ** REMEMBER, WRITE YOUR TRANSLATION IN THE EMPTY LINES, DO");
                sw.WriteLine("// ** NOT CHANGE THE EXISTING TEXT.");

                foreach (string key in _translatedLines.Keys)
                {
                    sw.WriteLine(key);
                    sw.WriteLine(_translatedLines[key]);
                }
            }
            this.Modified = false;
        }

        public void LoadData()
        {
            _translatedLines = new Dictionary<string, string>();

            using (StreamReader sr = new StreamReader(FileName, Encoding.Default))
            {
                string line;
                while ((line = sr.ReadLine()) != null)
                {
                    if (line.StartsWith("//"))
                    {
                        ReadSpecialTags(line);
                        continue;
                    }
                    string originalText = line;
                    string translatedText = sr.ReadLine();
                    if (translatedText == null)
                    {
                        break;
                    }
                    // Silently ignore any duplicates, as we can't report warnings here
                    if (!_translatedLines.ContainsKey(originalText))
                    {
                        _translatedLines.Add(originalText, translatedText);
                    }
                }
            }
        }

        private void ReadSpecialTags(string line)
        {
            if (line.StartsWith(NORMAL_FONT_TAG))
            {
                _normalFont = ReadOptionalInt(line.Substring(NORMAL_FONT_TAG.Length));
            }
            else if (line.StartsWith(SPEECH_FONT_TAG))
            {
                _speechFont = ReadOptionalInt(line.Substring(SPEECH_FONT_TAG.Length));
            }
            else if (line.StartsWith(TEXT_DIRECTION_TAG))
            {
                string directionText = line.Substring(TEXT_DIRECTION_TAG.Length);
                if (directionText == TAG_DIRECTION_LEFT)
                {
                    _rightToLeftText = false;
                }
                else if (directionText == TAG_DIRECTION_RIGHT)
                {
                    _rightToLeftText = true;
                }
                else
                {
                    _rightToLeftText = null;
                }
            }
        }

        private int? ReadOptionalInt(string textToParse)
        {
            if (textToParse == TAG_DEFAULT)
            {
                return null;
            }
            return Convert.ToInt32(textToParse);
        }

        private string WriteOptionalInt(int? currentValue)
        {
            if (currentValue == null)
            {
                return TAG_DEFAULT;
            }
            return currentValue.Value.ToString();
        }
    }
}
