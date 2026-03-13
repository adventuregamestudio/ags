using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AGS.Editor
{
    /// <summary>
    /// A collection of utilities for AGS script parsing.
    /// </summary>
    public static class ScriptParsing
    {
        public const char NEWLINE_CHAR1 = '\n';
        public const char NEWLINE_CHAR2 = '\r';
        public const char SYMBOL_MEMBER_ACCESS = '.';

        /// <summary>
        /// Tells if the character is a valid symbol for a script keyword or a variable name.
        /// </summary>
        public static bool IsScriptWordChar(this Char theChar)
        {
            return ((theChar >= 'A') && (theChar <= 'Z')) ||
                ((theChar >= 'a') && (theChar <= 'z')) ||
                ((theChar >= '0') && (theChar <= '9')) ||
                (theChar == '_');
        }

        /// <summary>
        /// Compares two sections by their start position indexes.
        /// </summary>
        private class SectionComparer : IComparer<Tuple<int, int>>
        {
            int IComparer<Tuple<int, int>>.Compare(Tuple<int, int> x, Tuple<int, int> y)
            {
                return x.Item1 - y.Item1;
            }
        }

        /// <summary>
        /// ParserState contains information gathered during parsing of the script.
        /// It's meant to be used as an argument to some of the parsing methods.
        /// </summary>
        public class ParserState
        {
            private string _script;
            private SortedSet<Tuple<int, int>> _comments;

            public ParserState(string script)
            {
                _script = script;
                _comments = new SortedSet<Tuple<int, int>>(new SectionComparer());
            }

            public string Script { get { return _script; } }
            public SortedSet<Tuple<int, int>> Comments { get { return _comments; } }
        }

        /// <summary>
        /// Parses the whole script and fills Comment sections in.
        /// </summary>
        public static void FillCommentSections(ParserState state)
        {
            state.Comments.Clear();
            for (int index = 0; index < state.Script.Length;)
            {
                int nextIndex = SkipComments(state, index);
                index = (nextIndex == index) ? index + 1 : nextIndex;
            }
        }

        /// <summary>
        /// Assumes that we are inside an expression, which *may* or *may not* be
        /// inside a function argument list. Backtraces the script until it finds
        /// either a matching opening bracket, or a beginning of expression
        /// (or rather end of the previous expression).
        /// Returns the index of the opening bracket, or -1 if one was not found.
        /// </summary>
        public static int BacktraceToOpenBracket(ParserState state, int backFromIndex)
        {
            string script = state.Script;
            int bracketLevel = 1;
            for (int index = backFromIndex - 1; index >= 0; --index)
            {
                char symbol = script[index];
                if (symbol == '(')
                {
                    bracketLevel--;
                    if (bracketLevel == 0)
                        return index;
                }
                else if (symbol == ')')
                {
                    bracketLevel++;
                }
                else if (symbol == '/')
                {
                    if (index > 0 && script[index - 1] == '*')
                    {
                        index = SkipCommentBackwards(state, index - 1);
                    }
                }
                else if (symbol == NEWLINE_CHAR1 || symbol == NEWLINE_CHAR2)
                {
                    for (; index >= 0 && (script[index] == NEWLINE_CHAR1 || script[index] == NEWLINE_CHAR2); --index) ;
                    // When we reach the previous line (it appears that this is a multi-line expression),
                    // we, unfortunately, must first prescan and check if this line is commented out
                    int indexAfterComment = SkipCommentBackwards(state, index);
                    if (indexAfterComment == index)
                        index++; // compensate for --index on next iteration
                }
                else if (symbol == '\"')
                {
                    index = SkipStringLiteralBackwards(script, index);
                }
                else if (symbol == '{' || symbol == '}' || symbol == ';')
                {
                    return -1;
                }
            }
            return -1;
        }

        public static int BacktraceScriptSymbol(string script, int backFromIndex)
        {
            int index = backFromIndex - 1;
            for (; index >= 0 && char.IsWhiteSpace(script[index]); --index) ;
            for (; index >= 0 && script[index].IsScriptWordChar(); --index) ;
            return index + 1;
        }

        public static int BacktraceScriptSymbolComposite(string script, int backFromIndex)
        {
            int index = backFromIndex - 1;
            int lastValidIndex = index;
            do
            {
                for (; index >= 0 && char.IsWhiteSpace(script[index]); --index) ;
                for (; index >= 0 && script[index].IsScriptWordChar(); --index) ;
                lastValidIndex = index;
                for (; index >= 0 && char.IsWhiteSpace(script[index]); --index) ;
            }
            while (script[index--] == SYMBOL_MEMBER_ACCESS);
            return lastValidIndex + 1;
        }

        public static string GetCurrentFunctionCall(ParserState state, int backFromIndex, bool lookupForOuterCall = false)
        {
            int argListStart = -1, lastFoundOpenBracket, searchFrom = backFromIndex;
            do
            {
                lastFoundOpenBracket = BacktraceToOpenBracket(state, searchFrom);
                if (lastFoundOpenBracket >= 0)
                {
                    argListStart = lastFoundOpenBracket;
                    searchFrom = lastFoundOpenBracket;
                }
            }
            while (lookupForOuterCall && lastFoundOpenBracket >= 0);

            if (argListStart >= 0)
            {
                int exprStartAt = BacktraceScriptSymbolComposite(state.Script, argListStart);
                if (exprStartAt >= 0)
                {
                    return state.Script.Substring(exprStartAt, (backFromIndex - exprStartAt));
                }
            }

            return string.Empty;
        }

        /// <summary>
        /// Skip comments in the script that might have speech marks
        /// in them, which could confuse the parser.
        /// </summary>
        public static int SkipComments(ParserState state, int index)
        {
            string script = state.Script;
            if ((index < script.Length - 1) &&
                (script[index] == '/') && (script[index + 1] == '/'))
            {
                int startIndex = index;
                for (; (index < script.Length) &&
                    (script[index] != NEWLINE_CHAR1) &&
                    (script[index] != NEWLINE_CHAR2);
                    ++index) ;
                int endIndex = index;
                var section = new Tuple<int, int>(startIndex, endIndex);
                if (!state.Comments.Contains(section))
                    state.Comments.Add(section);
            }
            if ((index < script.Length - 1) &&
                (script[index] == '/') && (script[index + 1] == '*'))
            {
                int startIndex = index;
                index += 2;
                for (; index < script.Length - 1; ++index)
                {
                    if ((script[index] == '*') && (script[index + 1] == '/'))
                    {
                        index += 2;
                        break;
                    }
                }
                int endIndex = index;
                var section = new Tuple<int, int>(startIndex, endIndex);
                if (!state.Comments.Contains(section))
                    state.Comments.Add(section);
            }
            return index;
        }

        /// <summary>
        /// Backtraces script starting with the given index (exclusive),
        /// looking up the list of comment blocks. If the given index
        /// is inside any comment, then returns the index of the comment's
        /// start. If it's not, then returns same index.
        /// </summary>
        public static int SkipCommentBackwards(ParserState state, int backFromIndex)
        {
            var view = state.Comments.GetViewBetween(new Tuple<int, int>(0, 0), new Tuple<int, int>(backFromIndex, 0));
            foreach (var comment in view.Reverse())
            {
                if (backFromIndex >= comment.Item2)
                    return backFromIndex; // not inside comment
                if (comment.Item1 <= backFromIndex && backFromIndex < comment.Item2)
                    return comment.Item1;
            }
            return backFromIndex;
        }

        /// <summary>
        /// Backtraces script starting with the given index (exclusive),
        /// *assuming* that we are inside a *proper* string literal,
        /// until we find the opening double quotes char ('\"').
        /// Returns index of the opening double quotes, or -1 if one
        /// was not found until we reached the start of the line.
        /// </summary>
        public static int SkipStringLiteralBackwards(string script, int backFromIndex, int scriptStartAt = 0)
        {
            for (int index = backFromIndex - 1; index >= scriptStartAt; --index)
            {
                if (script[index] == '\"')
                {
                    return index;
                }
            }
            return -1;
        }
    }
}
