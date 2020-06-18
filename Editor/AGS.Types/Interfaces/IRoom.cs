using System;

namespace AGS.Types
{
	public interface IRoom : IComparable<IRoom>, IToXml
	{
		int Number { get; }
		string Description { get; set; }
		string FileName { get; }
		bool StateSaving { get; }
		string ScriptFileName { get; }
		Script Script { get; }
        string UserFileName { get; }
        void LoadScript();
        void UnloadScript();
	}
}
