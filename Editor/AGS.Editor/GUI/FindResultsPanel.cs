using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using AGS.Types.AutoComplete;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public partial class FindResultsPanel : DockContent
    {
        private List<ScriptTokenReference> _results;
        

        public FindResultsPanel()
        {
            InitializeComponent();            
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
        }

		public void SetImageList(ImageList list)
		{
			lvwResults.SmallImageList = list;
		}

        public List<ScriptTokenReference> Results
        {
            get { return _results; }
            set { _results = value; RefreshGui(); }
        }

        private void RefreshGui()
        {
            lvwResults.Items.Clear();
            if (_results != null)
            {
                PopulateListBoxWithResults();

                if (lvwResults.Items.Count > 0)
                {
                    lvwResults.Items[0].Selected = true;
                }
                
                Text = string.Format("Find Symbol Results - {0} matches found", Results.Count);
            }
        }

        private void PopulateListBoxWithResults()
        {
            foreach (ScriptTokenReference scriptTokenReference in Results)
            {
                if (scriptTokenReference.Script != null)
                {
                    ListViewItem newItem = lvwResults.Items.Add(scriptTokenReference.Script.FileName);
                    newItem.SubItems.Add((scriptTokenReference.LineIndex + 1).ToString());
                    newItem.SubItems.Add(scriptTokenReference.CurrentLine);
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
                    //Factory.GUIController.ZoomToFile(selectedItem.SubItems[0].Text, 
                    //    Convert.ToInt32(selectedItem.SubItems[1].Text));

                    ScriptTokenReference scriptTokenReference =
                            Results[lvwResults.SelectedIndices[0]];

                    if (scriptTokenReference.Script == null) return;

                    Factory.GUIController.ZoomToFile(scriptTokenReference.Script.FileName,
                        ZoomToFileZoomType.ZoomToCharacterPosition, scriptTokenReference.CharacterIndex);
                    /*if (Scintilla != null)
                    {                                
                        ScriptTokenReference scriptTokenReference = 
                            Results[lvwResults.SelectedIndices[0]];                        
                        Scintilla.SetSelection(scriptTokenReference.CharacterIndex,
                            scriptTokenReference.Token.Length);
                    }     */               
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
            lvwResults.BackColor = t.GetColor("find-results-panel/background");
            lvwResults.ForeColor = t.GetColor("find-results-panel/foreground");
            lvwResults.OwnerDraw = t.GetBool("find-results-panel/owner-draw");
            lvwResults.GridLines = t.GetBool("find-results-panel/grid-lines");
            lvwResults.Layout += (s, a) =>
            {
                lvwResults.Columns[lvwResults.Columns.Count - 1].Width = t.GetInt("find-results-panel/last-column-width");
            };
            lvwResults.DrawItem += (s, a) => a.DrawDefault = t.GetBool("find-results-panel/draw-item");
            lvwResults.DrawSubItem += (s, a) => a.DrawDefault = t.GetBool("find-results-panel/draw-sub-item");
            lvwResults.DrawColumnHeader += (s, a) =>
            {
                a.Graphics.FillRectangle(new SolidBrush(t.GetColor("find-results-panel/column-header/background")), a.Bounds);
                a.Graphics.DrawString(a.Header.Text, lvwResults.Font, new SolidBrush(t.GetColor("find-results-panel/column-header/foreground")), a.Bounds.X + 5, a.Bounds.Y + a.Bounds.Size.Height / 5);
                a.Graphics.DrawRectangle(new Pen(new SolidBrush(t.GetColor("find-results-panel/column-header/border"))), a.Bounds.X - 1, a.Bounds.Y - 1, a.Bounds.Size.Width, a.Bounds.Size.Height);
            };
        }
    }
}
