using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;

namespace AGS.Types
{
    public class DockData
    {
        public DockData(DockingState dockState, Rectangle location)
        {
            DockState = dockState;
            Location = location;
        }

        public DockingState DockState { get; private set; }
        public Rectangle Location { get; private set; }
    }
}
