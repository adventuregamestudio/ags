using System;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Controls
{
    public class ListViewContextMenuEventArgs : ContextMenuTriggerEventArgs
    {
        public ListViewContextMenuEventArgs(Point position, ListViewItem item)
            : base(position)
        {
            Item = item;
        }

        public ListViewItem Item { get; private set; }
    }
}
