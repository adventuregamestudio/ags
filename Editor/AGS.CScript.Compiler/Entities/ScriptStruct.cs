using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class ScriptStruct : DataGroup
	{
		public string Name;
		public bool IsManaged = false;
		public ScriptStruct Extends = null;
        public Modifiers Modifiers = null;
        public bool PrototypeOnly = false;

		public ScriptStruct(string name)
		{
			Name = name;
		}

	}
}
