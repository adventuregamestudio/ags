using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class ScriptReader
    {
        private TokenizedScript _source = null;
        private List<Token> _tokenStream;
        private Stack<Pair<int, int>> _savedLocations = new Stack<Pair<int, int>>();
        private int _readIndex = 0;
        private int _currentLineNumber = 0;

        public ScriptReader(TokenizedScript source)
        {
            _source = source;
            _tokenStream = source.TokenStream;
        }

        public ScriptReader(Expression expr, int initialLineNumber)
        {
            _tokenStream = expr;
            _currentLineNumber = initialLineNumber;
        }

        public int LineNumber
        {
            get { return _currentLineNumber; }
        }

        public Token ReadNextToken()
        {
            return PeekNextToken(out _currentLineNumber, out _readIndex);
        }

        public void PushLocation()
        {
            _savedLocations.Push(new Pair<int, int>(_currentLineNumber, _readIndex));
        }

        public void DeletePushedLocation()
        {
            _savedLocations.Pop();
        }

        public void PopLocation()
        {
            Pair<int, int> popped = _savedLocations.Pop();
            _currentLineNumber = popped.First;
            _readIndex = popped.Second;
        }

        public Token PeekNextToken()
        {
            int lineNumTemp, readIndexTemp;
            Token toReturn = PeekNextToken(out lineNumTemp, out readIndexTemp);
            return toReturn;
        }

        /// <summary>
        /// Determines if the next token is the specified keyword token. If it
        /// is, reads it from the stream. If not, leaves the stream as-is.
        /// </summary>
        public bool NextIsKeyword(PredefinedSymbol symbolType)
        {
            return NextIsKeyword(symbolType, false);
        }

        /// <summary>
        /// Determines if the next token is the specified keyword token. Behaviour
        /// depends on parameters.
        /// </summary>
        /// <param name="peekOnly">do not advance the stream if the symbol is found</param>
        /// <returns></returns>
        public bool NextIsKeyword(PredefinedSymbol symbolType, bool peekOnly)
        {
            return NextIsKeyword(new PredefinedSymbol[1] { symbolType }, false, peekOnly, null);
        }

        private bool NextIsKeyword(PredefinedSymbol[] symbolTypes, bool throwIfNot, bool peekOnly, string customErrorMessage)
        {
            int lineNumberOfNext, readIndexOfNext;

            Token nextToken = PeekNextToken(out lineNumberOfNext, out readIndexOfNext);
            if (nextToken is KeywordToken)
            {
                foreach (PredefinedSymbol symbolType in symbolTypes)
                {
                    if (((KeywordToken)nextToken).SymbolType == symbolType)
                    {
                        if (!peekOnly)
                        {
                            _currentLineNumber = lineNumberOfNext;
                            _readIndex = readIndexOfNext;
                        }
                        return true;
                    }
                }
            }

            if (throwIfNot)
            {
                string errorMessage;
                if (customErrorMessage != null)
                {
                    errorMessage = customErrorMessage;
                }
                else
                {
                    errorMessage = "Unexpected '" + nextToken.Name + "'; was expecting one of ";
                    foreach (PredefinedSymbol symbolType in symbolTypes)
                    {
                        errorMessage += "'" + symbolType.ToString() + "'; ";
                    }
                }
                throw new CompilerMessage(ErrorCode.UnexpectedToken, errorMessage);
            }
            return false;
        }

        /// <summary>
        /// Reads the next token, and expects it to be one of the passed in symbolTypes.
        /// If it isn't, throws an exception.
        /// </summary>
        public void ExpectKeyword(params PredefinedSymbol[] symbolTypes)
        {
            NextIsKeyword(symbolTypes, true, false, null);
        }

        /// <summary>
        /// Reads the next token, and expects it to be the passed in type.
        /// If it isn't, throws an exception with the specified message.
        /// </summary>
        public void ExpectKeyword(PredefinedSymbol symbolTypes, string errorMessage)
        {
            NextIsKeyword(new PredefinedSymbol[] { symbolTypes }, true, false, errorMessage);
        }

        /// <summary>
        /// Reads the next token from the stream. If it's not a variable type,
        /// throws an exception.
        /// </summary>
        public Token ReadNextAsVariableType()
        {
            Token nextToken = ReadNextToken();
            if (!nextToken.IsVariableType)
            {
                throw new CompilerMessage(ErrorCode.VariableTypeExpected, "Variable type expected at '" + nextToken.Name + "'");
            }
            return nextToken;
        }

        /// <summary>
        /// Reads the next token from the stream. If it's not a global variable,
        /// throws an exception.
        /// </summary>
        public Token ReadNextAsGlobalVariable()
        {
            Token nextToken = ReadNextToken();
            if (nextToken.Type != TokenType.GlobalVariable)
            {
                throw new CompilerMessage(ErrorCode.GlobalVariableExpected, "Global variable expected at '" + nextToken.Name + "'; it must be defined first");
            }
            return nextToken;
        }

        /// <summary>
        /// Reads the next token from the stream. If it's not a keyword,
        /// throws an exception.
        /// </summary>
        public KeywordToken ReadNextAsKeyword()
        {
            Token nextToken = ReadNextToken();
            if (!(nextToken is KeywordToken))
            {
                throw new CompilerMessage(ErrorCode.UnexpectedToken, "Keyword expected at '" + nextToken.Name + "'");
            }
            return (KeywordToken)nextToken;
        }

        /// <summary>
        /// Reads the next token from the stream and returns a Const Int that
        /// it represents. If it doens't, throws an exception.
        /// </summary>
        public int ReadNextAsConstInt()
        {
            Token nextToken = PeekNextToken();
            if (nextToken.Type == TokenType.Constant)
            {
                ReadNextToken();
                return (int)nextToken.Value;
            }

            LiteralToken next = ReadNextAsLiteral();
            if (!next.IsInteger)
            {
                throw new CompilerMessage(ErrorCode.ConstIntExpected, "A constant integer was expected at '" + next.Name + "'");
            }

            return Convert.ToInt32(next.Name);
        }

        /// <summary>
        /// Reads the next token from the stream. If it's not a literal,
        /// throws an exception.
        /// </summary>
        private LiteralToken ReadNextAsLiteral()
        {
            Token nextToken = ReadNextToken();
            bool isNegative = false;

            if ((nextToken is OperatorToken) &&
                (nextToken.Name == "-"))
            {
                isNegative = true;
                nextToken = ReadNextToken();
            }

            if (!(nextToken is LiteralToken))
            {
                throw new CompilerMessage(ErrorCode.UnexpectedToken, "Literal expected at '" + nextToken.Name + "'");
            }

            Token tokenToReturn = nextToken;

            if (isNegative)
            {
                if (_source == null)
                {
                    throw new CompilerMessage(ErrorCode.InternalError, "Internal error: attempted to use tokenized script with no source");
                }
                tokenToReturn = _source.FindTokenWithName("-" + nextToken.Name);
                if (tokenToReturn == null)
                {
                    tokenToReturn = new LiteralToken("-" + nextToken.Name);
                    _source.AddToken(tokenToReturn);
                }
            }

            return (LiteralToken)tokenToReturn;
        }

        private Token PeekNextToken(out int lineNumber, out int readIndex)
        {
            readIndex = _readIndex;
            lineNumber = _currentLineNumber;

            if (_readIndex >= _tokenStream.Count)
            {
                return new EndOfStreamToken();
            }

            Token tokenToReturn;
            do
            {
                tokenToReturn = _tokenStream[readIndex];
                if (tokenToReturn is LineNumberToken)
                {
                    lineNumber = ((LineNumberToken)tokenToReturn).LineNumber;
                }
                else if (tokenToReturn is EndOfStreamToken)
                {
                    // don't increment the counter -- just leave it returning
                    // the end of stream
                    break;
                }
                readIndex++;
            }
            while (tokenToReturn is LineNumberToken);

            return tokenToReturn;
        }

        public Token ReadNextTokenAndThrowIfAlreadyDefined()
        {
            Token nextToken = ReadNextToken();
            if (nextToken.Defined)
            {
                throw new CompilerMessage(ErrorCode.TokenAlreadyDefined, "Token '" + nextToken.Name + "' is already defined");
            }
            return nextToken;
        }

        /// <summary>
        /// If the next token is an asterisk, skips over it. If not, nothing happens.
        /// </summary>
        public void IgnoreAsteriskIfPresent()
        {
            if (this.PeekNextToken().Name == "*")
            {
                this.ReadNextToken();
            }
        }
    }
}
