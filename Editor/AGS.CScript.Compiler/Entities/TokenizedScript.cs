using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class TokenizedScript
	{
		private Dictionary<string, Token> _tokens = new Dictionary<string,Token>();
		private List<Token> _tokenStream = new List<Token>();

		public TokenizedScript()
		{
			CreateDefaultTokens();
		}

		public List<Token> TokenStream
		{
			get { return _tokenStream; }
		}

		public Token FindTokenWithName(string name)
		{
			if (!_tokens.ContainsKey(name))
			{
				return null;
			}
			return _tokens[name];
		}

		public void AddToken(Token tokenToAdd)
		{
			_tokens.Add(tokenToAdd.Name, tokenToAdd);
		}

        public void WriteToken(string tokenName)
		{
			if (!_tokens.ContainsKey(tokenName))
			{
				if ((tokenName.Length > 0) &&
					(Char.IsLetter(tokenName[0]) || (tokenName[0] == '_')))
				{
					_tokens.Add(tokenName, new Token(tokenName));
				}
				else
				{
					// Doesn't start with A-Z or _ so it must be a literal of some sort
					_tokens.Add(tokenName, new LiteralToken(tokenName));
				}
			}
			_tokenStream.Add(_tokens[tokenName]);
		}

		public void WriteEndOfStream()
		{
			_tokenStream.Add(new EndOfStreamToken());
		}

		public void WriteNewLineNumber(int lineNumber)
		{
			if ((_tokenStream.Count > 0) && 
				(_tokenStream[_tokenStream.Count - 1] is LineNumberToken))
			{
				// If the last token was also a line number, remove it (we
				// don't need two in a row if the line was blank)
				_tokenStream.RemoveAt(_tokenStream.Count - 1);
			}
			_tokenStream.Add(new LineNumberToken(lineNumber));
		}

		private void CreateDefaultTokens()
		{
			foreach (Token token in DefaultTokenStore.GetTokenList())
			{
				_tokens.Add(token.Name, token);
			}
		}
	}
}
