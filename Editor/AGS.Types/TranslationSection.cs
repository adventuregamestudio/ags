using System;
using System.Collections.Generic;

namespace AGS.Types
{
    public class TranslationSection
    {
        public TranslationSection(string name)
        {
            Name = name;
            TranslationEntryKeys = new HashSet<string>();
        }

        public TranslationSection(string name, string comment)
        {
            Name = name;
            Comment = comment;
            TranslationEntryKeys = new HashSet<string>();
        }

        public string Name { get; set; }
        public string Comment { get; set; }
        public HashSet<string> TranslationEntryKeys { get; set; }
    }
}
