using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
	public interface IRoom : IComparable<IRoom>
	{
		int Number { get; }
		string Description { get; set; }
		string FileName { get; }
		bool StateSaving { get; }
		string ScriptFileName { get; }
		Script Script { get; }
		void LoadScript();
	}
}
