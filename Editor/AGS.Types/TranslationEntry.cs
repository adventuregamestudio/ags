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
            Metadata = new List<string>();
        }

        public string Context { get; set; }
        public string Key { get; set; }
        public string Value { get; set; }

        // TEMPORARY only for preserving comments and other metadata
        // Will be split into translator-comments, extracted-comments, reference, flag
        public List<string> Metadata { get; set; } 

    }
}
