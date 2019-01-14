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
    public partial class CallStackPanel : DockContent
    {
        private DebugCallStack _callStack;

        public CallStackPanel()
        {
            InitializeComponent();            
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
        }

		public void SetImageList(ImageList list)
		{
			lvwResults.SmallImageList = list;
		}

        public DebugCallStack CallStack
        {
            get { return _callStack; }
            set { _callStack = value; RefreshList(); }
        }

        private void RefreshList()
        {
            lvwResults.Items.Clear();
            if (_callStack != null)
            {
                PopulateListBoxWithCallStack(_callStack.Lines);

                if (lvwResults.Items.Count > 0)
                {
                    lvwResults.Items[0].Selected = true;
                }
            }
        }

        private void PopulateListBoxWithCallStack(List<CallStackLine> callStackLines)
        {
            foreach (CallStackLine callStackEntry in callStackLines)
            {
                ListViewItem newItem = lvwResults.Items.Add(callStackEntry.ScriptName);
                newItem.SubItems.Add(callStackEntry.LineNumber.ToString());
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
					Factory.GUIController.ZoomToFile(selectedItem.SubItems[0].Text, Convert.ToInt32(selectedItem.SubItems[1].Text), true, _callStack.ErrorMessage);
				}
			}
        }

		private void lvwResults_Click(object sender, EventArgs e)
		{
		}

		private void ShowContextMenu(Point menuPosition)
		{
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
            lvwResults.BackColor = t.GetColor("call-stack-panel/background");
            lvwResults.ForeColor = t.GetColor("call-stack-panel/foreground");
            lvwResults.OwnerDraw = t.GetBool("call-stack-panel/owner-draw");
            lvwResults.GridLines = t.GetBool("call-stack-panel/grid-lines");
            lvwResults.Layout += (s, a) =>
            {
                lvwResults.Columns[lvwResults.Columns.Count - 1].Width = t.GetInt("call-stack-panel/last-column-width");
            };
            lvwResults.DrawItem += (s, a) => a.DrawDefault = t.GetBool("call-stack-panel/draw-item");
            lvwResults.DrawSubItem += (s, a) => a.DrawDefault = t.GetBool("call-stack-panel/draw-sub-item");
            lvwResults.DrawColumnHeader += (s, a) =>
            {
                a.Graphics.FillRectangle(new SolidBrush(t.GetColor("call-stack-panel/column-header/background")), a.Bounds);
                a.Graphics.DrawString(a.Header.Text, lvwResults.Font, new SolidBrush(t.GetColor("call-stack-panel/column-header/foreground")), a.Bounds.X + 5, a.Bounds.Y + a.Bounds.Size.Height / 5);
                a.Graphics.DrawRectangle(new Pen(new SolidBrush(t.GetColor("call-stack-panel/column-header/border"))), a.Bounds.X - 1, a.Bounds.Y - 1, a.Bounds.Size.Width, a.Bounds.Size.Height);
            };
        }
    }
}
