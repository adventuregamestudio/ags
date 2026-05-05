using System;
using System.Collections.Generic;

namespace AGS.Types
{
    public class TranslationSection
    {
        public TranslationSection(string name)
        {
            Name = name;
        }

        public TranslationSection(string name, string comment)
        {
            Name = name;
            Comment = comment;
        }

        public string Name { get; set; }
        public string Comment { get; set; }
    }
}
