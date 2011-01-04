using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	public class CompilerMessage : ApplicationException
	{
		private ErrorCode _errorCode;
		private string _message;
		private int _lineNumber;
		private string _scriptName;

		internal CompilerMessage(ErrorCode errorCode, string message, int lineNumber, string scriptName)
		{
			_message = message;
			_errorCode = errorCode;
			_lineNumber = lineNumber;
			_scriptName = scriptName ?? string.Empty;
		}

		internal CompilerMessage(ErrorCode errorCode, string message)
		{
			_message = message;
			_errorCode = errorCode;
		}

		public new string Message
		{
			get { return _message; }
		}

		public ErrorCode Code
		{
			get { return _errorCode; }
		}

		public int LineNumber
		{
			get { return _lineNumber; }
		}

		public string ScriptName
		{
			get { return _scriptName; }
		}
	}
}
