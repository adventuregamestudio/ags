using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class LineNumberToken : Token
    {
        private int _lineNumber;

        public LineNumberToken(int lineNumber) : base(null, true)
        {
            _lineNumber = lineNumber;
        }

        public int LineNumber
        {
            get { return _lineNumber; }
        }

        public override string ToString()
        {
            return "LINE NUMBER TOKEN: " + _lineNumber;
        }
    }
}
