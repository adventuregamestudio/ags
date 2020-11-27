using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.IO;
using System.Text;
using System.Windows.Forms;
using AGS.Editor.Utils;

namespace AGS.Editor
{
    public partial class StartNewGameWizardPage2 : WizardPage
    {
        public StartNewGameWizardPage2(string baseDirectory)
        {
            InitializeComponent();
			txtCreateInFolder.Text = baseDirectory;
        }

        public string FileName
        {
            get { return txtFileName.Text; }
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

        public override bool NextButtonPressed()
        {
            if (txtFriendlyName.TextLength == 0)
            {
                Factory.GUIController.ShowMessage("You must enter a name for your game", MessageBoxIcon.Warning);
                return false;
            }

            if (txtFileName.TextLength == 0)
            {
                Factory.GUIController.ShowMessage("You must enter a file name for your game", MessageBoxIcon.Warning);
                return false;
            }

            if (txtCreateInFolder.TextLength == 0)
            {
                Factory.GUIController.ShowMessage("You must choose a directory where the game project will be created", MessageBoxIcon.Warning);
                return false;
            }

            if (!Validation.FilenameIsValid(txtFileName.Text) ||
                !Validation.StringIsAsciiCharactersOnly(txtFileName.Text))
            {
                Factory.GUIController.ShowMessage("The file name you have specified includes some invalid characters. Please use just letters and numbers", MessageBoxIcon.Warning);
                return false;
            }

            if (!Validation.PathIsValid(txtCreateInFolder.Text))
            {
                Factory.GUIController.ShowMessage("The path to the directory you have specified includes some invalid characters", MessageBoxIcon.Warning);
                return false;
            }

            string fullpath;
            try
            {
                fullpath = Path.Combine(txtCreateInFolder.Text, txtFileName.Text);
            }
            catch
            {
                Factory.GUIController.ShowMessage("Error combining full project path" , MessageBoxIcon.Error);
                return false;
            }

            if (!Validation.PathIsAbsolute(fullpath) ||
                !Validation.PathIsAbsoluteDriveLetter(fullpath))
            {
                Factory.GUIController.ShowMessage("The project directory must be an absolute path that starts with a drive letter", MessageBoxIcon.Warning);
                return false;
            }

            if (!Validation.PathIsAvailable(fullpath))
            {
                Factory.GUIController.ShowMessage("The chosen project directory already exists", MessageBoxIcon.Warning);
                return false;
            }

            return true;
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
