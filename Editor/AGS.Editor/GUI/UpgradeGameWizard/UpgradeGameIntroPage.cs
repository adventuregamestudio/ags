using System;
using System.IO;
using AGS.Types;

namespace AGS.Editor
{
    public partial class UpgradeGameIntroPage : UpgradeGameWizardPage
    {
        private UpgradeGameIntroAndBackupTask _introTask;
        private string _backupPath;

        public UpgradeGameIntroPage()
        {
            InitializeComponent();
        }

        public UpgradeGameIntroPage(Game game, UpgradeGameIntroAndBackupTask task)
            : base(game, task)
        {
            _introTask = task;
            InitializeComponent();
        }

        public override string TitleText
        {
            get { return "Project backup options"; }
        }

        public override bool NextButtonPressed()
        {
            _introTask.Enabled = chkBackup.Checked;
            _introTask.BackupPath = _backupPath;
            return true;
        }

        private void FormatIntroText()
        {
            // TODO: have this string in the form resources?
            // TODO: find out why does RichTextBox loose the styling when the page switches back and forth
            richDescription.Rtf =
@"{\rtf1\ansi
There have been a number of significant changes to the game format since the version of AGS in which you worked on your game last time. Because of that, the upgrade process will apply modifications to your game which will either be impossible to revert, may require additional manual fixes, or otherwise demand your attention.\par
\par
\b We STRONGLY RECOMMEND that you make a backup copy in case anything goes wrong with the upgrade, or if you don't like the resulting changes.\b0\par
\par
You may do the backup yourself, or let the AGS Editor to create a copy of your essential game files in the following directory as shown below.\par
\par
Note that this wizard will only backup standard game files that may in theory be modified by this update. All other files that are not going to be changed won't be copied.\par
}";
        }

        private void UpgradeGameIntroPage_Load(object sender, EventArgs e)
        {
            _backupPath = Path.Combine(Game.DirectoryPath,
                Utilities.MakeUniqueDirectory(Game.DirectoryPath, "Backup"));
            lblBackupLocation.Text = "Backup files will be placed in directory:" + Environment.NewLine + _backupPath;
            FormatIntroText();
        }

        private void chkBackup_CheckedChanged(object sender, EventArgs e)
        {
            lblBackupLocation.Enabled = chkBackup.Checked;
        }

        private void UpgradeGameIntroPage_VisibleChanged(object sender, EventArgs e)
        {
            if (Visible && !DesignMode)
            {
                FormatIntroText();
            }
        }
    }
}
