using System;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class TreeItemTryDragEventArgs : ItemDragEventArgs
    {
        public TreeItemTryDragEventArgs(ItemDragEventArgs baseArgs)
            : base(baseArgs.Button, baseArgs.Item)
        {
            AllowedEffect = DragDropEffects.None;
        }

        public DragDropEffects AllowedEffect { get; set; }
    }
}
