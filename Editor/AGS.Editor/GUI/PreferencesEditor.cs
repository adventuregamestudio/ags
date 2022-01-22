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
        private AppSettings _settings;
        public void UpdateControlsForPreferences()
        {
            txtImportPath.Text = _settings.DefaultImportPath;
            radFolderPath.Checked = (_settings.DefaultImportPath != string.Empty);
            radGamePath.Checked = (_settings.DefaultImportPath == string.Empty);
            txtImportPath.Enabled = radFolderPath.Checked;
            btnChooseFolder.Enabled = txtImportPath.Enabled;

            txtNewGamePath.Text = _settings.NewGamePath;
            radNewGameSpecificPath.Checked = (_settings.NewGamePath != string.Empty);
            radNewGameMyDocs.Checked = (_settings.NewGamePath == string.Empty);
            txtNewGamePath.Enabled = radNewGameSpecificPath.Checked;
            btnNewGameChooseFolder.Enabled = radNewGameSpecificPath.Checked;

            txtPaintProgram.Text = _settings.PaintProgramPath;
            radPaintProgram.Checked = (_settings.PaintProgramPath != string.Empty);
            radDefaultPaintProgram.Checked = (_settings.PaintProgramPath == string.Empty);
            txtPaintProgram.Enabled = radPaintProgram.Checked;
            btnSelectPaintProgram.Enabled = txtPaintProgram.Enabled;

            udTabWidth.Value = _settings.TabSize;
            cmbTestGameStyle.SelectedIndex = (int)_settings.TestGameWindowStyle;
            cmbEditorStartup.SelectedIndex = (int)_settings.StartupPane;
            cmbMessageOnCompile.SelectedIndex = (int)_settings.MessageBoxOnCompile;
            cmbIndentStyle.SelectedIndex = _settings.IndentUseTabs ? 1 : 0;
            chkAlwaysShowViewPreview.Checked = _settings.ShowViewPreviewByDefault;
            cmbSpriteImportTransparency.SelectedIndex = (int)_settings.SpriteImportMethod;
            cmbColorTheme.DataSource = Factory.GUIController.ColorThemes.Themes;
            cmbColorTheme.SelectedIndex = Factory.GUIController.ColorThemes.Themes.ToList().FindIndex(t => t.Name == _settings.ColorTheme);
            chkUsageInfo.Checked = _settings.SendAnonymousStats;
            udBackupInterval.Value = (_settings.BackupWarningInterval > 0) ? _settings.BackupWarningInterval : 1;
            chkBackupReminders.Checked = (_settings.BackupWarningInterval != 0);
            udBackupInterval.Enabled = chkBackupReminders.Checked;
            chkRemapBgImport.Checked = _settings.RemapPalettizedBackgrounds;
            chkKeepHelpOnTop.Checked = _settings.KeepHelpOnTop;
            chkPromptDialogOnTabsClose.Checked = _settings.DialogOnMultipleTabsClose;
            cmbScriptReloadOnExternalChange.SelectedIndex = (int)_settings.ReloadScriptOnExternalChange;
        }

        public PreferencesEditor()
        {
            InitializeComponent();

            _settings = Factory.AGSEditor.Settings.CloneAppSettings();
            // just in case they had it set to something silly in 2.72
            if (_settings.TabSize < udTabWidth.Minimum) _settings.TabSize = (int)udTabWidth.Minimum;
            if (_settings.TabSize > udTabWidth.Maximum) _settings.TabSize = (int)udTabWidth.Maximum;

            UpdateControlsForPreferences();

            propertyGridPreferences.SelectedObject = _settings;
            Utilities.CheckLabelWidthsOnForm(this);
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            cmbColorTheme.SelectedIndex = Factory.GUIController.ColorThemes.Themes.ToList().FindIndex(t => t.Name == _settings.ColorTheme);
            if ((ColorTheme)cmbColorTheme.SelectedItem != Factory.GUIController.ColorThemes.Current)
            {
                Factory.GUIController.ShowMessage(
                    "You must restart the editor for changed color theme to work properly.",
                    MessageBoxIcon.Information);
                Factory.GUIController.ColorThemes.Current = (ColorTheme)cmbColorTheme.SelectedItem;
            }

            Factory.AGSEditor.Settings.Apply(_settings);
        }

		private void radFolderPath_CheckedChanged(object sender, EventArgs e)
		{
			txtImportPath.Enabled = radFolderPath.Checked;
			btnChooseFolder.Enabled = txtImportPath.Enabled;
            _settings.DefaultImportPath = (radGamePath.Checked ? string.Empty : txtImportPath.Text);
        }

		private void radGamePath_CheckedChanged(object sender, EventArgs e)
		{
			txtImportPath.Enabled = radFolderPath.Checked;
			btnChooseFolder.Enabled = txtImportPath.Enabled;

            _settings.DefaultImportPath = (radGamePath.Checked ? string.Empty : txtImportPath.Text);
        }

		private void btnChooseFolder_Click(object sender, EventArgs e)
		{
            string selectedPath = Factory.GUIController.ShowSelectFolderOrDefaultDialog("Please select the folder that you wish to import files from.", txtImportPath.Text, false);
            if (!string.IsNullOrEmpty(selectedPath))
            {
                if (System.IO.Directory.Exists(selectedPath))
                {
                    txtImportPath.Text = selectedPath;
                    _settings.DefaultImportPath = txtImportPath.Text;
                    return;
                }
                else
                {
                    MessageBox.Show("The directory you have selected does not exist. Please select a valid path.", "Invalid directory", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }

            radFolderPath.Checked = !string.IsNullOrEmpty(_settings.DefaultImportPath);
            radGamePath.Checked = string.IsNullOrEmpty(_settings.DefaultImportPath);
            txtImportPath.Text = _settings.DefaultImportPath;
        }

		private void btnSelectPaintProgram_Click(object sender, EventArgs e)
		{
			string selectedFile = Factory.GUIController.ShowOpenFileDialog("Select paint program to use", "Executable files (*.exe)|*.exe", false);
			if (!string.IsNullOrEmpty(selectedFile))
			{
                if (System.IO.File.Exists(selectedFile))
                {
                    txtPaintProgram.Text = selectedFile;
                    _settings.PaintProgramPath = txtPaintProgram.Text;
                    return;
                }
                else
                {
                    MessageBox.Show("The paint program you have selected does not exist. Please select a valid application.", "Invalid file", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }
            
            radDefaultPaintProgram.Checked = string.IsNullOrEmpty(_settings.PaintProgramPath);
            radPaintProgram.Checked = !string.IsNullOrEmpty(_settings.PaintProgramPath);
            txtPaintProgram.Text = _settings.PaintProgramPath;
        }

		private void radDefaultPaintProgram_CheckedChanged(object sender, EventArgs e)
		{
			radPaintProgram_CheckedChanged(sender, e);
		}

		private void radPaintProgram_CheckedChanged(object sender, EventArgs e)
		{
			txtPaintProgram.Enabled = radPaintProgram.Checked;
			btnSelectPaintProgram.Enabled = radPaintProgram.Checked;

            _settings.PaintProgramPath = (radDefaultPaintProgram.Checked ? string.Empty : txtPaintProgram.Text);
        }

        private void btnNewGameChooseFolder_Click(object sender, EventArgs e)
        {
            string selectedPath = Factory.GUIController.ShowSelectFolderOrDefaultDialog("Please select the folder that you wish to make a default for your projects.", txtNewGamePath.Text);
            if (!string.IsNullOrEmpty(selectedPath))
            {
                if (System.IO.Directory.Exists(selectedPath))
                {
                    txtNewGamePath.Text = selectedPath;
                    _settings.NewGamePath = txtNewGamePath.Text;
                    return;
                }
                else
                {
                    MessageBox.Show("The directory you have selected does not exist. Please select a valid path.", "Invalid directory", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }

            radNewGameSpecificPath.Checked = !string.IsNullOrEmpty(_settings.NewGamePath);
            radNewGameMyDocs.Checked = string.IsNullOrEmpty(_settings.NewGamePath);
            txtNewGamePath.Text = _settings.NewGamePath;
        }

		private void radNewGameMyDocs_CheckedChanged(object sender, EventArgs e)
		{
			radNewGameSpecificPath_CheckedChanged(sender, e);
		}

		private void radNewGameSpecificPath_CheckedChanged(object sender, EventArgs e)
		{
			txtNewGamePath.Enabled = radNewGameSpecificPath.Checked;
			btnNewGameChooseFolder.Enabled = radNewGameSpecificPath.Checked;

            _settings.NewGamePath = (radNewGameMyDocs.Checked ? string.Empty : txtNewGamePath.Text);
        }

        private void lnkUsageInfo_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
            Factory.GUIController.LaunchHelpForKeyword("Anonymous usage information");
        }

        private void chkBackupReminders_CheckedChanged(object sender, EventArgs e)
        {
            udBackupInterval.Enabled = chkBackupReminders.Checked;
            _settings.BackupWarningInterval = (chkBackupReminders.Checked ? (int)udBackupInterval.Value : 0);
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
        
        private void udBackupInterval_ValueChanged(object sender, EventArgs e)
        {
            if (chkBackupReminders.Checked) {
                _settings.BackupWarningInterval = (int)udBackupInterval.Value;
            }
        }

        private void udTabWidth_ValueChanged(object sender, EventArgs e)
        {
            _settings.TabSize = Convert.ToInt32(udTabWidth.Value);
        }

        private void chkAlwaysShowViewPreview_CheckedChanged(object sender, EventArgs e)
        {
            _settings.ShowViewPreviewByDefault = chkAlwaysShowViewPreview.Checked;
        }


        private void chkUsageInfo_CheckedChanged(object sender, EventArgs e)
        {
            _settings.SendAnonymousStats = chkUsageInfo.Checked;
        }

        private void chkRemapBgImport_CheckedChanged(object sender, EventArgs e)
        {
            _settings.RemapPalettizedBackgrounds = chkRemapBgImport.Checked;
        }

        private void chkKeepHelpOnTop_CheckedChanged(object sender, EventArgs e)
        {
            _settings.KeepHelpOnTop = chkKeepHelpOnTop.Checked;
        }

        private void chkPromptDialogOnTabsClose_CheckedChanged(object sender, EventArgs e)
        {
            _settings.DialogOnMultipleTabsClose = chkPromptDialogOnTabsClose.Checked;
        }

        private void cmbTestGameStyle_SelectedIndexChanged(object sender, EventArgs e)
        {
            var newIndex = (TestGameWindowStyle)((ComboBox)sender).SelectedIndex;
            if (_settings.TestGameWindowStyle != newIndex)
            {
                _settings.TestGameWindowStyle = newIndex;
            }
        }

        private void cmbEditorStartup_SelectedIndexChanged(object sender, EventArgs e)
        {
            var newIndex = (StartupPane)((ComboBox)sender).SelectedIndex;
            if (_settings.StartupPane != newIndex)
            {
                _settings.StartupPane = newIndex;
            }
        }

        private void cmbMessageOnCompile_SelectedIndexChanged(object sender, EventArgs e)
        {
            var newIndex = (MessageBoxOnCompile)((ComboBox)sender).SelectedIndex;
            if (_settings.MessageBoxOnCompile != newIndex)
            {
                _settings.MessageBoxOnCompile = newIndex;
            }
        }

        private void cmbIndentStyle_SelectedIndexChanged(object sender, EventArgs e)
        {
            bool newIndentStyle = (cmbIndentStyle.SelectedIndex == 1);
            if (_settings.IndentUseTabs != newIndentStyle)
            {
                _settings.IndentUseTabs = newIndentStyle;
            }
        }
        private void cmbSpriteImportTransparency_SelectedIndexChanged(object sender, EventArgs e)
        {
            var newIndex = (SpriteImportMethod)((ComboBox)sender).SelectedIndex;
            if (_settings.SpriteImportMethod != newIndex)
            {
                _settings.SpriteImportMethod = newIndex;
            }

        }

        private void cmbScriptReloadOnExternalChange_SelectedIndexChanged(object sender, EventArgs e)
        {
            var newIndex = (ReloadScriptOnExternalChange)((ComboBox)sender).SelectedIndex;
            if(_settings.ReloadScriptOnExternalChange != newIndex)
            {
                _settings.ReloadScriptOnExternalChange = newIndex;
            }
        }

        private void cmbColorTheme_SelectedIndexChanged(object sender, EventArgs e)
        {
            string newColorTheme = ((ColorTheme)cmbColorTheme.SelectedItem).Name;
            if (_settings.ColorTheme != newColorTheme)
            {
                _settings.ColorTheme = newColorTheme;
            }
        }

        private void tabControl1_Selected(object sender, TabControlEventArgs e)
        {
            TabControl tc = (TabControl)sender;
            if (tc.SelectedTab != this.tabPageLast)
            {
                UpdateControlsForPreferences();
            }
            else
            {
                this.propertyGridPreferences.Refresh();
            }
        }

    }
}