using System;
using System.Text;
using System.Text.RegularExpressions;

namespace AGS.Editor
{
    /// <summary>
    /// Utilities for game upgrade process.
    /// TODO: move anything here that might require unit tests.
    /// </summary>
    public static class UpgradeGameUtils
    {
        /// <summary>
        /// Converts old style linebreak "[" to "\\n" (pseudo-escaped for displaying in properties).
        /// Converts escaped sequence "\[" to "[", since it no longer has to be escaped.
        /// </summary>
        public static string ConvertOldStyleLinebreak(string text)
        {
            var matches = Regex.Matches(text, @"(\\.)|(\[)");
            if (matches.Count == 0)
                return text;

            StringBuilder sb = new StringBuilder();
            int lastSrcIndex = 0;
            foreach (Match m in matches)
            {
                sb.Append(text.Substring(lastSrcIndex, m.Index - lastSrcIndex));
                if (m.Value == "\\[")
                    sb.Append("[");
                else if (m.Value == "[")
                    sb.Append("\\n");
                else
                    sb.Append(m.Value);
                lastSrcIndex = m.Index + m.Length;
            }
            if (lastSrcIndex < text.Length)
                sb.Append(text.Substring(lastSrcIndex));
            return sb.ToString();
        }
    }
}
