using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class FindReplaceDialog : Form
    {
		private bool _pressedReplace = false;
		private bool _showingReplaceOptions = false;
        private EditorPreferences _preferences;

        public FindReplaceDialog(string defaultSearchText, string defaultReplaceText, EditorPreferences preferences)
        {
            _preferences = preferences;
            InitializeComponent();
            btnOK.Enabled = false;

            foreach (string previousSearch in preferences.RecentSearches)
            {
                cmbFind.Items.Add(previousSearch);
            }
            
            cmbFind.Text = defaultSearchText;
			txtReplaceWith.Text = defaultReplaceText;
			btnCancel.Left = btnReplace.Left;
			cmbFind.Focus();
        }

        public string TextToFind
        {
            get { return cmbFind.Text; }
        }

		public string TextToReplaceWith
		{
			get { return txtReplaceWith.Text; }
		}

        public bool CaseSensitive
        {
            get { return chkCaseSensitive.Checked; }
            set { chkCaseSensitive.Checked = value; }
        }

		public bool ShowingReplaceDialog
		{
			set { ShowOrHideReplaceControls(value); }
			get { return _showingReplaceOptions; }
		}

		public bool IsReplace
		{
			get { return _pressedReplace; }
		}

        private void btnOK_Click(object sender, EventArgs e)
        {
            if ((_preferences.RecentSearches.Count == 0) ||
                (cmbFind.Text != _preferences.RecentSearches[0]))
            {
                if (_preferences.RecentSearches.Contains(cmbFind.Text))
                {
                    _preferences.RecentSearches.Remove(cmbFind.Text);
                }
                _preferences.RecentSearches.Insert(0, cmbFind.Text);
                _preferences.SaveToRegistry();
            }
			_pressedReplace = false;
            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

		private void btnReplace_Click(object sender, EventArgs e)
		{
			_pressedReplace = true;
			this.Close();
		}

		private void ShowOrHideReplaceControls(bool show)
		{
			_showingReplaceOptions = show;
			txtReplaceWith.Visible = show;
			lblReplaceWith.Visible = show;
			btnReplace.Visible = show;
			if (show)
			{
				cmdToggleReplace.Text = "<< F&ind";
				btnCancel.Left = btnReplace.Right + 20;
			}
			else
			{
				cmdToggleReplace.Text = "&Replace >>";
				btnCancel.Left = btnReplace.Left;
			}
		}

		private void cmdToggleReplace_Click(object sender, EventArgs e)
		{
			ShowOrHideReplaceControls(!txtReplaceWith.Visible);
		}

        private void cmbFind_TextChanged(object sender, EventArgs e)
        {
            btnOK.Enabled = (cmbFind.Text.Length > 0);
        }

    }
}