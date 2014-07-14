using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class ViewChooser : Form
    {
        private AGS.Types.View _selectedView;
        private int _startingView;

        public ViewChooser(int currentView)
        {
            InitializeComponent();
            _startingView = currentView;
        }

        public AGS.Types.View SelectedView
        {
            get { return _selectedView; }
        }

        public static AGS.Types.View ShowViewChooser(IWin32Window owner, int currentView)
        {
            AGS.Types.View chosenView = null;
            ViewChooser chooser = new ViewChooser(currentView);
            if (chooser.ShowDialog(owner) == DialogResult.OK)
            {
                chosenView = chooser.SelectedView;
            }
            chooser.Dispose();
            return chosenView;
        }

        private void AddFolderToTree(ViewFolder folder, TreeNodeCollection parentNodeList)
        {
            foreach (ViewFolder subFolder in folder.SubFolders)
            {
                TreeNode newNode = parentNodeList.Add(null, subFolder.Name, "GenericFolderIcon", "GenericFolderIcon");
                newNode.Tag = subFolder;
                AddFolderToTree(subFolder, newNode.Nodes);
            }

            foreach (AGS.Types.View view in folder.Views)
            {
                TreeNode newNode = parentNodeList.Add(null, view.NameAndID, "ViewsIcon", "ViewsIcon");
                newNode.Tag = view;
                if (view.ID == _startingView)
                {
                    newNode.TreeView.SelectedNode = newNode;
                }
            }
        }

        private void ViewChooser_Load(object sender, EventArgs e)
        {
            viewTree.ImageList = Factory.GUIController.ImageList;
            viewTree.Nodes.Add(null, "Views", "GenericFolderIcon", "GenericFolderIcon");
            AddFolderToTree(Factory.AGSEditor.CurrentGame.RootViewFolder, viewTree.Nodes[0].Nodes);
            viewTree.Nodes[0].Expand();

            if (_startingView < 0)
            {
                viewTree.SelectedNode = viewTree.Nodes[0];
            }
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            _selectedView = (AGS.Types.View)viewTree.SelectedNode.Tag;
            this.DialogResult = DialogResult.OK;
            this.Close();
        }

        private void viewTree_AfterSelect(object sender, TreeViewEventArgs e)
        {
            if (e.Node.Tag is AGS.Types.View)
            {
                btnOK.Enabled = true;
                viewPreview.ViewToPreview = (AGS.Types.View)e.Node.Tag;
            }
            else
            {
                btnOK.Enabled = false;
                viewPreview.ViewToPreview = null;
            }
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        private void viewTree_NodeMouseDoubleClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            if (btnOK.Enabled)
            {
                btnOK_Click(sender, null);
            }
        }
    }
}