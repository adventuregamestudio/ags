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
    }
}
