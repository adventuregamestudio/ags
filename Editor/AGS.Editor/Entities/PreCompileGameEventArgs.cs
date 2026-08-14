using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
	public class PreCompileGameEventArgs
	{
		private bool _forceRebuild;
		private DateTime? _forceRebuildTime;

		public PreCompileGameEventArgs(bool forceRebuild, DateTime? dt)
		{
			_forceRebuild = forceRebuild;
			_forceRebuildTime = dt;
			AllowCompilation = true;
		}

		public bool ForceRebuild
		{
			get { return _forceRebuild; }
		}

		public DateTime? ForceRebuildTime
		{
			get { return _forceRebuildTime; }
		}

		public bool AllowCompilation;
		public CompileMessages Errors;
	}
}
