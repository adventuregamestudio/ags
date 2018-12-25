using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public partial class SpriteExportDialog : Form
    {
        public string ExportPath
        {
            get { return Path.Combine(txtFolder.Text, txtFilename.Text); }
        }

        public bool Recurse
        {
            get { return chkRecurse.Checked; }
        }

        public bool UseRootFolder
        {
            get { return radRootFolder.Checked; }
        }

        public bool SkipValidSpriteSource
        {
            get { return chkSkipValidSpriteSource.Checked; }
        }

        public bool UpdateSpriteSource
        {
            get { return chkUpdateSpriteSource.Checked; }
        }

        public SpriteExportDialog(SpriteFolder folder)
        {
            InitializeComponent();
            radThisFolder.Text = String.Format("This sprite folder ({0})", folder.Name);

            if (folder == Factory.AGSEditor.CurrentGame.RootSpriteFolder)
            {
                radRootFolder.Checked = true;
                radThisFolder.Enabled = false;
            }
        }

        private void btnBrowse_Click(object sender, EventArgs e)
        {
            string folderPath = Factory.GUIController.ShowSelectFolderOrNoneDialog("Export sprites to folder...", Directory.GetCurrentDirectory());

            if (folderPath != null)
            {
                txtFolder.Text = folderPath;
            }
        }
    }
}
