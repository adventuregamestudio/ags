using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class StartNewGameWizardPage2 : WizardPage
    {
        public StartNewGameWizardPage2(string baseDirectory)
        {
            InitializeComponent();
            txtCreateInFolder.Text = baseDirectory;
        }

        public string NewGameName
        {
            get { return txtFriendlyName.Text; }
        }

        public string GetFullPath()
        {
            try
            {
                return Path.Combine(txtCreateInFolder.Text, txtFileName.Text);
            }
            catch (ArgumentException)
            {
                return "[invalid path]";
            }
        }

        private bool VerifyPathContainsValidCharacters()
        {
            foreach (char c in GetFullPath())
            {
                foreach (char invalidChar in Path.GetInvalidPathChars())
                {
                    if (invalidChar == c)
                    {
                        return false;
                    }
                }
            }
            if (!txtCreateInFolder.Text.Contains(@"\"))
            {
                // Path must contain at least one \
                return false;
            }
            if (!txtCreateInFolder.Text.Contains(":"))
            {
                // Path must contain at least one :
                return false;
            }
            return true;
        }

        public override bool NextButtonPressed()
        {
            if ((txtFileName.Text.Length > 0) &&
                (txtFriendlyName.Text.Length > 0) &&
                (txtCreateInFolder.Text.Length > 0))
            {
                if (!Utilities.DoesFileNameContainOnlyValidCharacters(txtFileName.Text))
                {
                    Factory.GUIController.ShowMessage("The file name you have specified includes some invalid characters. Please use just letters and numbers.", MessageBoxIcon.Warning);
                    return false;
                }
                if (!VerifyPathContainsValidCharacters())
                {
                    Factory.GUIController.ShowMessage("The folder name you have specified includes some invalid characters. Please use just letters and numbers.", MessageBoxIcon.Warning);
                    return false;
                }
                if (Directory.Exists(GetFullPath()))
                {
                    Factory.GUIController.ShowMessage("The directory '" + GetFullPath() + "', already exists. Please choose another file name.", MessageBoxIcon.Warning);
                    return false;
                }
                return true;
            }
            Factory.GUIController.ShowMessage("You must type in a file name and a game name to continue.", MessageBoxIcon.Information);
            return false;
        }

        public override string TitleText
        {
            get
            {
                return "Decide what your game's name will be.";
            }
        }

        public override void PageShown()
        {
            txtFriendlyName.Focus();
        }

        private void txtFileName_TextChanged(object sender, EventArgs e)
        {
            if (txtFileName.Text.Length > 0)
            {
                lblFilePath.Text = "Game will be created in: " + GetFullPath();
            }
            else
            {
                lblFilePath.Text = string.Empty;
            }
        }

        private void txtCreateInFolder_TextChanged(object sender, EventArgs e)
        {
            txtFileName_TextChanged(sender, e);
        }

        private void btnCreateInBrowse_Click(object sender, EventArgs e)
        {
            txtCreateInFolder.Text = Factory.GUIController.ShowSelectFolderOrDefaultDialog("Please select the folder that you wish to create your new project in.", txtCreateInFolder.Text);
        }
    }
}
