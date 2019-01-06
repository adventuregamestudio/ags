using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public partial class SpriteExportDialog : Form
    {
        private TextBox textbox; 

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

            List<ToolStripMenuItem> tokens = new List<ToolStripMenuItem>();

            foreach(PropertyInfo property in typeof(Sprite).GetProperties())
            {
                tokens.Add(new ToolStripMenuItem(String.Format("%{0}%", property.Name)));
            }

            menuItemToken.DropDownItems.AddRange(tokens.ToArray());
        }

        private void btnBrowse_Click(object sender, EventArgs e)
        {
            string folderPath = Factory.GUIController.ShowSelectFolderOrNoneDialog("Export sprites to folder...", Directory.GetCurrentDirectory());

            if (folderPath != null)
            {
                txtFolder.Text = folderPath;
            }
        }

        private void contextMenuStripExport_Opening(object sender, CancelEventArgs e)
        {
            // track the SourceControl since behaviour varies for sub-menu items
            // depending on which .NET Framework version is being used
            textbox = contextMenuStripExport.SourceControl as TextBox;

            if (textbox != null)
            {
                menuItemCopy.Enabled =
                    menuItemCut.Enabled = textbox.SelectionLength > 0;

                menuItemPaste.Enabled = Clipboard.ContainsText();
            }
        }

        private void menuItemCopy_Click(object sender, EventArgs e)
        {
            if (textbox != null)
            {
                textbox.Copy();
            }
        }

        private void menuItemCut_Click(object sender, EventArgs e)
        {
            if (textbox != null)
            {
                textbox.Cut();
            }
        }

        private void menuItemPaste_Click(object sender, EventArgs e)
        {
            if (textbox != null)
            {
                textbox.Paste();
            }
        }

        private void menuItemToken_DropDownItemClicked(object sender, ToolStripItemClickedEventArgs e)
        {
            if (textbox != null)
            {
                int index = textbox.SelectionStart;

                if (textbox.SelectionLength != 0)
                {
                    textbox.Text = textbox.Text.Remove(index, textbox.SelectionLength);
                }

                textbox.Text = textbox.Text.Insert(index, e.ClickedItem.Text);
                textbox.SelectionStart = index + e.ClickedItem.Text.Length;
            }
        }
    }
}
