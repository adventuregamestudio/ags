using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using AGS.Types.Enums;
using AGS.Editor.TextProcessing;

namespace AGS.Editor
{
    public partial class FindReplaceDialog : Form
    {
		private bool _pressedReplace = false;
		private bool _showingReplaceOptions = false;
        private bool _showingFindReplaceAll = false;
        private bool _closing = false;
        private FindReplace _findReplace;

        private const string LOOK_IN_CURRENT_DOCUMENT = "Current Document";
        private const string LOOK_IN_CURRENT_PROJECT = "Current Project";

        private static string _lastSelectedLookIn;
        private static bool _lastSelectedCaseSensitive;

        public FindReplaceDialog(string defaultSearchText, 
            string defaultReplaceText, FindReplace findReplace)
        {
            _findReplace = findReplace;
            InitializeComponent();
            btnOK.Enabled = false;

            foreach (string previousSearch in Factory.AGSEditor.Settings.RecentSearches)
            {
                cmbFind.Items.Add(previousSearch);
                cmbReplace.Items.Add(previousSearch);
            }

            cmbLookIn.Items.Add(LOOK_IN_CURRENT_DOCUMENT);
            cmbLookIn.Items.Add(LOOK_IN_CURRENT_PROJECT);
            cmbLookIn.Text = _lastSelectedLookIn ?? LOOK_IN_CURRENT_DOCUMENT;

            chkCaseSensitive.Checked = _lastSelectedCaseSensitive;
            
            cmbFind.Text = defaultSearchText;
			cmbReplace.Text = defaultReplaceText;
			btnCancel.Left = btnReplace.Left;
			cmbFind.Focus();
        }

        public LookInDocumentType LookIn
        {
            get
            { 
                switch (cmbLookIn.Text)
                {
                    case LOOK_IN_CURRENT_DOCUMENT:
                        return LookInDocumentType.CurrentDocument;
                    case LOOK_IN_CURRENT_PROJECT:
                        return LookInDocumentType.CurrentProject;
                    default:
                        return LookInDocumentType.CurrentDocument;
                }
            }
        }

        public string TextToFind
        {
            get { return cmbFind.Text; }
        }

		public string TextToReplaceWith
		{
            get { return cmbReplace.Text; }
		}

        public bool CaseSensitive
        {
            get { return chkCaseSensitive.Checked; }
        }

		public bool ShowingReplaceDialog
		{
			set { ShowOrHideReplaceControls(value); }
			get { return _showingReplaceOptions; }
		}

        public bool ShowingAllDialog
        {
            get { return _showingFindReplaceAll; }
            set { ShowOrHideFindReplaceAll(value); }
        }

		public bool IsReplace
		{
			get { return _pressedReplace; }
		}
        
        private void btnOK_Click(object sender, EventArgs e)
        {
            if ((Factory.AGSEditor.Settings.RecentSearches.Count == 0) ||
                (cmbFind.Text != Factory.AGSEditor.Settings.RecentSearches[0]))
            {
                if (Factory.AGSEditor.Settings.RecentSearches.Contains(cmbFind.Text))
                {
                    Factory.AGSEditor.Settings.RecentSearches.Remove(cmbFind.Text);
                }
                Factory.AGSEditor.Settings.RecentSearches.Insert(0, cmbFind.Text);
                Factory.AGSEditor.Settings.Save();
            }
            _pressedReplace = false;
            FindReplace();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            
            this.Close();
        }

		private void btnReplace_Click(object sender, EventArgs e)
		{
			_pressedReplace = true;
            FindReplace();
            //this.Close();
		}

        private void FindReplace()
        {
            if (!_findReplace.PerformFindReplace())
            {
                this.Close();
            }            
        }

        private void ShowOrHideFindReplaceAll(bool show)
        {
            _showingFindReplaceAll = show;            
            if (show)
            {
                btnReplace.Text = "&Replace All...";
                btnOK.Text = "&Find All...";
            }
            else
            {
                btnReplace.Text = "&Replace";
                btnOK.Text = "&Find Next";
            }
        }

		private void ShowOrHideReplaceControls(bool show)
		{
			_showingReplaceOptions = show;
            cmbReplace.Visible = show;
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
            ShowOrHideReplaceControls(!cmbReplace.Visible);
		}

        private void cmbFind_TextChanged(object sender, EventArgs e)
        {
            btnOK.Enabled = (cmbFind.Text.Length > 0);
        }

        private void onFormClosed(object sender, FormClosedEventArgs e)
        {
            _lastSelectedLookIn = cmbLookIn.Text;
            _lastSelectedCaseSensitive = chkCaseSensitive.Checked;
        }

        private void onFormDeactivated(object sender, EventArgs e)
        {
            
            if (!_closing) this.Opacity = 0.7;
        }

        private void onFormActivated(object sender, EventArgs e)
        {
            this.Opacity = 1.0;
        }

        private void FindReplaceDialog_FormClosing(object sender, FormClosingEventArgs e)
        {
            _closing = true;
        }

    }
}