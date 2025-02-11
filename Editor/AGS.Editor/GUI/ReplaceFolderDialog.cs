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

namespace AGS.Editor
{
    public partial class ReplaceFolderDialog : Form
    {
        private string _makeRelativeToThisDir;

        public ReplaceFolderDialog()
        {
            InitializeComponent();
        }

        private ReplaceFolderDialog(string titleBar, string headerText, string oldPath, string newPath, string makeRelativeToThisDir = null)
        {
            InitializeComponent();
            Text = titleBar;
            lblOperationDescription.Text = headerText;
            _makeRelativeToThisDir = makeRelativeToThisDir;
            OldPath = oldPath;
            NewPath = newPath;
        }

        public string OldPath
        {
            get { return tbOldPath.Text; }
            set
            {
                string path = value;
                if (!string.IsNullOrEmpty(_makeRelativeToThisDir))
                    path = Utilities.GetRelativeToBasePath(path, _makeRelativeToThisDir);
                tbOldPath.Text = path;
            }
        }

        public string NewPath
        {
            get { return tbNewPath.Text; }
            set
            {
                string path = value;
                if (!string.IsNullOrEmpty(_makeRelativeToThisDir))
                    path = Utilities.GetRelativeToBasePath(path, _makeRelativeToThisDir);
                tbNewPath.Text = path;
            }
        }

        public static Tuple<string, string> Show(string titleBar, string headerText, string oldPath, string newPath, string makeRelativeToThisDir = null)
        {
            ReplaceFolderDialog dialog = new ReplaceFolderDialog(titleBar, headerText, oldPath, newPath, makeRelativeToThisDir);
            Tuple<string, string> result = null;
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                result = new Tuple<string, string>(dialog.OldPath, dialog.NewPath);
            }
            dialog.Dispose();
            return result;
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
            Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }

        private void btnBrowseOldPath_Click(object sender, EventArgs e)
        {
            OldPath = Factory.GUIController.ShowSelectFolderOrDefaultDialog("Select folder", tbOldPath.Text);
        }

        private void btnBrowseNewPath_Click(object sender, EventArgs e)
        {
            NewPath = Factory.GUIController.ShowSelectFolderOrDefaultDialog("Select folder", tbNewPath.Text);
        }
    }
}
