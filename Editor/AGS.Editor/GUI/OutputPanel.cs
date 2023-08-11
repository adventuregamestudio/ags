using AGS.Types;
using System;
using System.Collections;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public partial class OutputPanel : DockContent
    {
        private const string MENU_ITEM_COPYSEL_TO_CLIPBOARD = "CopySelectedToClipboard";
        private const string MENU_ITEM_COPYALL_TO_CLIPBOARD = "CopyAllToClipboard";
        private CompileMessages _errors;

        public OutputPanel()
        {
            InitializeComponent();
        }

		public void SetImageList(ImageList list)
		{
			lvwResults.SmallImageList = list;
		}

        public void SetMessages(string[] messages, string imageKey)
        {
            foreach (string message in messages)
            {
                SetMessage(message, imageKey);
            }
        }

        public void SetMessage(string message, string imageKey)
        {
            ListViewItem newItem = lvwResults.Items.Add(message);
            newItem.ImageKey = imageKey;
        }

        public CompileMessages ErrorsToList
        {
            get { return _errors; }
            set { _errors = value; RefreshList();  }
        }

        private void RefreshList()
        {
            lvwResults.Items.Clear();
            if (_errors != null)
            {
                foreach (CompileMessage error in _errors)
                {
                    ListViewItem newItem = lvwResults.Items.Add(error.Message);
					if (error is CompileError)
					{
						newItem.ImageKey = "CompileErrorIcon";
					}
					else
					{
						newItem.ImageKey = "CompileWarningIcon";
                    }

                    if (error.ScriptName.Length > 0)
                    {
                        newItem.SubItems.Add(error.ScriptName);
                    }

                    if (error.ScriptName.Length > 0 && error.LineNumber > 0)
                    {
                        newItem.SubItems.Add(error.LineNumber.ToString());
                    }
                }
            }
        }

        private void btnClose_Click(object sender, EventArgs e)
        {
            this.Visible = false;
        }

        private void lvwResults_ItemActivate(object sender, EventArgs e)
        {
			if (lvwResults.SelectedItems.Count > 0)
			{
				ListViewItem selectedItem = lvwResults.SelectedItems[0];
				if (selectedItem.SubItems.Count > 1)
				{
                    int line = 0;
                    if (selectedItem.SubItems.Count > 2 && selectedItem.SubItems[2].Text != null &&
                        int.TryParse(selectedItem.SubItems[2].Text, out line))
                    {
                        Factory.GUIController.ZoomToFile(selectedItem.SubItems[1].Text, Convert.ToInt32(selectedItem.SubItems[2].Text));
                    }
				}
			}
        }

        private string ListItemToString(ListViewItem item)
        {
            string thisLine = string.Empty;
            if (item.SubItems.Count > 1)
            {
                thisLine += item.SubItems[1].Text;  // filename

                if ((item.SubItems.Count > 2) &&
                    (item.SubItems[2].Text.Length > 0))
                {
                    thisLine += "(" + item.SubItems[2].Text + ")";  // line number
                }
            }
            if (thisLine.Length > 0)
            {
                thisLine += ": ";
            }
            thisLine += item.SubItems[0].Text;
            return thisLine;
        }

        private string ListItemsToString(IEnumerable list)
        {
            StringBuilder sb = new StringBuilder();
            foreach(ListViewItem item in list)
            {
                sb.Append(ListItemToString(item));
                sb.Append(Environment.NewLine);
            }
            return sb.ToString();
        }

		private void ContextMenuEventHandler(object sender, EventArgs e)
		{
            String result = string.Empty;
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Name == MENU_ITEM_COPYSEL_TO_CLIPBOARD)
            {
                result = ListItemsToString(lvwResults.SelectedItems);
            }
            else if (item.Name == MENU_ITEM_COPYALL_TO_CLIPBOARD)
            {
                result = ListItemsToString(lvwResults.Items);
            }

            if (!string.IsNullOrEmpty(result))
            {
                Utilities.CopyTextToClipboard(result);
            }
		}

		private void ShowContextMenu(Point menuPosition)
		{
			ContextMenuStrip menu = new ContextMenuStrip();
            menu.Items.Add(new ToolStripMenuItem("Copy selection to clipboard", null, ContextMenuEventHandler, MENU_ITEM_COPYSEL_TO_CLIPBOARD));
            menu.Items.Add(new ToolStripMenuItem("Copy all to clipboard", null, ContextMenuEventHandler, MENU_ITEM_COPYALL_TO_CLIPBOARD));

			menu.Show(lvwResults, menuPosition);
		}

		private void lvwResults_MouseUp(object sender, MouseEventArgs e)
		{
			if (e.Button == MouseButtons.Right)
			{
				ShowContextMenu(e.Location);
			}
		}

        private void LoadColorTheme(ColorTheme t)
        {
            t.SetColor("global/pane/background", c => BackColor = c);
            t.ListViewHelper(lvwResults, "output-panel");
        }

        private void OutputPanel_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
