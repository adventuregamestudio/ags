using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
	internal enum ModifierTargets
	{
		GlobalFunction = 1,
		GlobalVariable = 2,
		MemberFunction = 4,
		MemberVariable = 8,
		Struct = 0x10,
		FunctionParameter = 0x20,
		Function = 0x40,
        LocalVariable = 0x80
	}
}
