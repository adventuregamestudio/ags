using AGS.Types;
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class ProjectTreeItem : IProjectTreeItem
    {
		public delegate bool CanDropHereDelegate(ProjectTreeItem source, ProjectTreeItem target);
		public delegate void DropHereDelegate(ProjectTreeItem source, ProjectTreeItem target);

        public bool AllowLabelEdit = false;
        public PropertyInfo LabelTextProperty;
        public PropertyInfo LabelTextDescriptionProperty;
        public object LabelTextDataSource;
        public string ID = "";
        public string LabelTextBeforeLabelEdit;
        public TreeNode TreeNode;
		public bool AllowDragging = false;
		public CanDropHereDelegate CanDropHere = null;
		public DropHereDelegate DropHere = null;

        public ProjectTreeItem(string id, TreeNode node)
        {
            ID = id;
            TreeNode = node;
        }
    }
}
