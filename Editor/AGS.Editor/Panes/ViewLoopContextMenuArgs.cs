using System;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public class ViewLoopContextMenuArgs : EventArgs
    {
        public ViewLoopContextMenuArgs(ContextMenuStrip menu, ViewLoop loop, ViewFrame frame)
        {
            Menu = menu;
            Loop = loop;
            Frame = frame;
        }

        public ContextMenuStrip Menu { get; private set; }
        public ViewLoop Loop { get; private set; }
        public ViewFrame Frame { get; private set; }
        public bool ItemsOverriden { get; set; }
    }
}
