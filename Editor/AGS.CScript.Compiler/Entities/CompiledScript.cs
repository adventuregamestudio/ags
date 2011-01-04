using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class CompiledScript
	{
		private List<int> _bytecode = new List<int>();
        private List<ScriptStruct> _structs = new List<ScriptStruct>();
        private GlobalData _globalData = new GlobalData();

        public GlobalData GlobalData
        {
            get { return _globalData; }
        }

		public void WriteCode(int code)
		{
			_bytecode.Add(code);
		}

        public void AddStruct(ScriptStruct newStruct)
        {
            _structs.Add(newStruct);
        }
	}
}
