using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	public class GetScriptHeaderListEventArgs
	{
		IList<Script> _scripts;

		public GetScriptHeaderListEventArgs(IList<Script> scriptHeaders)
		{
			_scripts = scriptHeaders;
		}

		public IList<Script> ScriptHeaders
		{
			get { return _scripts; }
		}
	}
}
