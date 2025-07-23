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

        private void UpgradeGameIntroPage_Load(object sender, EventArgs e)
        {
            _backupPath = Path.Combine(Game.DirectoryPath,
                Utilities.MakeUniqueDirectory(Game.DirectoryPath, "Backup"));
            lblBackupLocation.Text = "Backup files will be placed in directory:" + Environment.NewLine + _backupPath;

            richDescription.Select(331, 470 - 331);
            richDescription.SelectionFont = new System.Drawing.Font(richDescription.SelectionFont, System.Drawing.FontStyle.Bold);
            richDescription.DeselectAll();
        }

        private void chkBackup_CheckedChanged(object sender, EventArgs e)
        {
            lblBackupLocation.Enabled = chkBackup.Checked;
        }
    }
}
