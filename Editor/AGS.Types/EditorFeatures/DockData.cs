using System;
using System.Collections.Generic;
using System.Text;
using WeifenLuo.WinFormsUI.Docking;
using System.Drawing;

namespace AGS.Types
{
    public class DockData
    {
        public DockData(DockState dockState, Rectangle location)
        {
            DockState = dockState;
            Location = location;
        }

        public DockState DockState { get; private set; }
        public Rectangle Location { get; private set; }        
    }
}
