using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    // TODO: merge this with AGS.Native.IScriptCompiler!
    // I wrote a new variant of IScriptCompiler inside AGS.Native lib, because
    // this one has certain ties within CScript.Compiler classes.
    // The only implementation of IScriptCompiler in here is not completed and
    // was never used in AGS Editor (apparently, not sure if possible to test).
    //
    // Proposal: rework CompileResults, make it contain CompiledScript and
    // CompileMessages collection. Merge or substitute CompilerMessage with
    // AGS.Types.CompileMessage. But note that CompilerMessage also has
    // Error Codes! Need to figure out how to make that shareable without strict
    // bind to the Compiler lib.
    // Re CompiledScript, see if possible to merge with AGS.Native.CompiledScript,
    // which has data stored as native ccScript (?). Need to think this through.
    //
    // Important: IScriptCompiler implementation does not necessarily has actual
    // compiler code inside. It may instead be a wrapper over a code that starts
    // **EXTERNAL** compiler, i.e. runs a standalone executable, passing all
    // parameters through command line.
    // Need to think through how to receive results in that case. Maybe CompileResults
    // could contain optionally an object that has compiled data in memory **AND**
    // optionally a path to compiled file. And interface user would test which one
    // is valid. That's just an idea for now.

    public interface IScriptCompiler
	{
		CompileResults CompileScript(string script);
	}
}
