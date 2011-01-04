using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal interface ITokenizer
	{
		TokenizedScript TokenizeScript(string script);
	}
}
