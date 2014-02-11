using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public interface IDockingPane
    {
        void Refresh();
        IFloatWindow FloatWindow { get; }
    }
}
