using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;
using static System.Collections.Specialized.BitVector32;

namespace AGS.Types
{
    public class Translation
    {
        public const string TRANSLATION_SOURCE_FILE_EXTENSION = ".trs";
        public const string TRANSLATION_COMPILED_FILE_EXTENSION = ".tra";

        private const string NORMAL_FONT_TAG = "NormalFont";
        private const string SPEECH_FONT_TAG = "SpeechFont";
        private const string TEXT_DIRECTION_TAG = "TextDirection";
        private const string AUTO_PARSERSAID_TAG = "AutoTranslateParserSaid";
        private const string ENCODING_TAG = "Encoding";
        private const string LANGUAGE_TAG = "Language";
        private const string FONT_OVERRIDE_TAG = "Font";
        private const string TAG_DEFAULT = "DEFAULT";
        private const string TAG_DIRECTION_LEFT = "LEFT";
        private const string TAG_DIRECTION_RIGHT = "RIGHT";
        private const string TAG_ON = "ON";
        private const string TAG_OFF = "OFF";
        private const string ANNOTATE_SECTION = "SECTION";
        private const string ANNOTATE_OBSOLETE = "OBSOLETE";
        private const string ANNOTATE_PARSERWORD = "PARSERWORD";

        private string _name;
        private string _fileName;
        private bool _modified;
        private int? _normalFont;
        private int? _speechFont;
        private bool? _rightToLeftText;
        private bool _autoTranslateParserSaid = false;
        private string _encodingHint;
        private Encoding _encoding;
        private string _language;
        private Dictionary<int, Font> _fontOverrides = new Dictionary<int, Font>();
        private Dictionary<string, string> _translatedLines = new Dictionary<string, string>();
        private Dictionary<string, TranslationEntryOptions> _entryOptions = new Dictionary<string, TranslationEntryOptions>();
        private Dictionary<string, TranslationSection> _translationSections = new Dictionary<string, TranslationSection>();

        public Translation(string name)
        {
            this.Name = name;
            _modified = false;
            _normalFont = null;
            _speechFont = null;
            _rightToLeftText = null;
            EncodingHint = "UTF-8";
            _language = "en_US";
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

        /// <summary>
        /// A map of translated lines, where the text in base game language is a key,
        /// and the text in another language is a value.
        /// </summary>
        public Dictionary<string, string> TranslatedLines
        {
            get { return _translatedLines; }
            set { _translatedLines = value; }
        }

        /// <summary>
        /// A map of options per particular lines of text.
        /// </summary>
        public Dictionary<string, TranslationEntryOptions> TranslatedEntryOptions
        {
            get { return _entryOptions; }
            set { _entryOptions = value; }
        }

        /// <summary>
        /// A map of sections per particular lines of text.
        /// When translation source file is generated, the lines will be split up
        /// depending on this map. If line does not have a corresponding section,
        /// such line will be written as a part of the generic no-name section.
        /// </summary>
        public Dictionary<string, TranslationSection> TranslationSections
        {
            get { return _translationSections; }
            set { _translationSections = value; }
        }

        public int? NormalFont
        {
            get { return _normalFont; }
            set { _normalFont = value; }
        }

        public int? SpeechFont
        {
            get { return _speechFont; }
            set { _speechFont = value; }
        }

        public bool? RightToLeftText
        {
            get { return _rightToLeftText; }
            set { _rightToLeftText = value; }
        }

        public bool AutoTranslateParserSaid
        {
            get { return _autoTranslateParserSaid; }
            set { _autoTranslateParserSaid = value; }
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

        public string TextLanguage
        {
            get { return _language; }
            set { _language = value; }
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
            var sectionLists = PrepareDataForSave();
            using (StreamWriter sw = new StreamWriter(FileName, false, _encoding))
            {
                SaveDataImpl(sw, sectionLists);
            }
            Modified = false;
        }

        public void SaveData(StreamWriter sw)
        {
            var sectionLists = PrepareDataForSave();
            SaveDataImpl(sw, sectionLists);
            Modified = false;
        }

        /// <summary>
        /// Organizes translated lines by sections.
        /// </summary>
        private List<Tuple<TranslationSection, List<string>>> PrepareDataForSave()
        {
            var sectionLists = new List<Tuple<TranslationSection, List<string>>>();
            var sectionIndexByKey = new Dictionary<string, int>();
            var nonameSection = new TranslationSection(string.Empty);
            sectionLists.Add(new Tuple<TranslationSection, List<string>>(nonameSection, new List<string>()));
            sectionIndexByKey.Add(string.Empty, 0);
            foreach (var line in _translatedLines)
            {
                List<string> sectionList = null;
                TranslationSection section = null;
                _translationSections.TryGetValue(line.Key, out section);
                section = (section != null && !string.IsNullOrEmpty(section.Name)) ? section : nonameSection;
                int sectionIndex = 0;
                if (sectionIndexByKey.TryGetValue(section.Name, out sectionIndex))
                {
                    sectionList = sectionLists[sectionIndex].Item2;
                }
                else
                {
                    sectionList = new List<string>();
                    sectionLists.Add(new Tuple<TranslationSection, List<string>>(section, sectionList));
                    sectionIndexByKey.Add(section.Name, sectionLists.Count - 1);
                }
                sectionList.Add(line.Key);
            }
            return sectionLists;
        }

        /// <summary>
        /// Writes translation source file into the provided stream.
        /// </summary>
        private void SaveDataImpl(StreamWriter sw, List<Tuple<TranslationSection, List<string>>> sectionLists)
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
            sw.WriteLine("// Text language, use standard locale strings, like 'en', 'en_US', etc");
            sw.WriteLine($"//#Language={(_language != null ? _language.Replace('-', '_') : string.Empty)}");
            sw.WriteLine("// Whether engine should translate Parser.Said strings automatically - ON or OFF");
            sw.WriteLine($"//#AutoTranslateParserSaid={(_autoTranslateParserSaid ? TAG_ON : TAG_OFF)}");
            if (_fontOverrides.Count != 0)
            {
                WriteFontOverrides(sw);
            }
            sw.WriteLine("//  ");
            sw.WriteLine("// ** REMEMBER, WRITE YOUR TRANSLATION IN THE EMPTY LINES, DO");
            sw.WriteLine("// ** NOT CHANGE THE EXISTING TEXT.");

            TranslationEntryOptions entryOptions = null;
            foreach (var sectionList in sectionLists)
            {
                var section = sectionList.Item1;
                var lines = sectionList.Item2;
                if (lines.Count == 0)
                    continue;

                sw.WriteLine("//-----------------------------------------------------------------------------");
                if (string.IsNullOrWhiteSpace(section.Comment))
                    sw.WriteLine($"//$SECTION = {section.Name}");
                else
                    sw.WriteLine($"//$SECTION = {section.Name}; {section.Comment}");
                sw.WriteLine("//-----------------------------------------------------------------------------");
                foreach (string key in lines)
                {
                    if (_entryOptions.TryGetValue(key, out entryOptions))
                    {
                        foreach (var a in entryOptions.Metadata)
                            sw.WriteLine($"//${a}");
                    }

                    sw.WriteLine(key);
                    sw.WriteLine(_translatedLines[key]);
                }
            }
        }

        /// <summary>
        /// Loads translation data from the source file (TRS).
        /// Throws IO exceptions.
        /// </summary>
        public void LoadData()
        {
            TryLoadData();
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
                bool result = false;
                string old_encoding = _encodingHint;
                using (StreamReader sr = new StreamReader(FileName, _encoding))
                {
                    result = LoadDataImpl(sr, errors);
                }
                if (!result && (string.Compare(old_encoding, _encodingHint) != 0))
                {
                    // try again with the new encoding
                    using (StreamReader sr = new StreamReader(FileName, _encoding))
                    {
                        result = LoadDataImpl(sr, errors);
                    }
                }
            }
            catch (Exception e)
            {
                errors.Add(new CompileError(string.Format("Failed to load translation from {0}: \n{1}", FileName, e.Message)));
                _translatedLines.Clear(); // clear on failure
            }
            return errors;
        }

        public CompileMessages TryLoadData(StreamReader sr)
        {
            CompileMessages errors = new CompileMessages();
            try
            {
                if (!LoadDataImpl(sr, errors))
                {
                    return errors;
                }
            }
            catch (Exception e)
            {
                errors.Add(new CompileError(string.Format("Failed to load translation: \n{1}", e.Message)));
                _translatedLines.Clear(); // clear on failure
            }
            return errors;
        }

        // TODO: frankly I am not convinced that the TRS file reading/writing should be
        // in this Translation class. It might be more convenient to have them elsewhere,
        // in a translation file parser/serializer.
        private bool LoadDataImpl(StreamReader sr, CompileMessages errors)
        {
            _fontOverrides = new Dictionary<int, Font>();
            _translatedLines = new Dictionary<string, string>();
            _entryOptions = new Dictionary<string, TranslationEntryOptions>();
            _translationSections = new Dictionary<string, TranslationSection>();
            List<string> annotateNextLine = new List<string>();
            var sectionsByKey = new Dictionary<string, TranslationSection>();
            string currentSectionKey = string.Empty;
            string currentSectionComment = string.Empty;
            string old_encoding = _encodingHint;

            {
                string line;
                while ((line = sr.ReadLine()) != null)
                {
                    if (line.StartsWith("//"))
                    {
                        if (line.Length > 2 && line[2] == '#')
                        {
                            ReadSpecialTags(line.Substring(3));
                            if (string.Compare(old_encoding, _encodingHint) != 0)
                            {
                                return false;
                            }
                        }
                        else if (line.Length > 2 && line[2] == '$')
                        {
                            string annotation = line.Substring(3);
                            var keyValue = Utilities.ParseKeyValue(annotation);
                            if (keyValue.Key == ANNOTATE_SECTION)
                            {
                                int splitAt = keyValue.Value.IndexOf(';');
                                currentSectionKey = keyValue.Value.Substring(0, splitAt >= 0 ? splitAt : keyValue.Value.Length).Trim();
                                currentSectionComment = splitAt >= 0 ? keyValue.Value.Substring(splitAt + 1).Trim() : string.Empty;
                            }
                            else
                            {
                                annotateNextLine.Add(annotation);
                            }
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
                        if (annotateNextLine.Count > 0)
                        {
                            _entryOptions[originalText] = CreateEntryOptions(annotateNextLine);
                            annotateNextLine.Clear();
                        }
                        TranslationSection section = null;
                        if (!sectionsByKey.TryGetValue(currentSectionKey, out section))
                        {
                            section = new TranslationSection(currentSectionKey, currentSectionComment);
                            sectionsByKey[currentSectionKey] = section;

                        }
                        _translationSections[originalText] = section;
					}
                }
            }
            return true;
        }

        private void ReadSpecialTags(string line)
        {
            var keyValue = Utilities.ParseKeyValue(line);
            var key = keyValue.Key;
            var value = keyValue.Value;

            if (key == NORMAL_FONT_TAG)
            {
                _normalFont = ReadOptionalInt(value);
            }
            else if (key == SPEECH_FONT_TAG)
            {
                _speechFont = ReadOptionalInt(value);
            }
            else if (key == TEXT_DIRECTION_TAG)
            {
                string directionText = value;
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
            else if (key == AUTO_PARSERSAID_TAG)
            {
                if (value == TAG_ON)
                    _autoTranslateParserSaid = true;
                else
                    _autoTranslateParserSaid = false;
            }
            else if (key == ENCODING_TAG)
            {
                EncodingHint = value;
            }
            else if (key == LANGUAGE_TAG)
            {
                TextLanguage = value;
            }
            else if (key.StartsWith(FONT_OVERRIDE_TAG))
            {
                int fontIndex = ParseFontN(key);
                if (fontIndex >= 0)
                {
                    if (!FontOverrides.ContainsKey(fontIndex))
                    {
                        FontOverrides.Add(fontIndex, ParseFontOverride(value));
                    }
                }
            }
        }

        private int? ReadOptionalInt(string textToParse)
        {
            int value;
            if (textToParse == TAG_DEFAULT || !int.TryParse(textToParse, out value))
            {
                return null;
            }
            return value;
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
                        return Utilities.ParseKeyValue(s);
                    }).ToArray();
                foreach (var option in options)
                {
                    if (option.Key == "File")
                    {
                        font.ProjectFilename = option.Value;
                    }
                    else if (option.Key == "Size")
                    {
                        font.PointSize = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "SizeMultiplier")
                    {
                        font.SizeMultiplier = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "Outline")
                    {
                        if (option.Value == "NONE")
                        {
                            font.OutlineStyle = FontOutlineStyle.None;
                        }
                        else if (option.Value == "AUTO")
                        {
                            font.OutlineStyle = FontOutlineStyle.Automatic;
                        }
                        else
                        {
                            int outFontID = ParseFontN(option.Value);
                            if (outFontID >= 0)
                            {
                                font.OutlineStyle = FontOutlineStyle.UseOutlineFont;
                                font.OutlineFont = outFontID;
                            }
                        }
                    }
                    else if (option.Key == "AutoOutline")
                    {
                        if (option.Value == "SQUARED")
                        {
                            font.AutoOutlineStyle = FontAutoOutlineStyle.Squared;
                        }
                        else if (option.Value == "ROUND")
                        {
                            font.AutoOutlineStyle = FontAutoOutlineStyle.Rounded;
                        }
                    }
                    else if (option.Key == "AutoOutlineThickness")
                    {
                        font.AutoOutlineThickness = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "HeightDefinition")
                    {
                        if (option.Value == "NOMINAL")
                        {
                            font.HeightDefinedBy = FontHeightDefinition.NominalHeight;
                        }
                        else if (option.Value == "REAL")
                        {
                            font.HeightDefinedBy = FontHeightDefinition.PixelHeight;
                        }
                        else if (option.Value == "CUSTOM")
                        {
                            font.HeightDefinedBy = FontHeightDefinition.CustomValue;
                        }
                    }
                    else if (option.Key == "CustomHeight")
                    {
                        font.CustomHeightValue = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "VerticalOffset")
                    {
                        font.VerticalOffset = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "LineSpacing")
                    {
                        font.LineSpacing = option.Value.ParseIntOrDefault();
                    }
                    else if (option.Key == "CharacterSpacing")
                    {
                        font.CharacterSpacing = option.Value.ParseIntOrDefault();
                    }
                }
                return font;
            }
        }

        private TranslationEntryOptions CreateEntryOptions(List<string> annotations)
        {
            TranslationEntryOptions options = new TranslationEntryOptions();
            options.Metadata = new List<string>(annotations);

            // Parse for known annotations
            foreach (var annotation in annotations)
            {
                var keyValue = Utilities.ParseKeyValue(annotation);
                var key = keyValue.Key;
                var value = keyValue.Value;

                if (key == ANNOTATE_OBSOLETE)
                {
                    options.IsObsolete = true;
                }
                else if (key == ANNOTATE_PARSERWORD)
                {
                    options.ParserWordID = Utilities.ParseIntOrDefault(value, -1);
                }
            }

            return options;
        }
    }
}
