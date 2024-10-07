using System;
using System.Collections;

namespace AGS.Types
{
    /// <summary>
    /// FakeEnumerator is a IEnumerator implementation that represents a always empty collection.
    /// </summary>
    internal class FakeEnumerator : IEnumerator
    {
        public object Current
        {
            get { return null; }
        }

        public bool MoveNext()
        {
            return false;
        }

        public void Reset()
        {
        }
    }
}
