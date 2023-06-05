using System;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class RoomFilterContextMenuArgs : EventArgs
    {
        public RoomFilterContextMenuArgs(ContextMenuStrip menu, int x, int y)
        {
            Menu = menu;
            X = x;
            Y = y;
        }

        public ContextMenuStrip Menu { get; private set; }
        public int X { get; private set; }
        public int Y { get; private set; }
    }
}
