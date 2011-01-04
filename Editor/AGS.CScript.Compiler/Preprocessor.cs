using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class Preprocessor : IPreprocessor
	{
		private const int MAX_LINE_LENGTH = 500;

		private bool _inMultiLineComment = false;
		private PreprocessorState _state = new PreprocessorState();
		private Stack<bool> _conditionalStatements = new Stack<bool>();
		private CompileResults _results = new CompileResults();
		private int _lineNumber;
		private string _scriptName;
		private string _applicationVersion;

		internal Preprocessor(string applicationVersion)
		{
			_applicationVersion = applicationVersion;
		}

		public CompileResults Results
		{
			get { return _results; }
		}

		public void DefineMacro(string name, string value)
		{
			_state.Macros.Add(new Macro(name, value));
		}

		/// <summary>
		/// Preprocesses the script and returns the results. State is
		/// preserved between calls, which allows you to preprocess header
		/// files and then the main script in multiple calls.
		/// </summary>
		public string Preprocess(string script, string scriptName)
		{
			StringBuilder output = new StringBuilder(script.Length);
			output.AppendLine(Constants.NEW_SCRIPT_MARKER + scriptName + "\"");
			StringReader reader = new StringReader(script);
			string thisLine;
			_scriptName = scriptName;
			_lineNumber = 0;
			while ((thisLine = reader.ReadLine()) != null)
			{
				_lineNumber++;
				thisLine = RemoveComments(thisLine);
				if (thisLine.Length > 0)
				{
					if (thisLine[0] != '#')
					{
						thisLine = PreProcessLine(thisLine);
					}
					else
					{
						thisLine = PreProcessDirective(thisLine);
					}
				}
				if (thisLine.Length >= MAX_LINE_LENGTH)
				{
					// For compatibility with legacy CSPARSER, which assumes lines
					// will not be longer than 500 chars. Remove once new compiler
					// is implemented.
					RecordError(ErrorCode.LineTooLong, "Line too long (max line length = " + MAX_LINE_LENGTH + ")");
				}
				output.AppendLine(thisLine);
			}
			reader.Close();

			if (_conditionalStatements.Count > 0)
			{
				RecordError(ErrorCode.IfWithoutEndIf, "Missing #endif");
			}

			return output.ToString();
		}

		private void RecordError(ErrorCode errorCode, string message)
		{
			_results.Add(new Error(errorCode, message, _lineNumber, _scriptName));
		}

		private string PreProcessLine(string lineToProcess)
		{
			if (DeletingCurrentLine())
			{
				return string.Empty;
			}

			StringBuilder output = new StringBuilder(lineToProcess.Length);
			FastString line = new FastString(lineToProcess);
			while (line.Length > 0)
			{
				int i = 0;
				while ((i < line.Length) && (!Char.IsLetterOrDigit(line[i])))
				{
					if ((line[i] == '"') || (line[i] == '\''))
					{
						i = FindIndexOfMatchingCharacter(line.ToString(), i, line[i]);
						if (i < 0)
						{
							i = line.Length;
							break;
						}
					}
					i++;
				}

				output.Append(line.Substring(0, i));

				if (i >= line.Length)
				{
					break;
				}

				bool precededByDot = false;
				if (i > 0) precededByDot = (line[i - 1] == '.');

				line = line.Substring(i);

				string realStringLine = line.ToString();
				string theWord = GetNextWord(ref realStringLine, false, false);
				line = realStringLine;

				if ((!precededByDot) && (_state.Macros.Contains(theWord)))
				{
					output.Append(_state.Macros[theWord]);
				}
				else
				{
					output.Append(theWord);
				}

			}

			return output.ToString();
		}

		private string GetNextWord(ref string text)
		{
			return GetNextWord(ref text, true, false);
		}

		private string GetNextWord(ref string text, bool trimText, bool includeDots)
		{
			int i = 0;
			while ((i < text.Length) && 
				   ((Char.IsLetterOrDigit(text[i])) || (text[i] == '_') ||
				    (includeDots && (text[i] == '.')))
				   )
			{
				i++;
			}
			string word = text.Substring(0, i);
			text = text.Substring(i);
			if (trimText)
			{
				text = text.Trim();
			}
			return word;
		}

		private void ProcessConditionalDirective(string directive, string line, CompileResults results)
		{
			string macroName = GetNextWord(ref line, true, true);
			if (macroName.Length == 0)
			{
				RecordError(ErrorCode.MacroNameMissing, "Expected something after '" + directive + "'");
				return;
			}

			bool includeCodeBlock = true;

			if ((_conditionalStatements.Count > 0) &&
				(_conditionalStatements.Peek() == false))
			{
				includeCodeBlock = false;
			}
			else if (directive.EndsWith("def"))
			{
				includeCodeBlock = _state.Macros.Contains(macroName);
				if (directive == "ifndef")
				{
					includeCodeBlock = !includeCodeBlock;
				}
			}
			else
			{
				if (!Char.IsDigit(macroName[0]))
				{
					RecordError(ErrorCode.InvalidVersionNumber, "Expected version number");
				}
				else
				{
					string appVer = _applicationVersion;
					if (appVer.Length > macroName.Length)
					{
						// AppVer is "3.0.1.25", macroNAme might be "3.0" or "3.0.1"
						appVer = appVer.Substring(0, macroName.Length);
					}
					includeCodeBlock = (appVer.CompareTo(macroName) >= 0);
					if (directive == "ifnver")
					{
						includeCodeBlock = !includeCodeBlock;
					}
				}
			}

			_conditionalStatements.Push(includeCodeBlock);
		}

		private bool DeletingCurrentLine()
		{
			return ((_conditionalStatements.Count > 0) &&
					(_conditionalStatements.Peek() == false));
		}

		private string PreProcessDirective(string line)
		{
			line = line.Substring(1);
			string directive = GetNextWord(ref line);

			if ((directive == "ifdef") || (directive == "ifndef") ||
				(directive == "ifver") || (directive == "ifnver"))
			{
				ProcessConditionalDirective(directive, line, _results);
			}
			else if (directive == "endif")
			{
				if (_conditionalStatements.Count > 0)
				{
					_conditionalStatements.Pop();
				}
				else
				{
					RecordError(ErrorCode.EndIfWithoutIf, "#endif has no matching #if");
				}
			}
			else if (DeletingCurrentLine())
			{
				// allow the line to be deleted, we are inside a failed #ifdef
			}
			else if (directive == "define")
			{
				string macroName = GetNextWord(ref line);
				if (macroName.Length == 0)
				{
					RecordError(ErrorCode.MacroNameMissing, "Macro name expected");
					return string.Empty;
				}
                else if (Char.IsDigit(macroName[0]))
                {
                    RecordError(ErrorCode.MacroNameInvalid, "Macro name '" + macroName + "' cannot start with a digit");
                }
				else if (_state.Macros.Contains(macroName))
				{
					RecordError(ErrorCode.MacroAlreadyExists, "Macro '" + macroName + "' is already defined");
				}
				else
				{
					_state.Macros.Add(new Macro(macroName, line));
				}
			}
			else if (directive == "undef")
			{
				string macroName = GetNextWord(ref line);
				if (macroName.Length == 0)
				{
					RecordError(ErrorCode.MacroNameMissing, "Macro name expected");
				}
				else if (!_state.Macros.Contains(macroName))
				{
					RecordError(ErrorCode.MacroDoesNotExist, "Macro '" + macroName + "' is not defined");
				}
				else
				{
					_state.Macros.Remove(macroName);
				}
			}
			else if (directive == "error")
			{
				RecordError(ErrorCode.UserDefinedError, "User error: " + line);
			}
			else if ((directive == "sectionstart") || (directive == "sectionend"))
			{
				// do nothing -- 2.72 put these as markers in the script
			}
			else
			{
				RecordError(ErrorCode.UnknownPreprocessorDirective, "Unknown preprocessor directive '" + directive + "'");
			}

			return string.Empty;  // replace the directive with a blank line
		}

		private int FindIndexOfMatchingCharacter(string text, int indexOfFirstSpeechMark, char charToMatch)
		{
			int endOfString = -1;
			int checkFrom = indexOfFirstSpeechMark + 1;
			for (int i = checkFrom; i < text.Length; i++)
			{
				if (text[i] == '\\')
				{
					i++;  // ignore next char
				}
				else if (text[i] == charToMatch)
				{
					endOfString = i;
					break;
				}
			}

			return endOfString;
		}

		private string RemoveComments(string text)
		{
			if (_inMultiLineComment)
			{
				int commentEnd = text.IndexOf("*/");
				if (commentEnd < 0)
				{
					return string.Empty;
				}
				text = text.Substring(commentEnd + 2);
				_inMultiLineComment = false;
			}

			StringBuilder output = new StringBuilder(text.Length);
			for (int i = 0; i < text.Length; i++)
			{
				if (!_inMultiLineComment)
				{
					if ((text[i] == '"') || (text[i] == '\''))
					{
						int endOfString = FindIndexOfMatchingCharacter(text, i, text[i]);
						if (endOfString < 0)
						{
							break;
						}
						endOfString++;
						output.Append(text.Substring(i, endOfString - i));
						text = text.Substring(endOfString);
						i = -1;
					}
					else if ((i < text.Length - 1) && (text[i] == '/') && (text[i + 1] == '/'))
					{
						break;
					}
					else if ((i < text.Length - 1) && (text[i] == '/') && (text[i + 1] == '*'))
					{
						_inMultiLineComment = true;
						i++;
					}
					else
					{
						output.Append(text[i]);
					}
				}
				else if ((i < text.Length - 1) && (text[i] == '*') && (text[i + 1] == '/'))
				{
					_inMultiLineComment = false;
					i++;
				}
			}

			return output.ToString().Trim();
		}
	}
}
