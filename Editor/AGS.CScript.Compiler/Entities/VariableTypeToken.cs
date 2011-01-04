using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class VariableTypeToken : Token
	{
		private int _memorySizeInBytes;

		public VariableTypeToken(string name, int memorySizeInBytes)
			: base(name)
		{
			_memorySizeInBytes = memorySizeInBytes;
		}
	}
}
