using System;
using System.Text;
using System.Text.RegularExpressions;

namespace AGS.Editor
{
    public static class TextUtils
    {
        /// <summary>
        /// This *double escapes* an escaped '[' character (old-style linebreak,
        /// which must be escaped by user if they want a literal '[' in text).
        /// This is required before doing standard unescaping for this line,
        /// for in such case "\[" will be treated as a unknown escape sequence,
        /// while "\\[" will be converted to "\[" by merging "\\" pair.
        /// </summary>
        public static string PreprocessLineForOldStyleLinebreaks(string text)
        {
            // Lookup for any "\x" pairs; this includes "\\" too,
            // and lets us skip all escaped escape chars.
            var matches = Regex.Matches(text, @"(\\.)");
            if (matches.Count == 0)
                return text; // nothing to escape

            StringBuilder sb = new StringBuilder();
            int lastSrcIndex = 0;
            foreach (Match m in matches)
            {
                sb.Append(text.Substring(lastSrcIndex, m.Index - lastSrcIndex));
                if (m.Value == "\\[")
                    sb.Append("\\\\["); // double escape escaped bracket
                else
                    sb.Append(m.Value); // copy any other escape sequence without a change
                lastSrcIndex = m.Index + m.Length;
            }
            if (lastSrcIndex < text.Length)
                sb.Append(text.Substring(lastSrcIndex));
            return sb.ToString();
        }
    }
}
