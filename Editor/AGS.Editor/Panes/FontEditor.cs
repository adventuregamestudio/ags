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
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
        }

        public FontEditor(AGS.Types.Font selectedFont) : this()
        {
            _item = selectedFont;
            PaintFont();
        }

        private AGS.Types.Font _item;

        public AGS.Types.Font ItemToEdit
        {
            get { return _item; }
            set
            {
                _item = value;
                PaintFont();
            }
        }

        protected override string OnGetHelpKeyword()
        {
            return "Fonts";
        }

        private void PaintFont()
        {
            if (_item == null)
            {
                pictureBox.Image = null;
                return;
            }

            if (imagePanel.ClientSize.Width == 0)
                return; // sometimes occurs during automatic rearrangement of controls

            int height = Factory.NativeProxy.DrawFont(IntPtr.Zero, 0, 0, imagePanel.ClientSize.Width, _item.ID);
            Bitmap bmp = new Bitmap(imagePanel.ClientSize.Width, height);

            Graphics g = Graphics.FromImage(bmp);
            Factory.NativeProxy.DrawFont(g.GetHdc(), 0, 0, imagePanel.ClientSize.Width, _item.ID);
            g.ReleaseHdc();

            pictureBox.Image = bmp;
        }

        private void ImportTTFFont(string fileName, string newTTFName, string newWFNName)
        {
            int fontSize = NumberEntryDialog.Show("Font size", "Select the font size to import this TTF font at:", DEFAULT_IMPORTED_FONT_SIZE, 1);
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
                    _item.SourceFilename = Utilities.GetRelativeToProjectPath(fileName);
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

                if (fileName.ToLower().EndsWith(".wfn"))
                {
                    File.Copy(fileName, newWFNName, true);
                }
                else
                {
                    Factory.NativeProxy.ImportSCIFont(fileName, _item.ID);
                }

                _item.PointSize = 0;
                _item.SourceFilename = Utilities.GetRelativeToProjectPath(fileName);
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
                PaintFont();
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
                    string fileName = Factory.GUIController.ShowOpenFileDialog("Select font to import...", Constants.FONT_FILE_FILTER);
                    if (fileName != null)
                    {
                        ImportFont(fileName);
                    }
                }
            }
        }

        private void imagePanel_SizeChanged(object sender, EventArgs e)
        {
            PaintFont();
        }

        private void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("font-editor/background");
            ForeColor = t.GetColor("font-editor/foreground");
            currentItemGroupBox.BackColor = t.GetColor("font-editor/box/background");
            currentItemGroupBox.ForeColor = t.GetColor("font-editor/box/foreground");
            btnImportFont.BackColor = t.GetColor("font-editor/btn-import/background");
            btnImportFont.ForeColor = t.GetColor("font-editor/btn-import/foreground");
            btnImportFont.FlatStyle = (FlatStyle)t.GetInt("font-editor/btn-import/flat/style");
            btnImportFont.FlatAppearance.BorderSize = t.GetInt("font-editor/btn-import/flat/border/size");
            btnImportFont.FlatAppearance.BorderColor = t.GetColor("font-editor/btn-import/flat/border/color");
        }
    }
}
