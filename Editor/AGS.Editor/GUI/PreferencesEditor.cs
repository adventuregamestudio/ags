using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class PreferencesEditor : Form
    {
        private EditorPreferences _preferences;

        public PreferencesEditor(EditorPreferences prefs)
        {
            InitializeComponent();
            _preferences = prefs;
			// just in case they had it set to something silly in 2.72
			if (_preferences.TabSize < udTabWidth.Minimum) _preferences.TabSize = (int)udTabWidth.Minimum;
			if (_preferences.TabSize > udTabWidth.Maximum) _preferences.TabSize = (int)udTabWidth.Maximum;

            udTabWidth.Value = _preferences.TabSize;
            cmbTestGameStyle.SelectedIndex = (int)_preferences.TestGameStyle;
			cmbEditorStartup.SelectedIndex = (int)_preferences.StartupPane;
			radFolderPath.Checked = (_preferences.DefaultImportPath != string.Empty);
			txtImportPath.Text = _preferences.DefaultImportPath;
			txtImportPath.Enabled = radFolderPath.Checked;
			btnChooseFolder.Enabled = txtImportPath.Enabled;
			radNewGameSpecificPath.Checked = (_preferences.ExplicitNewGamePath != string.Empty);
			txtNewGamePath.Text = _preferences.ExplicitNewGamePath;
			txtNewGamePath.Enabled = radNewGameSpecificPath.Checked;
			btnNewGameChooseFolder.Enabled = radNewGameSpecificPath.Checked;
			cmbMessageOnCompile.SelectedIndex = (int)_preferences.MessageBoxOnCompileErrors;
			cmbIndentStyle.SelectedIndex = _preferences.IndentUsingTabs ? 1 : 0;
			chkAlwaysShowViewPreview.Checked = _preferences.ShowViewPreviewByDefault;
			txtPaintProgram.Text = _preferences.PaintProgramPath;
			radPaintProgram.Checked = (_preferences.PaintProgramPath != string.Empty);
			txtPaintProgram.Enabled = radPaintProgram.Checked;
			btnSelectPaintProgram.Enabled = txtPaintProgram.Enabled;
			cmbSpriteImportTransparency.SelectedIndex = (int)_preferences.DefaultSpriteImportTransparency;
            chkUsageInfo.Checked = _preferences.SendAnonymousStats;
            chkBackupReminders.Checked = (_preferences.BackupWarningInterval != 0);
            udBackupInterval.Value = (_preferences.BackupWarningInterval > 0) ? _preferences.BackupWarningInterval : 1;
            udBackupInterval.Enabled = chkBackupReminders.Checked;
            chkRemapBgImport.Checked = _preferences.RemapPalettizedBackgrounds;
            chkKeepHelpOnTop.Checked = _preferences.KeepHelpOnTop;
            chkUseLegacyCompiler.Checked = _preferences.UseLegacyCompiler;
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

            _preferences.TabSize = Convert.ToInt32(udTabWidth.Value);
            _preferences.TestGameStyle = (TestGameWindowStyle)cmbTestGameStyle.SelectedIndex;
			_preferences.StartupPane = (EditorStartupPane)cmbEditorStartup.SelectedIndex;
			_preferences.DefaultImportPath = (radGamePath.Checked ? string.Empty : txtImportPath.Text);
			_preferences.MessageBoxOnCompileErrors = (MessageBoxOnCompile)cmbMessageOnCompile.SelectedIndex;
			_preferences.IndentUsingTabs = (cmbIndentStyle.SelectedIndex == 1);
			_preferences.ShowViewPreviewByDefault = chkAlwaysShowViewPreview.Checked;
			_preferences.PaintProgramPath = (radDefaultPaintProgram.Checked ? string.Empty : txtPaintProgram.Text);
			_preferences.DefaultSpriteImportTransparency = (SpriteImportMethod)cmbSpriteImportTransparency.SelectedIndex;
			_preferences.ExplicitNewGamePath = (radNewGameMyDocs.Checked ? string.Empty : txtNewGamePath.Text);
            _preferences.SendAnonymousStats = chkUsageInfo.Checked;
            _preferences.BackupWarningInterval = (chkBackupReminders.Checked ? (int)udBackupInterval.Value : 0);
            _preferences.RemapPalettizedBackgrounds = chkRemapBgImport.Checked;
            _preferences.KeepHelpOnTop = chkKeepHelpOnTop.Checked;
            _preferences.UseLegacyCompiler = chkUseLegacyCompiler.Checked;
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
    }
}