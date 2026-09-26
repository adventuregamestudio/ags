using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
	public class PreCompileGameEventArgs
	{
		private bool _forceRebuild = false;
		private DateTime? _forceRebuildTime = null;
		private bool _cancelIfUpgradeNeeded = false;
		private bool _upgradeRequired = false;

		public PreCompileGameEventArgs(bool forceRebuild, DateTime? dt, bool cancelIfUpgrade = false)
		{
			_forceRebuild = forceRebuild;
			_forceRebuildTime = dt;
			_cancelIfUpgradeNeeded = cancelIfUpgrade;
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

		public bool CancelIfUpgradeNeeded
		{
			get { return _cancelIfUpgradeNeeded; }
		}

		public bool UpgradeRequired
		{
			get { return _upgradeRequired; }
			set { _upgradeRequired = value; }
		}

		public bool AllowCompilation { get; set; }
		public CompileMessages Errors { get; set; }
	}
}
