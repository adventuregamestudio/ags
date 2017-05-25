using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class ScalarVariableTypeToken : Token
	{
		private int _memorySizeInBytes;

		public ScalarVariableTypeToken(string name, int memorySizeInBytes)
			: base(name, true, true)
		{
			_memorySizeInBytes = memorySizeInBytes;
		}

		public int SizeInBytes
		{
			get { return _memorySizeInBytes; }
		}
	}
}
