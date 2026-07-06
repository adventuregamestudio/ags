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
        public const char SYM_LINEFEED = '\n';
        public const char SYM_CARETRETURN = '\r';
        public const char SYM_MEMBER_ACCESS = '.';

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
            private int _baseLine = 0;
            private List<int> _lines;
            private SortedSet<Tuple<int, int>> _comments;
            private int _index = 0;
            private char _curChar = (char)0;

            public ParserState(string script, int baseLine = 0)
            {
                _script = script;
                _baseLine = baseLine;
                _lines = new List<int>();
                _comments = new SortedSet<Tuple<int, int>>(new SectionComparer());
                _curChar = !string.IsNullOrEmpty(_script) ? _script[0] : (char)0;
            }

            public string Script { get { return _script; } }
            public List<int> Lines { get { return _lines; } }
            public SortedSet<Tuple<int, int>> Comments { get { return _comments; } }

            public int CharIndex { get { return _index; } }
            public bool AtEnd { get { return _index == _script.Length; } }
            public bool AtLastChar { get { return _index == _script.Length - 1; } }

            public char Char { get { return _curChar; } }

            public int Line { get { return FindLineForCharIndex(_index); } }

            public int FindLineForCharIndex(int index)
            {
                int line = _lines.BinarySearch(index);
                if (line < 0)
                    line = ~line - 1; // returned bitwise complement of the index of the next element that is larger than item
                return line + _baseLine;
            }

            public void Reset()
            {
                _index = 0;
            }

            public char PeekChar()
            {
                return (_index < _script.Length - 1) ? _script[_index + 1] : (char)0;
            }

            public int Forward()
            {
                if (_index < _script.Length)
                {
                    _index++;
                    if (_index < _script.Length)
                        _curChar = _script[_index];
                }
                return _index;
            }

            public int Forward2()
            {
                _index = Math.Min(_index + 2, _script.Length);
                if (_index < _script.Length)
                    _curChar = _script[_index];
                return _index;
            }

            public int Back()
            {
                if (_index > 0)
                {
                    _index--;
                    _curChar = _script[_index];
                }
                return _index;
            }

            public int SetAt(int index)
            {
                if (index >= 0 && index < _script.Length)
                {
                    _index = index;
                    _curChar = _script[_index];
                }
                return _index;
            }
        }

        /// <summary>
        /// Parses the whole script and fills Lines set.
        /// </summary>
        public static void FillLines(ParserState state)
        {
            state.Lines.Clear();
            for (; !state.AtEnd; SkipLine(state))
            {
                state.Lines.Add(state.CharIndex);
            }
        }

        /// <summary>
        /// Parses the whole script and fills Comment sections in.
        /// </summary>
        public static void FillCommentSections(ParserState state)
        {
            state.Comments.Clear();
            while (!state.AtEnd)
            {
                SeekToComment(state);
                SkipComments(state);
            }
        }

        public static void SkipLine(ParserState state)
        {
            while (!state.AtEnd)
            {
                // Regardless whether CRLF or LF, LF comes last
                if (state.Char == SYM_LINEFEED)
                {
                    state.Forward();
                    return;
                }
                state.Forward();
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
                else if (symbol == SYM_LINEFEED || symbol == SYM_CARETRETURN)
                {
                    for (; index >= 0 && (script[index] == SYM_LINEFEED || script[index] == SYM_CARETRETURN); --index) ;
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
            while (script[index--] == SYM_MEMBER_ACCESS);
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
        /// Skips forward until finding a beginning of comment section.
        /// </summary>
        public static void SeekToComment(ParserState state)
        {
            while (!state.AtLastChar &&
                !((state.Char == '/') && (state.PeekChar() == '/')) &&
                !((state.Char == '/') && (state.PeekChar() == '*')))
                state.Forward();
            // If we are at the last char, that means that no comment section was found,
            // so forward once further to reach the end of the script.
            if (state.AtLastChar)
                state.Forward();
        }

        /// <summary>
        /// Skip comments in the script that might have speech marks
        /// in them, which could confuse the parser.
        /// </summary>
        public static void SkipComments(ParserState state)
        {
            if (!state.AtLastChar &&
                (state.Char == '/') && (state.PeekChar() == '/'))
            {
                int startIndex = state.CharIndex;
                for (; !state.AtEnd &&
                    (state.Char != SYM_LINEFEED) &&
                    (state.Char != SYM_CARETRETURN);
                    state.Forward()) ;
                int endIndex = state.CharIndex;
                var section = new Tuple<int, int>(startIndex, endIndex);
                if (!state.Comments.Contains(section))
                    state.Comments.Add(section);
            }
            if (!state.AtLastChar &&
                (state.Char == '/') && (state.PeekChar() == '*'))
            {
                int startIndex = state.CharIndex;
                state.Forward2();
                for (; !state.AtLastChar; state.Forward())
                {
                    if ((state.Char == '*') && (state.PeekChar() == '/'))
                    {
                        state.Forward2();
                        break;
                    }
                }
                int endIndex = state.CharIndex;
                var section = new Tuple<int, int>(startIndex, endIndex);
                if (!state.Comments.Contains(section))
                    state.Comments.Add(section);
            }
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
