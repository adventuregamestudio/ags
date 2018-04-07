using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using AGS.Editor.Preferences;

namespace AGS.Editor
{
    public partial class PreferencesEditor : Form
    {
        public PreferencesEditor()
        {
            InitializeComponent();
			// just in case they had it set to something silly in 2.72
			if (Factory.AGSEditor.Settings.TabSize < udTabWidth.Minimum) Factory.AGSEditor.Settings.TabSize = (int)udTabWidth.Minimum;
			if (Factory.AGSEditor.Settings.TabSize > udTabWidth.Maximum) Factory.AGSEditor.Settings.TabSize = (int)udTabWidth.Maximum;

            udTabWidth.Value = Factory.AGSEditor.Settings.TabSize;
            cmbTestGameStyle.SelectedIndex = (int)Factory.AGSEditor.Settings.TestGameWindowStyle;
			cmbEditorStartup.SelectedIndex = (int)Factory.AGSEditor.Settings.StartupPane;
			radFolderPath.Checked = (Factory.AGSEditor.Settings.DefaultImportPath != string.Empty);
			txtImportPath.Text = Factory.AGSEditor.Settings.DefaultImportPath;
			txtImportPath.Enabled = radFolderPath.Checked;
			btnChooseFolder.Enabled = txtImportPath.Enabled;
			radNewGameSpecificPath.Checked = (Factory.AGSEditor.Settings.ExplicitNewGamePath != string.Empty);
			txtNewGamePath.Text = Factory.AGSEditor.Settings.ExplicitNewGamePath;
			txtNewGamePath.Enabled = radNewGameSpecificPath.Checked;
			btnNewGameChooseFolder.Enabled = radNewGameSpecificPath.Checked;
			cmbMessageOnCompile.SelectedIndex = (int)Factory.AGSEditor.Settings.MessageBoxOnCompile;
			cmbIndentStyle.SelectedIndex = Factory.AGSEditor.Settings.IndentUseTabs ? 1 : 0;
			chkAlwaysShowViewPreview.Checked = Factory.AGSEditor.Settings.ShowViewPreviewByDefault;
			txtPaintProgram.Text = Factory.AGSEditor.Settings.PaintProgramPath;
			radPaintProgram.Checked = (Factory.AGSEditor.Settings.PaintProgramPath != string.Empty);
			txtPaintProgram.Enabled = radPaintProgram.Checked;
			btnSelectPaintProgram.Enabled = txtPaintProgram.Enabled;
			cmbSpriteImportTransparency.SelectedIndex = (int)Factory.AGSEditor.Settings.SpriteImportMethod;
            cmbColorTheme.DataSource = Factory.GUIController.ColorThemes.Themes;
            cmbColorTheme.SelectedIndex = Factory.GUIController.ColorThemes.Themes.ToList().FindIndex(t => t.Name == Factory.AGSEditor.Settings.ColorTheme);
            chkUsageInfo.Checked = Factory.AGSEditor.Settings.SendAnonymousStats;
            chkBackupReminders.Checked = (Factory.AGSEditor.Settings.BackupWarningInterval != 0);
            udBackupInterval.Value = (Factory.AGSEditor.Settings.BackupWarningInterval > 0) ? Factory.AGSEditor.Settings.BackupWarningInterval : 1;
            udBackupInterval.Enabled = chkBackupReminders.Checked;
            chkRemapBgImport.Checked = Factory.AGSEditor.Settings.RemapPalettizedBackgrounds;
            chkKeepHelpOnTop.Checked = Factory.AGSEditor.Settings.KeepHelpOnTop;
            chkPromptDialogOnTabsClose.Checked = Factory.AGSEditor.Settings.DialogOnMultibleTabsClose;
            Utilities.CheckLabelWidthsOnForm(this);
		}

        private void btnOK_Click(object sender, EventArgs e)
        {
			if ((txtImportPath.Text.Length > 0) &&
				(!System.IO.Directory.Exists(txtImportPath.Text)))
			{
				MessageBox.Show("The directory you have selected for import does not exist. Please enter a valid directory.", "Invalid directory", MessageBoxButtons.OK, MessageBoxIcon.Warning);
				this.DialogResult = DialogResult.None;
				return;
			}

			if ((radPaintProgram.Checked) &&
				(txtPaintProgram.Text.Length > 0) &&
				(!System.IO.File.Exists(txtPaintProgram.Text)))
			{
				MessageBox.Show("The paint program you have selected does not exist. Please select a valid application.", "Invalid file", MessageBoxButtons.OK, MessageBoxIcon.Warning);
				this.DialogResult = DialogResult.None;
				return;
			}

            Factory.AGSEditor.Settings.TabSize = Convert.ToInt32(udTabWidth.Value);
            Factory.AGSEditor.Settings.TestGameWindowStyle = (TestGameWindowStyle)cmbTestGameStyle.SelectedIndex;
			Factory.AGSEditor.Settings.StartupPane = (StartupPane)cmbEditorStartup.SelectedIndex;
			Factory.AGSEditor.Settings.DefaultImportPath = (radGamePath.Checked ? string.Empty : txtImportPath.Text);
			Factory.AGSEditor.Settings.MessageBoxOnCompile = (MessageBoxOnCompile)cmbMessageOnCompile.SelectedIndex;
			Factory.AGSEditor.Settings.IndentUseTabs = (cmbIndentStyle.SelectedIndex == 1);
			Factory.AGSEditor.Settings.ShowViewPreviewByDefault = chkAlwaysShowViewPreview.Checked;
			Factory.AGSEditor.Settings.PaintProgramPath = (radDefaultPaintProgram.Checked ? string.Empty : txtPaintProgram.Text);
			Factory.AGSEditor.Settings.SpriteImportMethod = (SpriteImportMethod)cmbSpriteImportTransparency.SelectedIndex;
			Factory.AGSEditor.Settings.ExplicitNewGamePath = (radNewGameMyDocs.Checked ? string.Empty : txtNewGamePath.Text);
            Factory.AGSEditor.Settings.SendAnonymousStats = chkUsageInfo.Checked;
            Factory.AGSEditor.Settings.BackupWarningInterval = (chkBackupReminders.Checked ? (int)udBackupInterval.Value : 0);
            Factory.AGSEditor.Settings.RemapPalettizedBackgrounds = chkRemapBgImport.Checked;
            Factory.AGSEditor.Settings.KeepHelpOnTop = chkKeepHelpOnTop.Checked;
            Factory.AGSEditor.Settings.DialogOnMultibleTabsClose = chkPromptDialogOnTabsClose.Checked;

            if ((ColorTheme)cmbColorTheme.SelectedItem != Factory.GUIController.ColorThemes.Current)
            {
                Factory.GUIController.ShowMessage(
                    "You must restart the editor for changed color theme to work properly.",
                    MessageBoxIcon.Information);
                Factory.GUIController.ColorThemes.Current = (ColorTheme)cmbColorTheme.SelectedItem;
            }
        }

		private void radFolderPath_CheckedChanged(object sender, EventArgs e)
		{
			txtImportPath.Enabled = radFolderPath.Checked;
			btnChooseFolder.Enabled = txtImportPath.Enabled;
		}

		private void radGamePath_CheckedChanged(object sender, EventArgs e)
		{
			txtImportPath.Enabled = radFolderPath.Checked;
			btnChooseFolder.Enabled = txtImportPath.Enabled;
		}

		private void btnChooseFolder_Click(object sender, EventArgs e)
		{
            txtImportPath.Text = Factory.GUIController.ShowSelectFolderOrDefaultDialog("Please select the folder that you wish to import files from.", txtImportPath.Text, false);
		}

		private void btnSelectPaintProgram_Click(object sender, EventArgs e)
		{
			string selectedFile = Factory.GUIController.ShowOpenFileDialog("Select paint program to use", "Executable files (*.exe)|*.exe", false);
			if (selectedFile != null)
			{
				txtPaintProgram.Text = selectedFile;
			}
		}

		private void radDefaultPaintProgram_CheckedChanged(object sender, EventArgs e)
		{
			radPaintProgram_CheckedChanged(sender, e);
		}

		private void radPaintProgram_CheckedChanged(object sender, EventArgs e)
		{
			txtPaintProgram.Enabled = radPaintProgram.Checked;
			btnSelectPaintProgram.Enabled = radPaintProgram.Checked;
		}

		private void btnNewGameChooseFolder_Click(object sender, EventArgs e)
		{
            txtNewGamePath.Text = Factory.GUIController.ShowSelectFolderOrDefaultDialog("Please select the folder that you wish to make a default for your projects.", txtNewGamePath.Text);
		}

		private void radNewGameMyDocs_CheckedChanged(object sender, EventArgs e)
		{
			radNewGameSpecificPath_CheckedChanged(sender, e);
		}

		private void radNewGameSpecificPath_CheckedChanged(object sender, EventArgs e)
		{
			txtNewGamePath.Enabled = radNewGameSpecificPath.Checked;
			btnNewGameChooseFolder.Enabled = radNewGameSpecificPath.Checked;
		}

        private void lnkUsageInfo_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Factory.GUIController.LaunchHelpForKeyword("Anonymous usage information");
        }

        private void chkBackupReminders_CheckedChanged(object sender, EventArgs e)
        {
            udBackupInterval.Enabled = chkBackupReminders.Checked;
        }

        private void btnImportColorTheme_Click(object sender, EventArgs e)
        {
            OpenFileDialog file = new OpenFileDialog { Filter = "JSON files (*.json)|*.json" };

            if (file.ShowDialog() == DialogResult.OK)
            {
                Factory.GUIController.ColorThemes.Import(file.FileName);
            }
        }

        private void cmbColorTheme_DropDown(object sender, EventArgs e)
        {
            Factory.GUIController.ColorThemes.Load();
            cmbColorTheme.DataSource = null;
            cmbColorTheme.DataSource = Factory.GUIController.ColorThemes.Themes;
        }
    }
}