using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class KeywordToken : Token
    {
        private PredefinedSymbol _symbolType;
        private bool _isModificationOperator;

        public KeywordToken(string name, PredefinedSymbol symbolType, bool isModificationOperator)
            : base(name, true)
        {
            _symbolType = symbolType;
            _isModificationOperator = isModificationOperator;
        }

        public KeywordToken(string name, PredefinedSymbol symbolType)
            : this(name, symbolType, false)
        { }

        public PredefinedSymbol SymbolType
        {
            get { return _symbolType; }
        }

        public bool IsModificationOperator
        {
            get { return _isModificationOperator; }
        }
    }
}
