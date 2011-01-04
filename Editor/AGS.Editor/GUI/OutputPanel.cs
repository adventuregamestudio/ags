using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class OutputPanel : UserControl
    {
		private const string MENU_ITEM_COPY_TO_CLIPBOARD = "CopyToClipboard";
        private CompileMessages _errors;

        public OutputPanel()
        {
            InitializeComponent();
            btnClose.Image = Resources.ResourceManager.GetBitmap("CloseButtonSmall.ico");
        }

		public void SetImageList(ImageList list)
		{
			lvwResults.SmallImageList = list;
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
					Factory.GUIController.ZoomToFile(selectedItem.SubItems[1].Text, Convert.ToInt32(selectedItem.SubItems[2].Text));
				}
			}
        }

		private void lvwResults_Click(object sender, EventArgs e)
		{

		}

		private void ContextMenuEventHandler(object sender, EventArgs e)
		{
			String result = string.Empty;
			foreach (ListViewItem item in lvwResults.Items)
			{
				String thisLine = string.Empty;
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
				thisLine += Environment.NewLine;
				result += thisLine;
			}

            if (!string.IsNullOrEmpty(result))
            {
                Utilities.CopyTextToClipboard(result);
            }
		}

		private void ShowContextMenu(Point menuPosition)
		{
			EventHandler onClick = new EventHandler(ContextMenuEventHandler);
			ContextMenuStrip menu = new ContextMenuStrip();
			menu.Items.Add(new ToolStripMenuItem("Copy contents to clipboard", null, onClick, MENU_ITEM_COPY_TO_CLIPBOARD));

			menu.Show(lvwResults, menuPosition);
		}

		private void lvwResults_MouseUp(object sender, MouseEventArgs e)
		{
			if (e.Button == MouseButtons.Right)
			{
				ShowContextMenu(e.Location);
			}
		}
    }
}
