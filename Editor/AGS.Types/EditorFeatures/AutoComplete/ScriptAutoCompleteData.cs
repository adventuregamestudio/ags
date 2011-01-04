using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types.AutoComplete;

namespace AGS.Types
{
    public class ScriptAutoCompleteData
    {
		private bool _populated = false;
        private List<ScriptVariable> _variables = new List<ScriptVariable>();
        private List<ScriptFunction> _functions = new List<ScriptFunction>();
        private List<ScriptDefine> _defines = new List<ScriptDefine>();
        private List<ScriptEnum> _enums = new List<ScriptEnum>();
        private List<ScriptStruct> _structs = new List<ScriptStruct>();

        public void CopyFrom(ScriptAutoCompleteData other)
        {
            this._variables = other.Variables;
            this._functions = other.Functions;
            this._defines = other.Defines;
            this._enums = other.Enums;
            this._structs = other.Structs;
			this._populated = other.Populated;
        }

        public ScriptFunction FindFunction(string name)
        {
            foreach (ScriptFunction func in _functions)
            {
                if (func.FunctionName == name)
                {
                    return func;
                }
            }
            return null;
        }

		public ScriptVariable FindVariable(string name)
		{
			foreach (ScriptVariable variable in _variables)
			{
				if (variable.VariableName == name)
				{
					return variable;
				}
			}
			return null;
		}

		public ScriptStruct FindStruct(string name)
		{
			foreach (ScriptStruct struc in _structs)
			{
				if (struc.Name == name)
				{
					return struc;
				}
			}
			return null;
		}

		public List<ScriptVariable> Variables
        {
            get { return _variables; }
        }

        public List<ScriptFunction> Functions
        {
            get { return _functions; }
        }

        public List<ScriptDefine> Defines
        {
            get { return _defines; }
        }

        public List<ScriptEnum> Enums
        {
            get { return _enums; }
        }

        public List<ScriptStruct> Structs
        {
            get { return _structs; }
        }

		/// <summary>
		/// Whether this autocomplete data has been populated
		/// </summary>
		public bool Populated
		{
			get { return _populated; }
			set { _populated = value; }
		}
    }
}
