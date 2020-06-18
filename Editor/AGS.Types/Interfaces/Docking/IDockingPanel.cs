using System;

namespace AGS.Types
{
    public interface IDockingPanel
    {
        event EventHandler ActiveContentChanged;

        bool IsDisposed { get; }
        IDockingPane ActivePane { get; }
        IDockingContent ActiveContent { get; }
    }
}
