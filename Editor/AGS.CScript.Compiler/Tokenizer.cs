using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class Tokenizer : ITokenizer
    {
        private bool _endSymbolOnNextCharacter = false;
        private bool _nextCharacterIsEscaped = false;

        public TokenizedScript TokenizeScript(string scriptToProcess)
        {
            TokenizedScript output = new TokenizedScript();

            int lineNumber = 1;
            FastString script = new FastString(scriptToProcess);
            while (script.Length > 0)
            {
                SkipWhitespace(ref script);

                if (script.Length == 0)
                {
                    break;
                }
                else if ((script[0] == '\r') || (script[0] == '\n'))
                {
                    output.WriteNewLineNumber(lineNumber);
                    lineNumber++;

                    if ((script.Length > 1) && (script[0] == '\r') && (script[1] == '\n'))
                    {
                        script = script.Substring(2);
                    }
                    else
                    {
                        script = script.Substring(1);
                    }
                }
                else
                {
                    int symbolStart = 0;
                    int i = 1;
                    while (IsPartOfSameSymbol(script, symbolStart, i))
                    {
                        i++;
                    }

                    string thisSymbol = script.Substring(symbolStart, i);
                    output.WriteToken(thisSymbol);
                    if (thisSymbol.StartsWith(Constants.NEW_SCRIPT_MARKER))
                    {
                        lineNumber = 1;
                    }
                    if (i < script.Length)
                    {
                        script = script.Substring(i);
                    }
                    else
                    {
                        break;
                    }
                }
            }

            output.WriteEndOfStream();
            return output;
        }

        private bool IsPartOfSameSymbol(FastString script, int startIndex, int checkIndex)
        {
            if (checkIndex >= script.Length)
            {
                return false;
            }
            char startChar = script[startIndex];
            char thisChar = script[checkIndex];

            if (_endSymbolOnNextCharacter)
            {
                _endSymbolOnNextCharacter = false;
                return false;
            }

            if ((startChar == '\"') || (startChar == '\''))
            {
                if (_nextCharacterIsEscaped)
                {
                    // an escaped " or whatever, so let it through
                    _nextCharacterIsEscaped = false;
                }
                else if (thisChar == '\\')
                {
                    _nextCharacterIsEscaped = true;
                }
                else if (thisChar == startChar)
                {
                    // Force the string to end on the next character
                    _endSymbolOnNextCharacter = true;
                }
                return true;
            }

            if (Char.IsDigit(startChar))
            {
                if (Char.IsDigit(thisChar))
                {
                    return true;
                }
                // float constant
                if (thisChar == '.')
                {
                    return true;
                }
                return false;
            }

            if (Char.IsLetter(startChar) || (startChar == '_'))
            {
                return (Char.IsLetterOrDigit(thisChar) || (thisChar == '_'));
            }

            // ==, >=, <=, !=, etc
            if (thisChar == '=')
            {
                if ((startChar == '=') || (startChar == '<') || (startChar == '>') ||
                    (startChar == '!') || (startChar == '+') || (startChar == '-'))
                {
                    return true;
                }
            }

            // && and ||, ++ and --
            if ((thisChar == '&') && (startChar == '&')) return true;
            if ((thisChar == '|') && (startChar == '|')) return true;
            if ((thisChar == '+') && (startChar == '+')) return true;
            if ((thisChar == '-') && (startChar == '-')) return true;
            // << and >>
            if ((thisChar == '<') && (startChar == '<')) return true;
            if ((thisChar == '>') && (startChar == '>')) return true;
            // ...
            if ((thisChar == '.') && (startChar == '.')) return true;
            // ::
            if ((thisChar == ':') && (startChar == ':')) return true;

            return false;
        }

        private static void SkipWhitespace(ref FastString script)
        {
            while (IsWhitespace(script[0]))
            {
                script = script.Substring(1);
                if (script.Length == 0)
                {
                    break;
                }
            }
        }

        private static bool IsWhitespace(char cht)
        {
            return ((cht == ' ') || (cht == 9) || (cht == 26) || (cht == 11) || (cht == 0));
        }
    }
}
