using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.CScript.Compiler
{
    internal class Pair<T, U>
    {
        T _t;
        U _u;

        public Pair(T t, U u)
        {
            _t = t;
            _u = u;
        }

        public T First
        {
            get { return _t; }
        }

        public U Second
        {
            get { return _u; }
        }
    }
}
