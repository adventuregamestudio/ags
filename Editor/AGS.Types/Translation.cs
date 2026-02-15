using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
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
        private const string ENCODING_TAG = "//#Encoding=";
        private const string FONT_OVERRIDE_TAG = "//#Font"; // dont include '=', as 'Font' is followed by an index
        private const string TAG_DEFAULT = "DEFAULT";
        private const string TAG_DIRECTION_LEFT = "LEFT";
        private const string TAG_DIRECTION_RIGHT = "RIGHT";

        private string _name;
        private string _fileName;
        private bool _modified;
        private int? _normalFont;
        private int? _speechFont;
        private bool? _rightToLeftText;
        private string _encodingHint;
        private Encoding _encoding;
        private Dictionary<int, Font> _fontOverrides = new Dictionary<int, Font>();
        private Dictionary<string, string> _translatedLines;

        public Translation(string name)
        {
            this.Name = name;
            _modified = false;
            _normalFont = null;
            _speechFont = null;
            _rightToLeftText = null;
            EncodingHint = "UTF-8";
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

        public string EncodingHint
        {
            get { return _encodingHint; }
            set
            {
                _encodingHint = value;
                _encoding = Encoding.Default;
                if (!string.IsNullOrEmpty(value))
                {
                    if (string.Compare(_encodingHint, "UTF-8", true) == 0)
                        _encoding = new UTF8Encoding(false); // UTF-8 w/o BOM
                }
            }
        }

        public Encoding Encoding
        {
            get { return _encoding; }
        }

        /// <summary>
        /// Contains definitions of font overrides provided for the given font indexes;
        /// these have to be used when this translation is loaded at runtime.
        /// If the Font object contains a real ID, that means that the override
        /// should replace a font with another existing font (so long as it's valid).
        /// Otherwise, this instructs that the new font should be generated with
        /// the given set of properties.
        /// </summary>
        public Dictionary<int, Font> FontOverrides
        {
            get { return _fontOverrides; }
            set { _fontOverrides = value; }
        }

        public bool Modified
        {
            get { return _modified; }
            set { _modified = value; }
        }

        public Translation(XmlNode node)
        {
            this.Name = SerializeUtils.GetElementString(node, "Name");
            _modified = false;
            _normalFont = null;
            _speechFont = null;
            _rightToLeftText = null;
            _encodingHint = null;
            _encoding = Encoding.Default;
            try
            {
                LoadData();
            }
            catch (Exception)
            {
                _translatedLines.Clear(); // clear on failure
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Translation");
            writer.WriteElementString("Name", _name);
            writer.WriteEndElement();
        }

        public void SaveData()
        {
            using (StreamWriter sw = new StreamWriter(FileName, false, _encoding))
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
                sw.WriteLine("// Text encoding hint - ASCII or UTF-8");
                sw.WriteLine("//#Encoding=" + (_encodingHint ?? "ASCII"));
                if (_fontOverrides.Count != 0)
                {
                    WriteFontOverrides(sw);
                }
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

        /// <summary>
        /// Loads translation data from the source file (TRS).
        /// Throws IO exceptions.
        /// </summary>
        public void LoadData()
        {
            CompileMessages errors = new CompileMessages();
            LoadDataImpl(errors);
        }

        /// <summary>
        /// Loads translation data from the source file (TRS).
        /// Suppresses exceptions and returns error messages.
        /// </summary>
        public CompileMessages TryLoadData()
        {
            CompileMessages errors = new CompileMessages();
            try
            {
                LoadDataImpl(errors);
            }
            catch (Exception e)
            {
                errors.Add(new CompileError(string.Format("Failed to load translation from {0}: \n{1}", FileName, e.Message)));
                _translatedLines.Clear(); // clear on failure
            }
            return errors;
        }

        // TODO: frankly I am not convinced that the TRS file reading/writing should be
        // in this Translation class. It might be more convenient to have them elsewhere,
        // in a translation file parser/serializer.
        private void LoadDataImpl(CompileMessages errors)
        {
            _fontOverrides = new Dictionary<int, Font>();
            _translatedLines = new Dictionary<string, string>();
            string old_encoding = _encodingHint;

            using (StreamReader sr = new StreamReader(FileName, _encoding))
            {
                string line;
                while ((line = sr.ReadLine()) != null)
                {
                    if (line.StartsWith("//"))
                    {
                        ReadSpecialTags(line);
                        if (string.Compare(old_encoding, _encodingHint) != 0)
                        {
                            sr.Close();
                            LoadDataImpl(errors); // try again with the new encoding
                            return;
                        }
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
            else if (line.StartsWith(ENCODING_TAG))
            {
                EncodingHint = line.Substring(ENCODING_TAG.Length);
            }
            else if (line.StartsWith(FONT_OVERRIDE_TAG))
            {
                int assignAt = line.IndexOf('=', FONT_OVERRIDE_TAG.Length);
                if (assignAt > 0)
                {
                    int fontIndex;
                    if (int.TryParse(line.Substring(FONT_OVERRIDE_TAG.Length, assignAt - FONT_OVERRIDE_TAG.Length), out fontIndex))
                    {
                        if (!FontOverrides.ContainsKey(fontIndex))
                        {
                            FontOverrides.Add(fontIndex, ParseFontOverride(line.Substring(assignAt + 1)));
                        }
                    }
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

        /// <summary>
        /// Writes font overrides into the translation source file.
        /// </summary>
        private void WriteFontOverrides(StreamWriter sw)
        {
            foreach (var fontOverride in _fontOverrides)
            {
                StringBuilder sb = new StringBuilder();
                int fontIndex = fontOverride.Key;
                Font font = fontOverride.Value;
                sb.Append($"//#Font{fontIndex}=");
                if (font.ID >= 0)
                {
                    sb.Append($"Font{font.ID}");
                }
                else
                {
                    // Only write non-default values. Unfortunately there's no way to know
                    // which values user set in the original source file.
                    sb.Append($"File={font.ProjectFilename};");
                    if (font.PointSize > 0)
                        sb.Append($"Size={font.PointSize.ToString()};");
                    if (font.SizeMultiplier > 1)
                        sb.Append($"SizeMultiplier={font.SizeMultiplier.ToString()};");

                    if (font.OutlineStyle == FontOutlineStyle.Automatic)
                        sb.Append($"Outline=AUTO;");
                    else if(font.OutlineStyle == FontOutlineStyle.UseOutlineFont)
                        sb.Append($"Outline=Font{font.OutlineFont};");

                    if (font.OutlineStyle == FontOutlineStyle.Automatic)
                    {
                        if (font.AutoOutlineStyle == FontAutoOutlineStyle.Rounded)
                            sb.Append($"AutoOutline=ROUND;");

                        sb.Append($"AutoOutlineThickness={font.AutoOutlineThickness};");
                    }

                    if (font.HeightDefinedBy == FontHeightDefinition.PixelHeight)
                        sb.Append($"HeightDefinition=REAL;");
                    else if (font.HeightDefinedBy == FontHeightDefinition.CustomValue)
                        sb.Append($"HeightDefinition=CUSTOM;");

                    if (font.HeightDefinedBy == FontHeightDefinition.CustomValue)
                    {
                        sb.Append($"CustomHeight={font.CustomHeightValue};");
                    }

                    if (font.VerticalOffset != 0)
                        sb.Append($"VerticalOffset={font.VerticalOffset};");
                    if (font.LineSpacing != 0)
                        sb.Append($"LineSpacing={font.LineSpacing};");
                    if (font.CharacterSpacing != 0)
                        sb.Append($"CharacterSpacing={font.CharacterSpacing};");
                }
                sw.WriteLine(sb.ToString());
            }
        }

        /// <summary>
        /// Parses "FontN" kind of string, where N is a font's ID.
        /// </summary>
        private int ParseFontN(string value)
        {
            int fontID;
            if (value.StartsWith("Font") && int.TryParse(value.Substring(4), out fontID))
                return fontID;
            return -1;
        }

        private Font ParseFontOverride(string value)
        {
            value = value.Trim();
            // Format 1:
            //    FontN
            // Format 2:
            //    Property1=Value1;Property2=Value2;Property3=Value3;...
            int reFontNumber = ParseFontN(value);
            if (reFontNumber >= 0)
            {
                var font = new Font();
                font.ID = reFontNumber;
                return font;
            }
            else
            {
                var font = new Font();
                font.ID = -1; // mark it as not one of the game's font
                var options = value.Split(';').Select(s =>
                    {
                        var keyValue = s.Split('=');
                        if (keyValue.Length == 2)
                            return new Tuple<string, string>(keyValue[0].Trim(), keyValue[1].Trim());
                        else
                            return new Tuple<string, string>(string.Empty, string.Empty);
                    }).ToArray();
                foreach (var option in options)
                {
                    if (option.Item1 == "File")
                    {
                        font.ProjectFilename = option.Item2;
                    }
                    else if (option.Item1 == "Size")
                    {
                        font.PointSize = option.Item2.ParseIntOrDefault();
                    }
                    else if (option.Item1 == "SizeMultiplier")
                    {
                        font.SizeMultiplier = option.Item2.ParseIntOrDefault();
                    }
                    else if (option.Item1 == "Outline")
                    {
                        if (option.Item2 == "NONE")
                        {
                            font.OutlineStyle = FontOutlineStyle.None;
                        }
                        else if (option.Item2 == "AUTO")
                        {
                            font.OutlineStyle = FontOutlineStyle.Automatic;
                        }
                        else
                        {
                            int outFontID = ParseFontN(option.Item2);
                            if (outFontID >= 0)
                            {
                                font.OutlineStyle = FontOutlineStyle.UseOutlineFont;
                                font.OutlineFont = outFontID;
                            }
                        }
                    }
                    else if (option.Item1 == "AutoOutline")
                    {
                        if (option.Item2 == "SQUARED")
                        {
                            font.AutoOutlineStyle = FontAutoOutlineStyle.Squared;
                        }
                        else if (option.Item2 == "ROUND")
                        {
                            font.AutoOutlineStyle = FontAutoOutlineStyle.Rounded;
                        }
                    }
                    else if (option.Item1 == "AutoOutlineThickness")
                    {
                        font.AutoOutlineThickness = option.Item2.ParseIntOrDefault();
                    }
                    else if (option.Item1 == "HeightDefinition")
                    {
                        if (option.Item2 == "NOMINAL")
                        {
                            font.HeightDefinedBy = FontHeightDefinition.NominalHeight;
                        }
                        else if (option.Item2 == "REAL")
                        {
                            font.HeightDefinedBy = FontHeightDefinition.PixelHeight;
                        }
                        else if (option.Item2 == "CUSTOM")
                        {
                            font.HeightDefinedBy = FontHeightDefinition.CustomValue;
                        }
                    }
                    else if (option.Item1 == "CustomHeight")
                    {
                        font.CustomHeightValue = option.Item2.ParseIntOrDefault();
                    }
                    else if (option.Item1 == "VerticalOffset")
                    {
                        font.VerticalOffset = option.Item2.ParseIntOrDefault();
                    }
                    else if (option.Item1 == "LineSpacing")
                    {
                        font.LineSpacing = option.Item2.ParseIntOrDefault();
                    }
                    else if (option.Item1 == "CharacterSpacing")
                    {
                        font.CharacterSpacing = option.Item2.ParseIntOrDefault();
                    }
                }
                return font;
            }
        }
    }
}
