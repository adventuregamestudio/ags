using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace AGS.Types
{
    public class Translation
    {
        public const string TRANSLATION_SOURCE_FILE_EXTENSION = ".po";
        public const string TRANSLATION_COMPILED_FILE_EXTENSION = ".tra";

        private const string NORMAL_FONT_TAG = "# $NormalFont=";
        private const string SPEECH_FONT_TAG = "# $SpeechFont=";
        private const string TEXT_DIRECTION_TAG = "# $TextDirection=";
        private const string ENCODING_TAG = "# $Encoding=";
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
        private Dictionary<string, TranslationEntry> _translatedLines;

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
                TranslatedLines.Clear(); // clear on failure
            }
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

        public Dictionary<string, TranslationEntry> TranslatedLines
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
                sw.WriteLine("# $NormalFont=" + WriteOptionalInt(_normalFont));
                sw.WriteLine("# The speech font to use - DEFAULT or font number");
                sw.WriteLine("# $SpeechFont=" + WriteOptionalInt(_speechFont));
                sw.WriteLine("# Text direction - DEFAULT, LEFT or RIGHT");
                sw.WriteLine("# $TextDirection=" + ((_rightToLeftText == true) ? TAG_DIRECTION_RIGHT : ((_rightToLeftText == null) ? TAG_DEFAULT : TAG_DIRECTION_LEFT)));
                sw.WriteLine("# Text encoding hint - ASCII or UTF-8");
                sw.WriteLine("# $Encoding=" + encoding);
                sw.WriteLine("#  ");
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

                foreach (string key in _translatedLines.Keys)
                {
                    TranslationEntry entry = _translatedLines[key];
                    sw.WriteLine("");
                    foreach (string metadata in entry.metadata)
                        sw.WriteLine(metadata);
                    if (entry.msgctxt != null)
                        sw.WriteLine("msgctxt \"" + entry.msgctxt + "\"");
                    sw.WriteLine("msgid \"" + entry.msgid + "\"");
                    sw.WriteLine("msgstr \"" + entry.msgstr + "\"");
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
                _translatedLines.Clear(); // clear on failure
            }
            return errors;
        }

        private void LoadDataImpl(CompileMessages errors)
        {
            _translatedLines = new Dictionary<string, TranslationEntry>();
            string old_encoding = _encodingHint;

            // .po file not found, check for legacy translation format
            if (!File.Exists(FileName) && File.Exists(_name + ".trs"))
            {
                LegacyLoadData();
                SaveData();
                return;
            }

            ParseState state = ParseState.NewEntry;
            TranslationEntry entry = new TranslationEntry();
            using (StreamReader sr = new StreamReader(FileName, _encoding))
            {
                string line;
                while ((line = sr.ReadLine()) != null)
                {
                    string extracted_string = POExtractString(line);

                    // Case 1: we got a string, figure out what to do with it
                    if (extracted_string != null)
                    {
                        // start of a new type
                        if (line.StartsWith("msgctxt")) // context
                        {
                            state = ParseState.ParsingContext;
                            entry.msgctxt = extracted_string;
                            continue;
                        }
                        if (line.StartsWith("msgid")) // string id, untranslated
                        {
                            state = ParseState.ParsingId;
                            entry.msgid = extracted_string;
                            continue;
                        }
                        if (line.StartsWith("msgstr")) // translated string
                        {
                            state = ParseState.ParsingString;
                            entry.msgstr = extracted_string;
                            continue;
                        }

                        // strings can be split on multiple lines, usually the first will be empty, but no guarantee
                        // an external tool may have done that, so let's append
                        switch (state)
                        {
                            case ParseState.ParsingContext:
                                entry.msgctxt += extracted_string;
                                continue;

                            case ParseState.ParsingId:
                                entry.msgid += extracted_string;
                                continue;

                            case ParseState.ParsingString:
                                entry.msgstr += extracted_string;
                                continue;

                            default: continue; // ignore orphaned strings since we can't throw errors
                        }
                    }


                    // Case 2: encountered metadata
                    // TODO: track different types of metadata (flags, comments, etc)
                    if (line.StartsWith("#"))
                    {
                        ReadSpecialTags(line);
                        if (string.Compare(old_encoding, _encodingHint) != 0)
                        {
                            sr.Close();
                            LoadDataImpl(errors); // try again with the new encoding
                            return;
                        }
                        entry.metadata.Add(line);
                        continue;
                    }


                    // Case 3: we were processing an entry and encountered an empty line
                    if (state != ParseState.NewEntry && line.Trim().Length == 0)
                    {
                        // note: a valid entry must have a non-empty msgid
                        // the first empty hardcoded entry, which is metadata, is ignored and recreated
                        if (entry.msgid != null && entry.msgid.Length > 0)
                        {
                            _translatedLines.Add(entry.msgid, entry);
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
            // TODO: make a generic dictionary instead and save any option
            else if (line.StartsWith(ENCODING_TAG))
            {
                EncodingHint = line.Substring(ENCODING_TAG.Length);
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

        // Migrations from .trs files
        private void LegacyLoadData()
        {
            _translatedLines = new Dictionary<string, TranslationEntry>();
            string old_encoding = _encodingHint;

            using (StreamReader sr = new StreamReader(_name + ".trs", _encoding))
            {
                string line;
                while ((line = sr.ReadLine()) != null)
                {
                    if (line.StartsWith("//"))
                    {
                        LegacyReadSpecialTags(line);
                        if (string.Compare(old_encoding, _encodingHint) != 0)
                        {
                            sr.Close();
                            LoadData(); // try again with the new encoding
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
                        TranslationEntry entry = new TranslationEntry();
                        entry.msgid = originalText;
                        entry.msgstr = translatedText;
                        _translatedLines.Add(originalText, entry);
                    }
                }
            }
        }

        private void LegacyReadSpecialTags(string line)
        {
            const string NORMAL_FONT_TAG = "# $NormalFont=";
            const string SPEECH_FONT_TAG = "# $SpeechFont=";
            const string TEXT_DIRECTION_TAG = "# $TextDirection=";
            const string ENCODING_TAG = "# $Encoding=";
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
        }


    }
}
