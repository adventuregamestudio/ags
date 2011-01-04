using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	public class Warning : CompilerMessage
	{
		public Warning(ErrorCode errorCode, string message, int lineNumber, string scriptName)
			: base(errorCode, message, lineNumber, scriptName)
		{
		}
	}
}
