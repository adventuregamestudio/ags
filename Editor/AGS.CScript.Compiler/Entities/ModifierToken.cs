using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class ModifierToken : Token
    {
        private ModifierTargets _targets;
        private PredefinedSymbol _type;

        internal ModifierToken(string name, PredefinedSymbol type, ModifierTargets targets)
            : base(name, true)
        {
            _targets = targets;
            _type = type;
        }

        public PredefinedSymbol Symbol
        {
            get { return _type; }
        }

        public ModifierTargets Targets
        {
            get { return _targets; }
        }
    }
}
