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
    public partial class ImportGameWizardPage : WizardPage
    {
        public ImportGameWizardPage(string backupLocation)
        {
            InitializeComponent();
            this.lblBackupLocation.Text = "Backup files will be placed in directory:" + Environment.NewLine + backupLocation;
        }

        public bool BackupEnabled
        {
            get
            {
                return chkBackup.Checked;
            }
        }

        public override string TitleText
        {
            get
            {
                return "File backup options";
            }
        }

    }
}
