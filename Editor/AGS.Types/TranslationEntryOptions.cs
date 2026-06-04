using System;
using System.Collections.Generic;

namespace AGS.Types
{
    /// <summary>
    /// This is an extension of the translation entry, which is currently
    /// represented as a key/value strings in Translation class. We cannot
    /// replace the translation entry type in 3.*, because it's used by
    /// the Editor plugins. Therefore any additional data has to be added
    /// as a separate struct.
    /// </summary>
    public class TranslationEntryOptions
    {
        private bool _isObsolete = false;
        private int _parserWordID = -1;

        public TranslationEntryOptions()
        {
            Metadata = new List<string>();
        }

        public bool IsObsolete { get { return _isObsolete; } set { _isObsolete = true; } }

        /// <summary>
        /// Whether this translation item belongs to the Text Parser's words dictionary.
        /// </summary>
        public bool IsParserDictionary { get { return ParserWordID >= 0; } }

        /// <summary>
        /// Text Parser's word ID, which this translation item corresponds to.
        /// -1 if does not correspond to any.
        /// </summary>
        public int ParserWordID { get { return _parserWordID; } set { _parserWordID = value; } }

        /// <summary>
        /// Any kind of annotations for this translation item.
        /// </summary>
        public List<string> Metadata { get; set; }
    }
}
