using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	public interface IPreprocessor
	{
		/// <summary>
		/// Predefines a macro so that the scripts can pick it up.
		/// </summary>
		void DefineMacro(string name, string value);

		/// <summary>
		/// Preprocesses the script and returns the results. State is
		/// preserved between calls, which allows you to preprocess header
		/// files and then the main script in multiple calls.
		/// </summary>
		string Preprocess(string script, string scriptName);

		/// <summary>
		/// Gets the results of the preprocess operations so far.
		/// </summary>
		CompileResults Results { get; }
	}
}
