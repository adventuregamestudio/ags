using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	public class CompileWarning : CompileMessage
	{
		public CompileWarning(string message, string scriptName, int lineNumber)
			: base(message, scriptName, lineNumber)
		{
		}

		public CompileWarning(string message)
			: base(message)
		{
		}

		public CompileWarning(string message, Exception innerException)
			: base(message, innerException)
		{
		}

		// Copy constructor required by BusyDialog
		public CompileWarning(CompileWarning cloneFrom)
            : base(cloneFrom)
		{
		}
	}
}
