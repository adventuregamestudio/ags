using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class PreprocessorState
    {
        private Macros _macros = new Macros();

        public Macros Macros
        {
            get { return _macros; }
        }
    }
}
