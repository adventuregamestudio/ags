using AGS.Types;
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
    public partial class FontEditor : EditorContentPanel
    {
        private const int DEFAULT_IMPORTED_FONT_SIZE = 10;

        public FontEditor()
        {
            InitializeComponent();
        }

        public FontEditor(AGS.Types.Font selectedFont) : this()
        {
            _item = selectedFont;
        }

        private AGS.Types.Font _item;

        public AGS.Types.Font ItemToEdit
        {
            get { return _item; }
            set { _item = value; }
        }

        protected override string OnGetHelpKeyword()
        {
            return "Fonts";
        }

        private void imagePanel_Paint(object sender, PaintEventArgs e)
        {
            if (_item != null)
            {
                IntPtr hdc = e.Graphics.GetHdc();
                Factory.NativeProxy.DrawFont(hdc, 0, 0, _item.ID);
                e.Graphics.ReleaseHdc();
            }
        }

        private void ImportTTFFont(string fileName, string newTTFName, string newWFNName)
        {
            int fontSize = NumberEntryDialog.Show("Font size", "Select the font size to import this TTF font at:", DEFAULT_IMPORTED_FONT_SIZE);
            if (fontSize > 0)
            {
                File.Copy(fileName, newTTFName, true);
                try
                {
                    if (File.Exists(newWFNName))
                    {
                        Factory.AGSEditor.DeleteFileOnDiskAndSourceControl(newWFNName);
                    }
                    Factory.NativeProxy.ReloadTTFFont(_item.ID);
                    _item.PointSize = fontSize;
                    _item.SourceFilename = fileName;
                }
                catch (AGSEditorException ex)
                {
                    Factory.GUIController.ShowMessage("Unable to import the font.\n\n" + ex.Message, MessageBoxIcon.Warning);
                    File.Delete(newTTFName);
                }
            }
        }

        private void ImportWFNFont(string fileName, string newTTFName, string newWFNName)
        {
            try
            {
                if (File.Exists(newTTFName))
                {
                    Factory.AGSEditor.DeleteFileOnDiskAndSourceControl(newTTFName);
                }
                Factory.NativeProxy.ImportSCIFont(fileName, _item.ID);
                _item.PointSize = 0;
                _item.SourceFilename = fileName;
            }
            catch (AGSEditorException ex)
            {
                Factory.GUIController.ShowMessage("Unable to import the font.\n\n" + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private void ImportFont(string fileName)
        {
            try
            {
                string newTTFName = "agsfnt" + _item.ID + ".ttf";
                string newWFNName = "agsfnt" + _item.ID + ".wfn";

                List<string> filesToCheck = new List<string>();
                filesToCheck.Add(newTTFName);
                filesToCheck.Add(newWFNName);
                if (!Factory.AGSEditor.AttemptToGetWriteAccess(filesToCheck))
                {
                    return;
                }

                if (fileName.ToLower().EndsWith(".ttf"))
                {
                    ImportTTFFont(fileName, newTTFName, newWFNName);
                }
                else
                {
                    ImportWFNFont(fileName, newTTFName, newWFNName);
                }
                Factory.NativeProxy.GameSettingsChanged(Factory.AGSEditor.CurrentGame);
                Factory.GUIController.SetPropertyGridObject(_item);
                imagePanel.Invalidate();
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("There was a problem importing the font. The error was: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private void btnImportFont_Click(object sender, EventArgs e)
        {
            if (_item != null)
            {
                if (Factory.GUIController.ShowQuestion("Importing a font will replace the current font. Are you sure you want to do this?") == DialogResult.Yes)
                {
                    string fileName = Factory.GUIController.ShowOpenFileDialog("Select font to import...", "All supported fonts (*.ttf; font.*)|*.ttf;font.*|TrueType font files (*.ttf)|*.ttf|SCI font files (FONT.*)|font.*");
                    if (fileName != null)
                    {
                        ImportFont(fileName);
                    }
                }
            }
        }

    }
}
