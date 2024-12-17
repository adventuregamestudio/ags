using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using AGS.Types;
using Version = System.Version;

namespace AGS.CScript.Compiler
{
	internal class Preprocessor : IPreprocessor
	{
		private bool _inMultiLineComment = false;
		private PreprocessorState _state = new PreprocessorState();
		// Conditional statement stack remembers the results of all the nested conditions
		// that we have entered.
		private Stack<bool> _conditionalStatements = new Stack<bool>();
		// Negative counter: it is incremented each time we enter a FALSE condition,
		// and decremented each time we exit a previous FALSE condition.
		private uint _negativeCounter = 0;
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
			output.AppendLine(Constants.NEW_SCRIPT_MARKER + scriptName.Replace(@"\", @"\\") + "\"");
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

			Stack<StringBuilder> previousOutput = new Stack<StringBuilder>();
			Stack<FastString> previousLine = new Stack<FastString>();
			StringBuilder output = new StringBuilder(lineToProcess.Length);
			FastString line = new FastString(lineToProcess);
			Stack<String> ignored = new Stack<String>();
			while (line.Length > 0)
			{
				int i = 0;
				while ((i < line.Length) && (!Char.IsLetterOrDigit(line[i])))
				{
                    if ((line[i] == '"') || (line[i] == '\''))
                    {
                        int end_of_literal = FindIndexOfMatchingCharacter(line.ToString(), i, line[i]);
                        if (end_of_literal < 0)
                        {
                            i = line.Length;
                            break;
                        }
                        if (i == 0 && line[0] == '"')
                        {
                            // '[end_of_literal]' contains the '"', we need the part before that
                            FastString literal = line.Substring(0, end_of_literal);
                            if (literal.StartsWith(Constants.NEW_SCRIPT_MARKER))
                            {
                                // Start the new script
                                _scriptName = literal.Substring(Constants.NEW_SCRIPT_MARKER.Length).ToString();
                                _lineNumber = 0;
                            }
                        }
                        i = end_of_literal;
					}
					i++;
				}

				output.Append(line.Substring(0, i));

				if (i < line.Length)
				{
					bool precededByDot = false;
					if (i > 0) precededByDot = (line[i - 1] == '.');

					line = line.Substring(i);

					string realStringLine = line.ToString();
					string theWord = GetNextWord(ref realStringLine, false, false);
					line = realStringLine;


					if ((!precededByDot) && (!ignored.Contains(theWord)) && (_state.Macros.Contains(theWord)))
					{
						previousOutput.Push(output);
						previousLine.Push(line);
						ignored.Push(theWord);
						line = new FastString(_state.Macros[theWord]);
						output = new StringBuilder(line.Length);
					}
					else
					{
						output.Append(theWord);
					}
				}
				else
					line = "";

				while (line.Length == 0 && previousOutput.Count > 0)
				{
					String result = output.ToString();
					output = previousOutput.Pop();
					line = previousLine.Pop();
					ignored.Pop();
					output.Append(result);
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
				   (text[i].IsScriptWordChar() ||
				    (includeDots && (text[i] == '.')))
				   )
			{
				i++;
			}

			if (i < text.Length && text[i] > 127)
			{
				RecordError(ErrorCode.InvalidCharacter, "Invalid character detected in script at position " + i.ToString() + " in '" + text + "'");
				string res = text;
				text = string.Empty; // need to end line
				return res;
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

			if (_negativeCounter > 0)
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
			else if (directive == "ifver" || directive == "ifnver")
			{
				// Compare provided version number with the current application version
				try
				{
					Version appVersion = new Version(_applicationVersion);
					// .NET Version class requires at least first two version components,
					// but AGS has traditionally supported one component too.
					int major_test;
					if (macroName.IndexOf('.') < 0 && Int32.TryParse(macroName, out major_test))
						macroName = macroName + ".0";
					Version macroVersion = new Version(macroName);
					includeCodeBlock = appVersion.CompareTo(macroVersion) >= 0;
					if (directive == "ifnver")
					{
						includeCodeBlock = !includeCodeBlock;
					}
				}
				catch (Exception e)
				{
					RecordError(ErrorCode.InvalidVersionNumber, String.Format("Cannot parse version number: {0}", e.Message));
				}
			}

			_conditionalStatements.Push(includeCodeBlock);
			if (!includeCodeBlock)
				_negativeCounter++; // more negative conditions
		}

		private bool DeletingCurrentLine()
		{
			return _negativeCounter > 0;
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
			else if (directive == "else")
			{
				if (_conditionalStatements.Count > 0)
				{
					// Negate previous condition
					bool prevCondition = _conditionalStatements.Pop();
					_conditionalStatements.Push(!prevCondition);
					if (prevCondition)
						_negativeCounter++; // it was positive before, but is negative now
					else
						_negativeCounter--; // it was negative before, but no more
				}
				else
				{
					RecordError(ErrorCode.ElseWithoutIf, "#else has no matching #if");
				}
			}
			else if (directive == "endif")
			{
				if (_conditionalStatements.Count > 0)
				{
					if (!_conditionalStatements.Pop())
						_negativeCounter--; // less negative conditions
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
			else if ((directive == "region") || (directive == "endregion"))
			{
				// do nothing -- scintilla can fold it, so it can be used to organize the code
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
                            RecordError(
                                ErrorCode.UnterminatedString,
                                $"Unterminated string: {text[i]} is missing");
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
