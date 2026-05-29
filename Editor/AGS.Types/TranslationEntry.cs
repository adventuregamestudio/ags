using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AGS.Types
{
    public class TranslationEntry
    {
        private const string CONTEXT_PARSERWORD = "PARSERWORD";

        private string _context = null;
        private int _parserWordID = -1;

        public TranslationEntry()
        {
            Key = string.Empty;
            Value = string.Empty;
            Metadata = new List<string>();
        }

        public TranslationEntry(string key)
        {
            Key = key;
            Value = string.Empty;
            Metadata = new List<string>();
        }

        public TranslationEntry(string key, string value)
        {
            Key = key;
            Value = value;
            Metadata = new List<string>();
        }

        public string Context
        {
            get { return _context; }
            set
            {
                _context = value;
                if (!string.IsNullOrEmpty(_context))
                {
                    if (_context.StartsWith(CONTEXT_PARSERWORD))
                    {
                        var keyValue = Utilities.ParseKeyValue(_context, ':');
                        if (keyValue.Key == CONTEXT_PARSERWORD)
                        {
                            _parserWordID = Utilities.ParseIntOrDefault(keyValue.Value, -1);
                        }
                    }
                }
            }
        }
        public string Key { get; set; }
        public string Value { get; set; }

        /// <summary>
        /// Whether this translation item belongs to the Text Parser's words dictionary.
        /// </summary>
        public bool IsParserDictionary { get { return ParserWordID >= 0; } }

        /// <summary>
        /// Text Parser's word ID, which this translation item corresponds to.
        /// -1 if does not correspond to any.
        /// </summary>
        public int ParserWordID
        {
            get
            {
                return _parserWordID;
            }
            set
            {
                _parserWordID = value;
                _context = $"PARSERWORD:{_parserWordID}";
            }
        }

        /// <summary>
        /// Any kind of comments and annotations for this translation item.
        /// </summary>
        public List<string> Metadata { get; set; } 
    }
}
