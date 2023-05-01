using System;
using System.Collections.Generic;
using WeifenLuo.WinFormsUI.Docking;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// Describes an event of gathering the list of Dockable controls
    /// for the purpose of adding them to the Window menu.
    /// </summary>
    public class DockingWindowMenuEventArgs
    {
        public DockingWindowMenuEventArgs(List<DockContent> panesList, List<ContentDocument> docsList)
        {
            DockingPanes = panesList;
            DockingDocuments = docsList;
        }

        public readonly List<DockContent> DockingPanes;
        public readonly List<ContentDocument> DockingDocuments;
    }
}
