using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class Macro
	{
		private string _name;
		private string _value;

		public Macro(string name, string value)
		{
			_name = name;
			_value = value;
		}

		public string Name
		{
			get { return _name; }
		}

		public string Value
		{
			get { return _value; }
		}
	}
}
