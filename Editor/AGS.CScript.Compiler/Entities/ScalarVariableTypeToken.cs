using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal class ScalarVariableTypeToken : Token
	{
		private int _memorySizeInBytes;
        private bool _isLegacyString = false;

		public ScalarVariableTypeToken(string name, int memorySizeInBytes)
			: base(name, true, true)
		{
			_memorySizeInBytes = memorySizeInBytes;
		}

        public ScalarVariableTypeToken(string name, int memorySizeInBytes, bool isLegacyString)
            : this(name, memorySizeInBytes)
        {
            _isLegacyString = isLegacyString;
        }

		public int SizeInBytes
		{
			get { return _memorySizeInBytes; }
		}
	}
}
