using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public interface IDockingPanel
    {
        event EventHandler ActiveContentChanged;

        bool IsDisposed { get; }
        IDockingContent ActiveContent { get; }
    }
}
