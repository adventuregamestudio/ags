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
            imagePanel.Invalidate();
        }

        public delegate void ImportFont(AGS.Types.Font font);

        private AGS.Types.Font _item;
        private int _previewHeight;

        public AGS.Types.Font ItemToEdit
        {
            get { return _item; }
            set
            {
                _item = value;
                imagePanel.Invalidate();
            }
        }

        public ImportFont ImportOverFont { get; set; }

        public void OnFontUpdated()
        {
            Factory.GUIController.RefreshPropertyGrid();
            imagePanel.Invalidate();
        }

        protected override string OnGetHelpKeyword()
        {
            return "Font Preview";
        }

        // TODO: reimplement this to e.g. only have a bitmap of the panel's size,
        // and draw a visible portion of the font preview.
        private void PaintFont(Graphics g)
        {
            if (_item == null)
                return;

            if (imagePanel.ClientSize.Width <= 0 || imagePanel.ClientSize.Height <= 0)
                return; // sometimes occurs during automatic rearrangement of controls

            int width = imagePanel.ClientSize.Width;
            int height = imagePanel.ClientSize.Height;
            int full_height = Factory.NativeProxy.DrawFont(IntPtr.Zero, _item.ID, 0, 0, width, 0, 0);
            if (full_height <= 0)
            {
                SetPreviewHeight(0);
                return; // something went wrong when calculating needed height
            }

            SetPreviewHeight(full_height);

            int scroll_y = -imagePanel.AutoScrollPosition.Y;
            try
            {
                Factory.NativeProxy.DrawFont(g.GetHdc(), _item.ID, 0, 0, width, height, scroll_y);
                g.ReleaseHdc();
            }
            catch (Exception)
            {
            }
        }

        private void SetPreviewHeight(int height)
        {
            _previewHeight = height;
            imagePanel.AutoScrollMinSize = new Size(0, height);
        }

        private void imagePanel_Paint(object sender, PaintEventArgs e)
        {
            PaintFont(e.Graphics);
        }

        private void imagePanel_SizeChanged(object sender, EventArgs e)
        {
            imagePanel.Invalidate();
        }

        private void imagePanel_Scroll(object sender, ScrollEventArgs e)
        {
            imagePanel.Invalidate();
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "font-editor");
            t.GroupBoxHelper(currentItemGroupBox, "font-editor/box");
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
