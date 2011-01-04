using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	public static class CompilerFactory
	{
		/// <summary>
		/// Creates a new Preprocessor which you can use to preprocess scripts.
		/// </summary>
		/// <param name="applicationVersion">The version of the application, used to preprocess #ifver/#ifnver directives.</param>
		public static IPreprocessor CreatePreprocessor(string applicationVersion)
		{
			return new Preprocessor(applicationVersion);
		}

		/// <summary>
		/// Creates a new script compiler.
		/// </summary>
		public static IScriptCompiler CreateScriptCompiler()
		{
			return new ScriptCompiler();
		}

		/// <summary>
		/// Creates a new Tokenizer which you can use to tokenize pre-processed scripts.
		/// </summary>
		internal static ITokenizer CreateTokenizer()
		{
			return new Tokenizer();
		}
	}
}
