using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public class DockingPane : IDockingPane
    {
        FloatingWindow _floatWindow;
        DockPane _dockPane;

        public DockingPane(DockPane dockPane)
        {
            _dockPane = dockPane;
            _floatWindow = new FloatingWindow(dockPane.FloatWindow);
        }

        public IFloatWindow FloatWindow { get { return _floatWindow; } }
    }
}
