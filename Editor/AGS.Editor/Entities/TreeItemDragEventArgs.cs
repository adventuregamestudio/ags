using System;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class TreeItemDragEventArgs : DragEventArgs
    {
        public TreeItemDragEventArgs(DragEventArgs dragArgs, TreeNode dragItem, TreeNode dropTarget,
                                     TargetDropZone dropZone, bool showLine, bool expandOnHover)
            : base(dragArgs.Data, dragArgs.KeyState, dragArgs.X, dragArgs.Y, dragArgs.AllowedEffect, dragArgs.Effect)
        {
            DragItem = dragItem;
            DropTarget = dropTarget;
            DropZone = dropZone;
            ShowLine = showLine;
            ExpandOnDragHover = expandOnHover;
        }

        public TreeNode DragItem { get; private set; }
        public TreeNode DropTarget { get; private set; }
        public TargetDropZone DropZone { get; private set; }
        public bool ShowLine { get; set; }
        public bool ExpandOnDragHover { get; set; }
    }
}
