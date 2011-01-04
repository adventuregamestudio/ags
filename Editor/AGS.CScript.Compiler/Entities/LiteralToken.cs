using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class LiteralToken : Token
	{
		public LiteralToken(string name)
			: base(name, true)
		{
		}

		public bool IsInteger
		{
			get
			{
				int result;
				return int.TryParse(_name, out result);
			}
		}

		public override string ToString()
		{
			return "LITERAL: " + _name;
		}
	}
}
