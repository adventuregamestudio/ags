using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using AGS.Editor.Preferences;
using AGS.Editor.Utils;
using AGS.Types;

namespace AGS.Editor
{
    public partial class PreferencesEditor : Form
    {
        private AppSettings _settings;

        private void UpdateAndroidKeystorePasswordState()
        {
            if (checkBoxAndroidShowPassword.Checked)
            {
                textBoxAndKeystorePassword.UseSystemPasswordChar = PasswordPropertyTextAttribute.No.Password;
                textBoxAndKeystoreKeyPassword.UseSystemPasswordChar = PasswordPropertyTextAttribute.No.Password;
            }
            else
            {
                // Hides password using dot list character
                textBoxAndKeystorePassword.UseSystemPasswordChar = PasswordPropertyTextAttribute.Yes.Password;
                textBoxAndKeystoreKeyPassword.UseSystemPasswordChar = PasswordPropertyTextAttribute.Yes.Password;
            }
        }

        // Make sure panels can have font and styles updated after settings is applied without restarting ags editor
        private void UpdateFontSettings()
        {
            foreach (ContentDocument pane in Factory.GUIController.Panes)
            {
                ScriptEditor scriptEditor = pane.Control as ScriptEditor;
                if (scriptEditor != null)
                {
                    ScintillaWrapper scintilla = scriptEditor.ScriptEditorControl as ScintillaWrapper;
                    if (scintilla != null)
                    {
                        scintilla.ScriptFont = _settings.ScriptFont;
                        scintilla.ScriptFontSize = _settings.ScriptFontSize;
                        scintilla.CallTipFont = _settings.ScriptTipFont;
                        scintilla.CallTipFontSize = _settings.ScriptTipFontSize;
                        scintilla.UpdateAllStyles();
                    }
                }
            }
        }

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
            string selected_color_theme = _settings.ColorTheme;
            cmbColorTheme.DataSource = Factory.GUIController.ColorThemes.Themes;
            cmbColorTheme.SelectedIndex = Factory.GUIController.ColorThemes.Themes.ToList().FindIndex(t => t.Name == selected_color_theme);
            chkUsageInfo.Checked = _settings.SendAnonymousStats;
            udBackupInterval.Value = (_settings.BackupWarningInterval > 0) ? _settings.BackupWarningInterval : 1;
            chkBackupReminders.Checked = (_settings.BackupWarningInterval != 0);
            udBackupInterval.Enabled = chkBackupReminders.Checked;
            chkRemapBgImport.Checked = _settings.RemapPalettizedBackgrounds;
            chkKeepHelpOnTop.Checked = _settings.KeepHelpOnTop;
            chkPromptDialogOnTabsClose.Checked = _settings.DialogOnMultipleTabsClose;
            cmbScriptReloadOnExternalChange.SelectedIndex = (int)_settings.ReloadScriptOnExternalChange;

            txtAndJavaHomePath.Text = _settings.AndroidJavaHome;
            radAndJavaHomePath.Checked = (_settings.AndroidJavaHome != string.Empty);
            radAndJavaHomeEnv.Checked = (_settings.AndroidJavaHome == string.Empty);
            txtAndJavaHomePath.Enabled = radAndJavaHomePath.Checked;
            btnAndChooseJavaHomePath.Enabled = txtAndJavaHomePath.Enabled;

            txtAndAndroidHomePath.Text = _settings.AndroidHome;
            radAndAndroidHomePath.Checked = (_settings.AndroidHome != string.Empty);
            radAndAndroidHomeEnv.Checked = (_settings.AndroidHome == string.Empty);
            txtAndAndroidHomePath.Enabled = radAndAndroidHomePath.Checked;
            btnAndChooseAndroidHomePath.Enabled = txtAndAndroidHomePath.Enabled;

            textBoxAndKeystoreFile.Text = _settings.AndroidKeystoreFile;
            textBoxAndKeystorePassword.Text = _settings.AndroidKeystorePassword;
            textBoxAndKeystoreKeyAlias.Text = _settings.AndroidKeystoreKeyAlias;
            textBoxAndKeystoreKeyPassword.Text = _settings.AndroidKeystoreKeyPassword;

            androidHomeCheck();
            javaHomeCheck();
        }

        public PreferencesEditor()
        {
            InitializeComponent();

            UpdateAndroidKeystorePasswordState();

            _settings = Factory.AGSEditor.Settings.CloneAppSettings();
            // just in case they had it set to something silly in 2.72
            if (_settings.TabSize < udTabWidth.Minimum) _settings.TabSize = (int)udTabWidth.Minimum;
            if (_settings.TabSize > udTabWidth.Maximum) _settings.TabSize = (int)udTabWidth.Maximum;

            UpdateControlsForPreferences();

            propertyGridPreferences.SelectedObject = _settings;
            Utilities.CheckLabelWidthsOnForm(this);
        }

        private void ApplySettings()
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
            UpdateFontSettings();
        }

        private void btnApply_Click(object sender, EventArgs e)
        {
            ApplySettings();
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            ApplySettings();
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
            ColorTheme newColorTheme = (ColorTheme)cmbColorTheme.SelectedItem;

            if (newColorTheme == null) return;

            if (_settings.ColorTheme != newColorTheme.Name)
            {
                _settings.ColorTheme = newColorTheme.Name;
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

        private void javaHomeCheck()
        {
            labelJdkOk.Visible = AndroidUtilities.IsJdkFound(_settings.AndroidJavaHome);
        }

        private void androidHomeCheck()
        {
            labelSdkOk.Visible = AndroidUtilities.IsSdkFound(_settings.AndroidHome);
        }


        private void radAndJavaHomeEnv_CheckedChanged(object sender, EventArgs e)
        {
            radAndJavaHomePath_CheckedChanged(sender, e);
        }

        private void radAndJavaHomePath_CheckedChanged(object sender, EventArgs e)
        {
            txtAndJavaHomePath.Enabled = radAndJavaHomePath.Checked;
            btnAndChooseJavaHomePath.Enabled = radAndJavaHomePath.Checked;

            _settings.AndroidJavaHome = (radAndJavaHomeEnv.Checked ? string.Empty : txtAndJavaHomePath.Text);
            javaHomeCheck();
        }

        private void radAndAndroidHomeEnv_CheckedChanged(object sender, EventArgs e)
        {
            radAndAndroidHomePath_CheckedChanged(sender, e);
        }

        private void radAndAndroidHomePath_CheckedChanged(object sender, EventArgs e)
        {
            txtAndAndroidHomePath.Enabled = radAndAndroidHomePath.Checked;
            btnAndChooseAndroidHomePath.Enabled = radAndAndroidHomePath.Checked;

            _settings.AndroidHome = (radAndAndroidHomeEnv.Checked ? string.Empty : txtAndAndroidHomePath.Text);
            androidHomeCheck();
        }

        private void btnAndChooseJavaHomePath_Click(object sender, EventArgs e)
        {
            string selectedPath = Factory.GUIController.ShowSelectFolderOrDefaultDialog("JAVA_HOME: Please select the JDK you wish to use (usually in a 'jre/' dir).", txtAndJavaHomePath.Text);
            if (!string.IsNullOrEmpty(selectedPath))
            {
                if (System.IO.Directory.Exists(selectedPath))
                {
                    txtAndJavaHomePath.Text = selectedPath;
                    _settings.AndroidJavaHome = txtAndJavaHomePath.Text;
                    javaHomeCheck();
                    return;
                }
                else
                {
                    MessageBox.Show("The directory you have selected does not exist. Please select a valid path.", "Invalid directory", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }

            radAndJavaHomePath.Checked = !string.IsNullOrEmpty(_settings.AndroidJavaHome);
            radAndJavaHomeEnv.Checked = string.IsNullOrEmpty(_settings.AndroidJavaHome);
            txtAndJavaHomePath.Text = _settings.AndroidJavaHome;
            javaHomeCheck();
        }

        private void btnAndChooseAndroidHomePath_Click(object sender, EventArgs e)
        {
            string selectedPath = Factory.GUIController.ShowSelectFolderOrDefaultDialog("ANDROID_HOME: Please select the Android SDK you wish to use (usually in a 'Sdk/' dir).", txtAndAndroidHomePath.Text);
            if (!string.IsNullOrEmpty(selectedPath))
            {
                if (System.IO.Directory.Exists(selectedPath))
                {
                    txtAndAndroidHomePath.Text = selectedPath;
                    _settings.AndroidHome = txtAndAndroidHomePath.Text;
                    androidHomeCheck();
                    return;
                }
                else
                {
                    MessageBox.Show("The directory you have selected does not exist. Please select a valid path.", "Invalid directory", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }

            radAndAndroidHomePath.Checked = !string.IsNullOrEmpty(_settings.AndroidHome);
            radAndAndroidHomeEnv.Checked = string.IsNullOrEmpty(_settings.AndroidHome);
            txtAndAndroidHomePath.Text = _settings.AndroidHome;
            androidHomeCheck();
        }

        private void checkBoxAndroidShowPassword_CheckStateChanged(object sender, EventArgs e)
        {
            UpdateAndroidKeystorePasswordState();
        }

        private void textBoxAndKeystoreFile_Validated(object sender, EventArgs e)
        {
            _settings.AndroidKeystoreFile = textBoxAndKeystoreFile.Text;
        }

        private void textBoxAndKeystorePassword_Validated(object sender, EventArgs e)
        {
            _settings.AndroidKeystorePassword = textBoxAndKeystorePassword.Text;
        }

        private void textBoxAndKeystoreKeyAlias_Validated(object sender, EventArgs e)
        {
            _settings.AndroidKeystoreKeyAlias = textBoxAndKeystoreKeyAlias.Text;
        }

        private void textBoxAndKeystoreKeyPassword_Validated(object sender, EventArgs e)
        {
            _settings.AndroidKeystoreKeyPassword = textBoxAndKeystoreKeyPassword.Text;
        }
        private void buttonAndroidGenerateKeystore_Click(object sender, EventArgs e)
        {

            GenerateAndroidKeystore dialog = new GenerateAndroidKeystore();
            if (dialog.ShowDialog() == DialogResult.OK && dialog.Response != null)
            {
                GenerateAndroidKeystoreResponse r = dialog.Response;

                // returned from OK, keystore info was updated 
                textBoxAndKeystoreFile.Text = r.Keystore;
                textBoxAndKeystorePassword.Text = r.Password;
                textBoxAndKeystoreKeyAlias.Text = r.KeyAlias;
                textBoxAndKeystoreKeyPassword.Text = r.KeyPassword;

                _settings.AndroidKeystoreFile = textBoxAndKeystoreFile.Text;
                _settings.AndroidKeystorePassword = textBoxAndKeystorePassword.Text;
                _settings.AndroidKeystoreKeyAlias = textBoxAndKeystoreKeyAlias.Text;
                _settings.AndroidKeystoreKeyPassword = textBoxAndKeystoreKeyPassword.Text;
            }
            dialog.Dispose();
        }

        private void buttonAndroidChooseKeystore_Click(object sender, EventArgs e)
        {
            string initialDir = Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments);
            if (!string.IsNullOrEmpty(textBoxAndKeystoreFile.Text)) {
                string dir = Path.GetDirectoryName(textBoxAndKeystoreFile.Text);
                if (Directory.Exists(dir)) initialDir = dir;
            }

            OpenFileDialog dialog = new OpenFileDialog();
            dialog.Title = "Select an existing keystore.";
            dialog.RestoreDirectory = true;
            dialog.CheckFileExists = true;
            dialog.CheckPathExists = true;
            dialog.InitialDirectory = initialDir;
            dialog.ValidateNames = true;
            dialog.Filter = "Key store file (*.jks)|*.jks";

            string selectedFile = null;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                selectedFile = dialog.FileName;
            }
            dialog.Dispose();

            if (!string.IsNullOrEmpty(selectedFile))
            {
                if (System.IO.File.Exists(selectedFile))
                {
                    textBoxAndKeystoreFile.Text = selectedFile;
                    _settings.AndroidKeystoreFile = textBoxAndKeystoreFile.Text;
                    return;
                }
                else
                {
                    MessageBox.Show("The key store file selected does not exist. If you don't have a valid keystore, generate a new one.", "Invalid file", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            }

        }
    }
}