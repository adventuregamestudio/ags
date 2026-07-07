using System;
using System.IO;
using System.Text;

namespace AGS.CommandLine.Services
{
    /// <summary>
    /// Reads and appends to AGS script (.asc) files atomically.
    /// </summary>
    public static class ScriptEditor
    {
        // UTF-8 without BOM — AGS Script compiler rejects files that start with a BOM marker
        private static readonly Encoding _utf8NoBom = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false);
        /// <summary>
        /// Append a function stub to the end of a script file.
        /// Writes to a temp file first, then replaces the original.
        /// Returns the 1-based line number where the function starts.
        /// </summary>
        public static int AppendFunction(
            string scriptFilePath,
            string functionName,
            string returnType,
            string parameters,
            string body)
        {
            if (!File.Exists(scriptFilePath))
                throw new FileNotFoundException(
                    string.Format("Script file not found: {0}", scriptFilePath));

            string existing = File.ReadAllText(scriptFilePath, _utf8NoBom);

            // Ensure file ends with a blank line before appending
            if (!existing.EndsWith("\n")) existing += "\r\n";
            existing += "\r\n";

            string stub = string.Format(
                "{0} {1}({2})\r\n{{\r\n    {3}\r\n}}\r\n",
                returnType,
                functionName,
                parameters,
                string.IsNullOrEmpty(body)
                    ? string.Format("// TODO: implement {0}", functionName)
                    : body);

            int startLine = CountLines(existing) + 1;
            string newContent = existing + stub;

            // Atomic write
            string tempPath = scriptFilePath + ".tmp";
            File.WriteAllText(tempPath, newContent, _utf8NoBom);
            File.Delete(scriptFilePath);
            File.Move(tempPath, scriptFilePath);

            return startLine;
        }

        /// <summary>
        /// Check whether a function with the given name is defined in a script file.
        /// Uses simple regex-style scan — sufficient for AGS Script.
        /// </summary>
        public static bool FunctionExists(string scriptFilePath, string functionName)
        {
            if (!File.Exists(scriptFilePath)) return false;
            string content = File.ReadAllText(scriptFilePath, _utf8NoBom);
            // Match: word boundary + functionName + optional whitespace + (
            return System.Text.RegularExpressions.Regex.IsMatch(
                content,
                string.Format(@"\b{0}\s*\(", System.Text.RegularExpressions.Regex.Escape(functionName)));
        }

        /// <summary>
        /// Replace the body of an existing function in a script file.
        /// Performs brace-aware parsing to handle braces in strings and comments.
        /// Returns true on success; errorMessage is null on success, set on failure.
        /// </summary>
        public static bool ReplaceFunction(
            string scriptFilePath,
            string functionName,
            string newBody,
            out string errorMessage)
        {
            if (!File.Exists(scriptFilePath))
            {
                errorMessage = string.Format("Script file not found: {0}", scriptFilePath);
                return false;
            }

            string content = File.ReadAllText(scriptFilePath, _utf8NoBom);

            // Find the function signature line (function_name followed by parentheses)
            var regex = new System.Text.RegularExpressions.Regex(
                string.Format(@"\b{0}\s*\(", System.Text.RegularExpressions.Regex.Escape(functionName)));

            var match = regex.Match(content);
            if (!match.Success)
            {
                errorMessage = string.Format("Function '{0}' not found in script.", functionName);
                return false;
            }

            // Start position of function name
            int startPos = match.Index;

            // Find the opening brace of this function's body
            int openBracePos = FindOpeningBraceAfterPosition(content, startPos);
            if (openBracePos == -1)
            {
                errorMessage = string.Format("Could not find opening brace for function '{0}'.", functionName);
                return false;
            }

            // Find the matching closing brace (brace-aware)
            int closeBracePos = FindMatchingClosingBrace(content, openBracePos);
            if (closeBracePos == -1)
            {
                errorMessage = string.Format("Could not find matching closing brace for function '{0}'.", functionName);
                return false;
            }

            // Reconstruct: everything before the opening brace + new body + everything after the closing brace
            string before = content.Substring(0, openBracePos + 1); // Include the opening brace
            string after = content.Substring(closeBracePos);         // Include the closing brace

            // Format the new body with proper indentation
            string formattedBody = FormatFunctionBody(newBody);
            string newContent = before + formattedBody + after;

            // Atomic write
            string tempPath = scriptFilePath + ".tmp";
            File.WriteAllText(tempPath, newContent, _utf8NoBom);
            File.Delete(scriptFilePath);
            File.Move(tempPath, scriptFilePath);

            errorMessage = null;
            return true;
        }

        /// <summary>
        /// Find the position of the opening brace '{' after a given position.
        /// This is brace-aware: skips braces inside strings and comments.
        /// </summary>
        private static int FindOpeningBraceAfterPosition(string content, int startPos)
        {
            for (int i = startPos; i < content.Length; i++)
            {
                char c = content[i];

                // Skip single-line comments
                if (c == '/' && i + 1 < content.Length && content[i + 1] == '/')
                {
                    // Find end of line
                    while (i < content.Length && content[i] != '\n')
                        i++;
                    continue;
                }

                // Skip multi-line comments
                if (c == '/' && i + 1 < content.Length && content[i + 1] == '*')
                {
                    i += 2;
                    while (i + 1 < content.Length && !(content[i] == '*' && content[i + 1] == '/'))
                        i++;
                    i += 2;
                    continue;
                }

                // Skip strings
                if (c == '"')
                {
                    i++;
                    while (i < content.Length)
                    {
                        if (content[i] == '\\' && i + 1 < content.Length)
                        {
                            i += 2; // Skip escaped character
                        }
                        else if (content[i] == '"')
                        {
                            i++;
                            break;
                        }
                        else
                        {
                            i++;
                        }
                    }
                    continue;
                }

                // Skip character literals
                if (c == '\'')
                {
                    i++;
                    while (i < content.Length)
                    {
                        if (content[i] == '\\' && i + 1 < content.Length)
                        {
                            i += 2;
                        }
                        else if (content[i] == '\'')
                        {
                            i++;
                            break;
                        }
                        else
                        {
                            i++;
                        }
                    }
                    continue;
                }

                if (c == '{')
                    return i;
            }

            return -1;
        }

        /// <summary>
        /// Find the position of the closing brace '}' that matches an opening brace at position openBracePos.
        /// This is brace-aware: properly handles nested braces and skips braces inside strings/comments.
        /// </summary>
        private static int FindMatchingClosingBrace(string content, int openBracePos)
        {
            int braceCount = 1;
            int i = openBracePos + 1;

            while (i < content.Length && braceCount > 0)
            {
                char c = content[i];

                // Skip single-line comments
                if (c == '/' && i + 1 < content.Length && content[i + 1] == '/')
                {
                    while (i < content.Length && content[i] != '\n')
                        i++;
                    continue;
                }

                // Skip multi-line comments
                if (c == '/' && i + 1 < content.Length && content[i + 1] == '*')
                {
                    i += 2;
                    while (i + 1 < content.Length && !(content[i] == '*' && content[i + 1] == '/'))
                        i++;
                    if (i + 1 < content.Length)
                        i += 2;
                    continue;
                }

                // Skip strings
                if (c == '"')
                {
                    i++;
                    while (i < content.Length)
                    {
                        if (content[i] == '\\' && i + 1 < content.Length)
                        {
                            i += 2;
                        }
                        else if (content[i] == '"')
                        {
                            i++;
                            break;
                        }
                        else
                        {
                            i++;
                        }
                    }
                    continue;
                }

                // Skip character literals
                if (c == '\'')
                {
                    i++;
                    while (i < content.Length)
                    {
                        if (content[i] == '\\' && i + 1 < content.Length)
                        {
                            i += 2;
                        }
                        else if (content[i] == '\'')
                        {
                            i++;
                            break;
                        }
                        else
                        {
                            i++;
                        }
                    }
                    continue;
                }

                // Count braces
                if (c == '{')
                {
                    braceCount++;
                }
                else if (c == '}')
                {
                    braceCount--;
                    if (braceCount == 0)
                        return i;
                }

                i++;
            }

            return braceCount == 0 ? i : -1;
        }

        /// <summary>
        /// Format the function body with proper indentation (4 spaces per line).
        /// Preserves blank lines but trims leading/trailing whitespace.
        /// </summary>
        private static string FormatFunctionBody(string body)
        {
            if (string.IsNullOrWhiteSpace(body))
                return "\r\n";

            // Split into lines
            string[] lines = body.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.None);

            // Trim each line and add indentation
            StringBuilder formatted = new StringBuilder();
            formatted.Append("\r\n");

            for (int i = 0; i < lines.Length; i++)
            {
                string line = lines[i].TrimEnd();

                if (string.IsNullOrWhiteSpace(line))
                {
                    formatted.Append("\r\n");
                }
                else
                {
                    formatted.AppendFormat("    {0}\r\n", line.TrimStart());
                }
            }

            return formatted.ToString();
        }

        /// <summary>
        /// Check whether a global variable with the given name is already declared in a script file.
        /// Matches patterns like:  int varName;   int varName =   bool varName;   String varName =
        /// Uses a word-boundary regex so "myVar" does not match "myVarOther".
        /// </summary>
        public static bool VariableExists(string scriptFilePath, string varName)
        {
            if (!File.Exists(scriptFilePath)) return false;
            string content = File.ReadAllText(scriptFilePath, _utf8NoBom);
            // Match: word_boundary + varName + optional whitespace + (;  or  =  or  [)
            return System.Text.RegularExpressions.Regex.IsMatch(
                content,
                string.Format(@"\b{0}\s*[;=\[]", System.Text.RegularExpressions.Regex.Escape(varName)));
        }

        /// <summary>
        /// Prepend a global variable declaration at the top of the script file.
        /// Emits:  varType varName;\r\n  as the very first line (before any existing content).
        /// </summary>
        public static void DeclareGlobalVariable(string scriptFilePath, string varType, string varName)
        {
            if (!File.Exists(scriptFilePath))
                throw new FileNotFoundException(
                    string.Format("Script file not found: {0}", scriptFilePath));

            string existing = File.ReadAllText(scriptFilePath, _utf8NoBom);

            // Build the declaration line and prepend it
            string declaration = string.Format("{0} {1};\r\n", varType, varName);
            string newContent = declaration + existing;

            // Atomic write
            string tempPath = scriptFilePath + ".tmp";
            File.WriteAllText(tempPath, newContent, _utf8NoBom);
            File.Delete(scriptFilePath);
            File.Move(tempPath, scriptFilePath);
        }

        private static int CountLines(string text)
        {
            int count = 1;
            foreach (char c in text)
                if (c == '\n') count++;
            return count;
        }
    }
}
