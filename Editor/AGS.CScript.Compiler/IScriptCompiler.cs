using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    public interface IScriptCompiler
    {
        CompileResults CompileScript(string script);
    }
}
