using System;
using System.Collections.Generic;
using System.Text;

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
        string Directory { get; }
        string DataFileName { get; }
        string UserFileName { get; }
        void LoadScript();
        void UnloadScript();
        string GetBackgroundFileName(int background);
        string GetMaskFileName(RoomAreaMaskType mask);

    }
}
