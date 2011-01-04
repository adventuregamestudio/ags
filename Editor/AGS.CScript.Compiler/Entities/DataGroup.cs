using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    /// <summary>
    /// Represents something which can contain variables (ie. either
    /// the global data area, or a struct definition)
    /// </summary>
    internal class DataGroup
    {
        public List<ScriptVariable> Members = new List<ScriptVariable>();
        public int SizeInBytes = 0;

        public bool HasNonImportedMemberOfType(Token tokenToLookFor)
        {
            foreach (ScriptVariable member in Members)
            {
                if (member.VariableTypeToken == tokenToLookFor)
                {
                    bool thisOneIsOk = true;

                    if ((member is FixedOffsetVariable) && 
                        (((FixedOffsetVariable)member).IsAttributeProperty))
                    {
                        thisOneIsOk = false;
                    }

                    if (thisOneIsOk)
                    {
                        return true;
                    }
                }
            }
            return false;
        }
    }
}
