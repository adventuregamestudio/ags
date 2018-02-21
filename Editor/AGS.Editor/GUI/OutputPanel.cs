using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public partial class OutputPanel : DockContent
    {
		private const string MENU_ITEM_COPY_TO_CLIPBOARD = "CopyToClipboard";
        private CompileMessages _errors;

        public OutputPanel()
        {
            InitializeComponent();            
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
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

        private void LoadColorTheme(ColorTheme t)
        {
            lvwResults.BackColor = t.GetColor("output-panel/background");
            lvwResults.ForeColor = t.GetColor("output-panel/foreground");
            lvwResults.OwnerDraw = t.GetBool("output-panel/owner-draw");
            lvwResults.GridLines = t.GetBool("output-panel/grid-lines");
            lvwResults.Layout += (s, a) =>
            {
                lvwResults.Columns[lvwResults.Columns.Count - 1].Width = t.GetInt("output-panel/last-column-width");
            };
            lvwResults.DrawItem += (s, a) => a.DrawDefault = t.GetBool("output-panel/draw-item");
            lvwResults.DrawSubItem += (s, a) => a.DrawDefault = t.GetBool("output-panel/draw-sub-item");
            lvwResults.DrawColumnHeader += (s, a) =>
            {
                a.Graphics.FillRectangle(new SolidBrush(t.GetColor("output-panel/column-header/background")), a.Bounds);
                a.Graphics.DrawString(a.Header.Text, lvwResults.Font, new SolidBrush(t.GetColor("output-panel/column-header/foreground")), a.Bounds.X + 5, a.Bounds.Y + a.Bounds.Size.Height / 5);
                a.Graphics.DrawRectangle(new Pen(new SolidBrush(t.GetColor("output-panel/column-header/border"))), a.Bounds.X - 1, a.Bounds.Y - 1, a.Bounds.Size.Width, a.Bounds.Size.Height);
            };
        }
    }
}
