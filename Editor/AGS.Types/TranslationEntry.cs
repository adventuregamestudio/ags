using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AGS.Types
{
    public class TranslationEntry
    {
        public TranslationEntry()
        {
            metadata = new List<string>();
        }

        public string msgctxt { get; set; }
        public string msgid { get; set; }
        public string msgstr { get; set; }

        // TEMPORARY only for preserving comments and other metadata
        // Will be split into translator-comments, extracted-comments, reference, flag
        public List<string> metadata { get; set; } 

    }
}
