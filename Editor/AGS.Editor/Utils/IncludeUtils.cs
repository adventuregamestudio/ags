using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace AGS.Editor
{
    /// <summary>
    /// IncludeUtils deal with the include/exclude list, containing patterns
    /// in the .gitignore format, which may instruct to either include or
    /// exclude particular file path.
    /// The code is based on the Python's fnmatch, and a gist demonstrating
    /// its port to C++, see:
    /// https://github.com/python/cpython/blob/main/Lib/fnmatch.py
    /// https://gist.github.com/alco/1869512
    /// NOTE: the level of support for .gitignore format is not clear,
    /// maybe not all of the possible edge cases are handled here.
    /// For the reference, .gitignore specs:
    /// https://git-scm.com/docs/gitignore
    /// </summary>
    public static class IncludeUtils
    {
        public enum PatternType
        {
            Include,
            Exclude
        };

        /// <summary>
        /// MatchOption flags tell how to interpret pattern and item contents.
        /// </summary>
        [Flags]
        public enum MatchOption
        {
            None = 0,
            /// <summary>
            /// Will ignore item's letter case
            /// </summary>
            CaseInsensitive = 0x0001
        }

        /// <summary>
        /// Describes a single match pattern, which instructs to either include
        /// or excluding matching items. Contains a prepared Regex object.
        /// </summary>
        public struct Pattern
        {
            public readonly PatternType Type;
            public readonly Regex Regex;
            public readonly string OriginalPattern;
            public readonly string RegexPattern;

            public Pattern(PatternType type, Regex rx, string originalPattern, string regexPattern)
            {
                Type = type;
                Regex = rx;
                OriginalPattern = originalPattern;
                RegexPattern = regexPattern;
            }
        }

        /// <summary>
        /// Creates a list of Patterns by parsing Stream contents as a text.
        /// </summary>
        public static Pattern[] CreatePatternList(Stream stream, MatchOption option = MatchOption.None)
        {
            var lines = ReadPatternLines(stream);
            return ParsePatterns(lines, option).ToArray();
        }

        /// <summary>
        /// Creates a list of Patterns by parsing a string.
        /// </summary>
        public static Pattern[] CreatePatternList(string str, MatchOption option = MatchOption.None)
        {
            var lines = ReadPatternLines(str);
            return ParsePatterns(lines, option).ToArray();
        }

        /// <summary>
        /// Filters a list of items using a list of patterns.
        /// Returns a filtered list.
        /// </summary>
        public static string[] FilterItemList(string[] items, Pattern[] patterns, MatchOption option = MatchOption.None)
        {
            // if a file entry matches no pattern of the include type it should not be included in the list
            // if a file entry matches a pattern of the include type it should be included unless a pattern of exclude type will match it after
            // if a pattern of exclude type exists but a later pattern causes it to be included, it should be included.
            List<string> matches = new List<string>();
            foreach (string item in items)
            {
                bool include = false;
                string matchItem = item;
                if (option.HasFlag(MatchOption.CaseInsensitive))
                    matchItem = matchItem.ToLowerInvariant();
                matchItem = NormalizePathSeparatorsInItem(matchItem);

                foreach (var pattern in patterns)
                {
                    if (pattern.Regex.IsMatch(matchItem))
                    {
                        if (pattern.Type == PatternType.Include)
                        {
                            include = true;
                        }
                        else if (pattern.Type == PatternType.Exclude)
                        {
                            include = false;
                        }
                    }
                }

                if (include)
                {
                    matches.Add(item);
                }
            }
            return matches.ToArray();
        }

        private static List<string> ReadPatternLines(Stream stream)
        {
            using (TextReader textReader = new StreamReader(stream))
                return ReadPatternLines(textReader);
        }

        private static List<string> ReadPatternLines(string str)
        {
            using (TextReader textReader = new StringReader(str))
                return ReadPatternLines(textReader);
        }

        private static List<string> ReadPatternLines(TextReader textReader)
        {
            List<string> lines = new List<string>();
            for (string line = textReader.ReadLine(); line != null; line = textReader.ReadLine())
            {
                lines.Add(line);
            }
            return lines;
        }

        /// <summary>
        /// Parses the given list of lines and creates a list of Patterns.
        /// </summary>
        private static List<Pattern> ParsePatterns(List<string> lines, MatchOption option)
        {
            List<Pattern> patterns = new List<Pattern>();
            foreach (string line in lines)
            {
                string parseLine = line.Trim();

                // line is empty or a comment
                if (string.IsNullOrEmpty(parseLine) || parseLine.StartsWith("#"))
                    continue;

                PatternType type = PatternType.Include;
                if (parseLine.StartsWith("!"))
                {
                    type = PatternType.Exclude;
                    parseLine = parseLine.Substring(1);
                }

                if (option.HasFlag(MatchOption.CaseInsensitive))
                    parseLine = parseLine.ToLowerInvariant();
                parseLine = NormalizePathSeparatorsInPattern(parseLine);
                string regexString = PatternToRegexString(parseLine, option);

                Regex rx = null;
                try
                {
                    rx = new Regex(regexString, RegexOptions.None);
                }
                catch (Exception)
                {
                }

                if (rx != null)
                {
                    Pattern p = new Pattern(type, rx, parseLine, regexString);
                    patterns.Add(p);
                }
            }
            return patterns;
        }

        /// <summary>
        /// Normalizes path separators in a input filepath item.
        /// </summary>
        private static string NormalizePathSeparatorsInItem(string pattern)
        {
            return pattern.Replace("\\", "/"); // convert to UNIX paths
        }

        /// <summary>
        /// Normalizes path separators in the pattern string.
        /// </summary>
        private static string NormalizePathSeparatorsInPattern(string pattern)
        {
            return pattern.Replace("\\\\", "/"); // convert to UNIX paths
        }

        /// <summary>
        /// Converts a include/exclude pattern string into the regex string,
        /// which may be used to construct a Regex object.
        /// </summary>
        private static string PatternToRegexString(string pattern, MatchOption option)
        {
            if (string.IsNullOrEmpty(pattern))
                return string.Empty;

            StringBuilder result = new StringBuilder();
            // The pattern must match the path section either at the beginning or right after the path separator
            if (!pattern.StartsWith("/"))
            {
                result.Append("(^|/)");
            }

            int i = 0;
            int n = pattern.Length;
            while (i < n)
            {
                char c = pattern[i++];
                if (c == '*')
                {
                    result.Append(".*");
                }
                else if (c == '?')
                {
                    result.Append('.');
                }
                else if (c == '[')
                {
                    int j = i;
                    /*
                     * The following two statements check if the sequence we stumbled
                     * upon is '[]' or '[!]' because those are not valid character
                     * classes.
                     */
                    if (j < n && pattern[j] == '!')
                        ++j;
                    if (j < n && pattern[j] == ']')
                        ++j;
                    /*
                     * Look for the closing ']' right off the bat. If one is not found,
                     * escape the opening '[' and continue.  If it is found, process
                     * the contents of '[...]'.
                     */
                    while (j < n && pattern[j] != ']')
                        ++j;

                    if (j >= n)
                    {
                        result.Append("\\[");
                    }
                    else
                    {
                        String part = pattern.Substring(i, j - i);
                        part.Replace("\\", " \\\\");
                        char first_char = pattern[i];
                        i = j + 1;
                        result.Append("[");
                        if (first_char == '!')
                        {
                            result.Append("^");
                            result.Append(part.Substring(1));
                        }
                        else if (first_char == '^')
                        {
                            result.Append("\\");
                            result.Append(part);
                        }
                        else
                        {
                            result.Append(part);
                        }
                        result.Append("]");
                    }
                }
                else
                {
                    if (char.IsLetterOrDigit(c) || c == '_')
                    {
                        result.Append(c);
                    }
                    else
                    {
                        // The python approach is to escape all characters that are not alphanumeric
                        result.Append("\\");
                        result.Append(c);
                    }
                }
            }

            // The pattern must match the path section either at the end or right before the path separator
            if (!pattern.EndsWith("/"))
            {
                result.Append("($|/)");
            }
            return result.ToString();
        }
    }
}
