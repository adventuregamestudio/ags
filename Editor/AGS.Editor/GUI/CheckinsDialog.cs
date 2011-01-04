using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class CheckinsDialog : Form
    {
        private string[] _files = null;

        private static Dictionary<string, string> _iconsForFileExtensions = null;

        public CheckinsDialog(string dialogTitle, string okButtonText, string[] filesToCheckOut)
        {
            InitializeComponent();

            this.Text = dialogTitle;
            btnCheckin.Text = okButtonText;
            _files = filesToCheckOut;

			EnsureFileExtensionsAreInitialized();
        }

		private static void EnsureFileExtensionsAreInitialized()
		{
			if (_iconsForFileExtensions == null)
			{
				_iconsForFileExtensions = new Dictionary<string, string>();
				_iconsForFileExtensions.Add(".asc", "ScriptIcon");
				_iconsForFileExtensions.Add(".ash", "ScriptIcon");
				_iconsForFileExtensions.Add(".crm", "RoomIcon");
				_iconsForFileExtensions.Add(".agf", "GameIcon");
				_iconsForFileExtensions.Add(".trs", "TranslationIcon");
				_iconsForFileExtensions.Add(".wfn", "FontIcon");
				_iconsForFileExtensions.Add(".ttf", "FontIcon");
                _iconsForFileExtensions.Add(".spr", "SpriteManagerIcon");
			}
		}

		public static void RegisterFileIcon(string extension, string iconKey)
		{
			if (!extension.StartsWith("."))
			{
				throw new ArgumentException("File extension must be in format:  .ext", "extension");
			}

			EnsureFileExtensionsAreInitialized();

			extension = extension.ToLower();

			if (_iconsForFileExtensions.ContainsKey(extension))
			{
				_iconsForFileExtensions.Add(extension, iconKey);
			}
			else
			{
				_iconsForFileExtensions[extension] = iconKey;
			}
		}

        public string[] SelectedFiles
        {
            get { return _files; }
        }

        public string Comments
        {
            get { return txtComments.Text; }
        }

        private void PendingCheckinsDialog_Load(object sender, EventArgs e)
        {
            fileList.ImageList = Factory.GUIController.ImageList;
            fileList.Nodes.Clear();
            string rootPath = Directory.GetCurrentDirectory().ToLower();

            for (int i = 0; i < _files.Length; i++)
            {
				string extension = Path.GetExtension(_files[i]).ToLower();
				string iconKey = null;
				string shortFileName = _files[i];

				if (_iconsForFileExtensions.ContainsKey(extension))
				{
					iconKey = _iconsForFileExtensions[extension];
				}

				if (!_files[i].ToLower().StartsWith(rootPath))
				{
					Factory.GUIController.ShowMessage("Invalid file name '" + _files[i] + "' (all files in source control file list must include full paths)", MessageBoxIcon.Warning);
				}
				else
				{
					shortFileName = _files[i].Substring(rootPath.Length + 1);
				}
                TreeNode newNode = fileList.Nodes.Add(i.ToString(), shortFileName, iconKey, iconKey);
                newNode.Checked = true;
            }

            if (fileList.Nodes.Count == 0)
            {
                txtComments.Enabled = false;
                btnCheckin.Enabled = false;
            }
        }

        private void btnCheckin_Click(object sender, EventArgs e)
        {
            List<string> filesToProcess = new List<string>();
            foreach (TreeNode node in fileList.Nodes)
            {
                if (node.Checked)
                {
                    int index = Convert.ToInt32(node.Name);
                    filesToProcess.Add(_files[index]);
                }
            }
            if (filesToProcess.Count > 0)
            {
                _files = filesToProcess.ToArray();
            }
            else
            {
                _files = null;
            }

            this.Close();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            _files = null;
            this.Close();
        }
    }
}