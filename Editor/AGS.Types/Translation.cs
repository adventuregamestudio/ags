using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace AGS.Types
{
    public class Translation
    {
        public const string TRANSLATION_SOURCE_FILE_EXTENSION = ".po";
        public const string TRANSLATION_COMPILED_FILE_EXTENSION = ".tra";

        private const string NORMAL_FONT_TAG = "NormalFont";
        private const string SPEECH_FONT_TAG = "SpeechFont";
        private const string TEXT_DIRECTION_TAG = "TextDirection";
        private const string ENCODING_TAG = "Encoding";
        private const string FONT_OVERRIDE_TAG = "Font";
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
        private string _baseLanguage;
        private Dictionary<int, Font> _fontOverrides = new Dictionary<int, Font>();
        private Dictionary<string, TranslationEntry> _translatedEntries;

        public Translation(string name, string baseLanguage)
        {
            this.Name = name;
            _modified = false;
            _normalFont = null;
            _speechFont = null;
            _rightToLeftText = null;
            EncodingHint = "UTF-8";
            _baseLanguage = baseLanguage;
        }

        public Translation(XmlNode node, string baseLanguage)
        {
            this.Name = SerializeUtils.GetElementString(node, "Name");
            _modified = false;
            _normalFont = null;
            _speechFont = null;
            _rightToLeftText = null;
            _encodingHint = null;
            _encoding = Encoding.Default;
            _baseLanguage = baseLanguage;
            try
            {
                LoadData();
            }
            catch (Exception)
            {
                _translatedEntries.Clear(); // clear on failure
            }
        }

        public string Name
        {
            get { return _name; }
            set
            {
                _name = value;
                _fileName = _name + TRANSLATION_SOURCE_FILE_EXTENSION;
            }
        }
        
        public string FileName
        {
            get { return _fileName; }
        }

        public string CompiledFileName
        {
            get { return _name + TRANSLATION_COMPILED_FILE_EXTENSION; }
        }

        public Dictionary<string, TranslationEntry> TranslatedEntries
        {
            get { return _translatedEntries; }
            set { _translatedEntries = value; }
        }

        /// <summary>
        /// TranslatedLines is a deprecated API, now a stub that prevents older plugins
        /// and tools from throwing exceptions.
        /// TODO: consider returning a filled dictionary generated from TranslatedEntries?
        /// </summary>
        [Obsolete]
        public Dictionary<string, string> TranslatedLines
        {
            get { return new Dictionary<string, string>(); }
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

        public string BaseLanguage
        {
            get { return _baseLanguage; }
            set { _baseLanguage = value; }
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
                string encoding = _encodingHint ?? "ASCII";
                sw.WriteLine("# AGS TRANSLATION SOURCE FILE");
                sw.WriteLine("# This is a PO file generated according to the gettext specificatins.");
                sw.WriteLine("# Special characters such as %%s symbolise things within the game,");
                sw.WriteLine("# so should be left in an appropriate place in the message.");
                sw.WriteLine("# ");
                sw.WriteLine("# ** Translation settings are below");
                sw.WriteLine("# ** Leave them as \"DEFAULT\" to use the game settings");
                sw.WriteLine("# The normal font to use - DEFAULT or font number");
                sw.WriteLine("# $NormalFont=" + _normalFont.NullableToString(TAG_DEFAULT));
                sw.WriteLine("# The speech font to use - DEFAULT or font number");
                sw.WriteLine("# $SpeechFont=" + _speechFont.NullableToString(TAG_DEFAULT));
                sw.WriteLine("# Text direction - DEFAULT, LEFT or RIGHT");
                sw.WriteLine("# $TextDirection=" + ((_rightToLeftText == true) ? TAG_DIRECTION_RIGHT : ((_rightToLeftText == null) ? TAG_DEFAULT : TAG_DIRECTION_LEFT)));
                sw.WriteLine("# Text encoding hint - ASCII or UTF-8");
                sw.WriteLine("# $Encoding=" + encoding);
                sw.WriteLine("#  ");
                if (_fontOverrides.Count != 0)
                {
                    WriteFontOverrides(sw);
                }
                sw.WriteLine("# ** IT IS SUGGESTED TO USE A THIRD-PARTY TOOL TO EDIT THIS FILE");
                // PO metadata
                sw.WriteLine("msgid \"\"");
                sw.WriteLine("msgstr \"\"");
                sw.WriteLine("\"Last-Translator: \\n\"");
                sw.WriteLine("\"Language-Team: \\n\"");
                sw.WriteLine("\"Language: " + Encode(_name) + "\\n\"");
                sw.WriteLine("\"X-Source-Language: " + Encode(_baseLanguage) + "\\n\"");
                sw.WriteLine("\"MIME-Version: 1.0\\n\"");
                sw.WriteLine("\"Content-Type: text/plain; charset=" + ( _encodingHint == "ASCII" ? "ISO-8859-1" : _encodingHint) + "\\n\"");
                sw.WriteLine("\"Content-Transfer-Encoding: 8bit\\n\"");
                sw.WriteLine("\"X-Generator: AGS " + Version.AGS_EDITOR_VERSION + "\\n\"");

                foreach (string key in _translatedEntries.Keys)
                {
                    TranslationEntry entry = _translatedEntries[key];
                    sw.WriteLine("");
                    foreach (string metadata in entry.Metadata)
                        sw.WriteLine(metadata);
                    if (entry.Context != null)
                        sw.WriteLine("msgctxt \"" + entry.Context + "\"");
                    sw.WriteLine("msgid \"" + entry.Key + "\"");
                    sw.WriteLine("msgstr \"" + entry.Value + "\"");
                }
                sw.WriteLine("");
            }
            this.Modified = false;
        }

        private enum ParseState
        {
            NewEntry,
            ParsingMeta,
            ParsingContext,
            ParsingId,
            ParsingString
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
                _translatedEntries.Clear(); // clear on failure
            }
            return errors;
        }

        // TODO: frankly I am not convinced that the TRS file reading/writing should be
        // in this Translation class. It might be more convenient to have them elsewhere,
        // in a translation file parser/serializer.
        private void LoadDataImpl(CompileMessages errors)
        {
            _fontOverrides = new Dictionary<int, Font>();
            _translatedEntries = new Dictionary<string, TranslationEntry>();
            string old_encoding = _encodingHint;

            if (!File.Exists(FileName))
            {
                return;
            }

            string fileEncodingHint;
            if (LoadDataImpl(errors, out fileEncodingHint))
                return;
            // If a different encoding is required, then try again with a proper encoding
            if (string.Compare(EncodingHint, fileEncodingHint) != 0)
            {
                EncodingHint = fileEncodingHint;
                LoadDataImpl(errors, out fileEncodingHint);
            }
        }

        private bool LoadDataImpl(CompileMessages errors, out string fileEncodingHint)
        {
            fileEncodingHint = EncodingHint;
            ParseState state = ParseState.NewEntry;
            TranslationEntry entry = new TranslationEntry();
            using (StreamReader sr = new StreamReader(FileName, _encoding))
            {
                string line;
                while ((line = sr.ReadLine()) != null)
                {
                    string extracted_string = POExtractString(line);

                    // Case 1: encountered metadata
                    // TODO: track different types of metadata (flags, comments, etc)
                    if (line.StartsWith("#"))
                    {
                        if (line.StartsWith("# $"))
                        {
                            ReadSpecialTags(line.Substring(3), ref fileEncodingHint);
                            if (string.Compare(EncodingHint, fileEncodingHint) != 0)
                            {
                                // Source file requires different encoding
                                return false;
                            }
                        }
                        entry.Metadata.Add(line);
                        continue;
                    }

                    // Case 2: we got a string, figure out what to do with it
                    if (extracted_string != null)
                    {
                        // start of a new type
                        if (line.StartsWith("msgctxt")) // context
                        {
                            state = ParseState.ParsingContext;
                            entry.Context = extracted_string;
                            continue;
                        }
                        if (line.StartsWith("msgid")) // string id, untranslated
                        {
                            state = ParseState.ParsingId;
                            entry.Key = extracted_string;
                            continue;
                        }
                        if (line.StartsWith("msgstr")) // translated string
                        {
                            state = ParseState.ParsingString;
                            entry.Value = extracted_string;
                            continue;
                        }

                        // strings can be split on multiple lines, usually the first will be empty, but no guarantee
                        // an external tool may have done that, so let's append
                        switch (state)
                        {
                            case ParseState.ParsingContext:
                                entry.Context += extracted_string;
                                continue;

                            case ParseState.ParsingId:
                                entry.Key += extracted_string;
                                continue;

                            case ParseState.ParsingString:
                                entry.Value += extracted_string;
                                continue;

                            default: continue; // ignore orphaned strings since we can't throw errors
                        }
                    }

                    // Case 3: we were processing an entry and encountered an empty line
                    if (state != ParseState.NewEntry && line.Trim().Length == 0)
                    {
                        // note: a valid entry must have a non-empty msgid
                        // the first empty hardcoded entry, which is metadata, is ignored and recreated
                        if (entry.Key != null && entry.Key.Length > 0)
                        {
                            _translatedEntries.Add(entry.Key, entry);
                        }
                        state = ParseState.NewEntry;
                        entry = new TranslationEntry();
                    }
                    else if (state == ParseState.NewEntry && line.Trim().Length == 0)
                    {
                        continue; // ignore additional empty lines
                    }


                } // while

                // note: if there's not an empty line at the end, the last record won't be stored
            }
            return true;
        }

        private readonly Regex POString = new Regex("^(?:msgctxt |msgid |msgstr |)+\"(.*)\"$", RegexOptions.Compiled | RegexOptions.IgnoreCase);

        // utility to extract a string between quotes, but only if it meets the format
        private string POExtractString(string line)
        {
            if (line == null) return null;

            Match match = POString.Match(line);
            if (match.Success)
                return (match.Groups[1].Value);

            return null;
        }

        private static string Decode(string text)
        {
            StringBuilder builder = new StringBuilder("");
            for (var i = 0; i < text.Length; i++)
            {
                var c = text[i];
                if (c != '\\')
                {
                    builder.Append(c);
                    continue;
                }

                if (++i < text.Length)
                {
                    c = text[i];
                    switch (c)
                    {
                        case '\\':
                        case '"':
                            builder.Append(c);
                            continue;
                        case 't':
                            builder.Append('\t');
                            continue;
                        case 'r':
                            var index = i;
                            if (++index + 1 < text.Length && text[index] == '\\' && text[++index] == 'n')
                                i = index;
                            // "\r" and "\r\n" are both accepted as new line
                            goto case 'n';
                        case 'n':
                            builder.Append("\n");
                            continue;
                    }
                }

                // invalid escape sequence
                //return i - 1;
            }

            return builder.ToString();
        }

        private static string Encode(string text)
        {
            StringBuilder builder = new StringBuilder("");
            for (var i = 0; i < text.Length; i++)
            {
                var c = text[i];
                switch (c)
                {
                    case '\\':
                    case '"':
                        builder.Append('\\').Append(c);
                        continue;
                    case '\t':
                        builder.Append('\\').Append('t');
                        continue;
                    case '\r':
                        var index = 0;
                        if (++index < text.Length && text[index] == '\n')
                            i = index;
                        // "\r" and "\r\n" are encoded the same as "\n" to keep PO content platform-independent
                        goto case '\n';
                    case '\n':
                        builder.Append('\\').Append('n');
                        continue;
                }

                builder.Append(c);
            }
            return builder.ToString();
        }

        private void ReadSpecialTags(string line, ref string encodingHint)
        {
            var keyValue = ParseKeyValue(line);
            var key = keyValue.Item1;
            var value = keyValue.Item2;

            if (key == NORMAL_FONT_TAG)
            {
                _normalFont = Utilities.ParseNullableInt(value, TAG_DEFAULT);
            }
            else if (key == SPEECH_FONT_TAG)
            {
                _speechFont = Utilities.ParseNullableInt(value, TAG_DEFAULT);
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
            else if (key == ENCODING_TAG)
            {
                EncodingHint = value;
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
                    sb.Append($"File={font.FontFileName};");
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

        // TODO: move to some utility module?
        private Tuple<string, string> ParseKeyValue(string line, char separator = '=')
        {
            int firstSep = line.IndexOf(separator);
            if (firstSep >= 0)
            {
                return new Tuple<string, string>(
                    line.Substring(0, firstSep).Trim(),
                    line.Substring(firstSep + 1).Trim());
            }
            else
            {
                return new Tuple<string, string>(line.Trim(), string.Empty);
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
                        return ParseKeyValue(s);
                    }).ToArray();
                foreach (var option in options)
                {
                    if (option.Item1 == "File")
                    {
                        font.FontFileName = option.Item2;
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
