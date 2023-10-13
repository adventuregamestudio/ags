using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace AGS.Editor
{
    /// <summary>
    /// Helper class ScriptGeneration provides methods for automatic script editing.
    /// </summary>
    public class ScriptGeneration
    {
        /// <summary>
        /// Counts braces starting with the startIndex, and finds the matching closing one.
        /// Returns an index of a closing brace.
        /// </summary>
        public static int FindClosingBrace(string text, int startIndex, char braceOpenChar = '{', char braceCloseChar = '}')
        {
            // TODO: is it possible to do this using regex?
            int count = 0;
            int closeBraceAt = -1;
            for (int i = startIndex; i < text.Length; ++i)
            {
                if (text[i] == braceOpenChar)
                {
                    count++;
                }
                else if (text[i] == braceCloseChar)
                {
                    count--;
                    if (count == 0)
                    {
                        closeBraceAt = i;
                        break;
                    }
                }
            }
            return closeBraceAt;
        }

        /// <summary>
        /// Appends a function with optional parameter list and optional contents to a
        /// script's text.
        /// </summary>
        /// <param name="amendExisting">Tells whether to amend the function code in existing
        /// function, or only set it if the new function is created.</param>
        /// <returns>Resulting script's text.</returns>
        public static string InsertFunction(string text, string functionName, string paramList = "", string functionCode = "",
            bool amendExisting = false, bool insertBeforeExistingCode = false)
        {
            // TODO: support matching indentation for the new code?

            // NOTE: we must find a function with opening brace, because there may also
            // be a function prototype somewhere.
            var match = Regex.Match(text, string.Format(@"function\s+{0}\s*\(.*\)\s*{{", functionName));
            if (match.Success && (!amendExisting || string.IsNullOrWhiteSpace(functionCode)))
                return text; // function already exists and don't have to amend
            if (match.Success)
            {
                // Find the required position in the existing function, and insert required code
                int functionStart = match.Index + match.Length;
                if (insertBeforeExistingCode)
                {
                    text = string.Format("{0}{1}{2}{3}{4}",
                            text.Substring(0, functionStart),
                            Environment.NewLine, functionCode, Environment.NewLine,
                            text.Substring(functionStart));
                }
                else
                {
                    int closeBraceAt = FindClosingBrace(text, match.Index);
                    if (closeBraceAt >= 0)
                    {
                        text = string.Format("{0}{1}{2}{3}{4}",
                            text.Substring(0, closeBraceAt),
                            Environment.NewLine, functionCode, Environment.NewLine,
                            text.Substring(closeBraceAt));
                    }
                    else
                    {
                        // Script missing closing brace?
                        text = string.Format("{0}{1}{2}{3}}}",
                            text, Environment.NewLine, functionCode, Environment.NewLine);
                    }
                }
            }
            else
            {
                // Add a new function to the end of the script
                text += string.Format("{0}function {1}({2}){3}{{{4}{5}{6}}}{7}",
                    Environment.NewLine, functionName, paramList, Environment.NewLine, Environment.NewLine,
                    functionCode, Environment.NewLine, Environment.NewLine);
            }
            return text;
        }
    }
}
