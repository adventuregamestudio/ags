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
        public FontEditor()
        {
            InitializeComponent();
        }

        public FontEditor(AGS.Types.Font selectedFont) : this()
        {
            _item = selectedFont;
            PaintFont();
        }

        public delegate void ImportFont(AGS.Types.Font font);

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

        public ImportFont ImportOverFont { get; set; }

        public void OnFontUpdated()
        {
            Factory.GUIController.RefreshPropertyGrid();
            PaintFont();
        }

        protected override string OnGetHelpKeyword()
        {
            return "Font Preview";
        }

        private void PaintFont()
        {
            if (_item == null)
            {
                pictureBox.Image = null;
                return;
            }

            if (imagePanel.ClientSize.Width <= 0)
                return; // sometimes occurs during automatic rearrangement of controls

            int height = Factory.NativeProxy.DrawFont(IntPtr.Zero, 0, 0, imagePanel.ClientSize.Width, _item.ID);
            if (height <= 0)
                return; // something went wrong when calculating needed height

            Bitmap bmp = new Bitmap(imagePanel.ClientSize.Width, height);
            Graphics g = Graphics.FromImage(bmp);
            Factory.NativeProxy.DrawFont(g.GetHdc(), 0, 0, imagePanel.ClientSize.Width, _item.ID);
            g.ReleaseHdc();

            pictureBox.Image = bmp;
        }


        private void btnImportFont_Click(object sender, EventArgs e)
        {
            if (_item != null && ImportOverFont != null)
            {
                if (Factory.GUIController.ShowQuestion("Importing a font will replace the current font. Are you sure you want to do this?") == DialogResult.Yes)
                {
                    ImportOverFont(_item);
                    OnFontUpdated();
                }
            }
        }

        private void imagePanel_SizeChanged(object sender, EventArgs e)
        {
            PaintFont();
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "font-editor");
            t.GroupBoxHelper(currentItemGroupBox, "font-editor/box");
            t.ButtonHelper(btnImportFont, "font-editor/btn-import");
        }

        private void FontEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
