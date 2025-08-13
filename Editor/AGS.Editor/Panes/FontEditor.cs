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
            fontViewPanel.Invalidate();
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
                fontViewPanel.Invalidate();
            }
        }

        public ImportFont ImportOverFont { get; set; }

        public void OnFontUpdated(bool fontStyle = true, bool fontGlyphPosition = true)
        {
            Factory.GUIController.RefreshPropertyGrid();
            if (fontStyle)
                fontViewPanel.Invalidate();
            if (fontStyle || fontGlyphPosition)
                textPreviewPanel.Invalidate();
        }

        protected override string OnGetHelpKeyword()
        {
            return "Font Preview";
        }

        private void textPreviewPanel_Paint(object sender, PaintEventArgs e)
        {
            if (_item == null)
                return;

            if (textPreviewPanel.ClientSize.Width <= 0 || textPreviewPanel.ClientSize.Height <= 0)
                return; // sometimes occurs during automatic rearrangement of controls

            Graphics g = e.Graphics;
            int g_width = (int)e.Graphics.ClipBounds.Width;
            int g_height = (int)e.Graphics.ClipBounds.Height;
            bool hdcReleased = false;
            try
            {
                Factory.NativeProxy.DrawTextUsingFont(g.GetHdc(), tbTextPreview.Text, _item.ID,
                    0, 0, g_width, g_height, 5, 5, g_width - 5);
                g.ReleaseHdc();
                hdcReleased = true;
            }
            catch (Exception ex)
            {
                int a = 0;
            }
            finally
            {
                if (!hdcReleased)
                    g.ReleaseHdc();
            }
        }

        // TODO: reimplement this to e.g. only have a bitmap of the panel's size,
        // and draw a visible portion of the font preview.
        private void PaintFont(Graphics g)
        {
            if (_item == null)
                return;

            if (fontViewPanel.ClientSize.Width <= 0 || fontViewPanel.ClientSize.Height <= 0)
                return; // sometimes occurs during automatic rearrangement of controls

            int width = fontViewPanel.ClientSize.Width;
            int height = fontViewPanel.ClientSize.Height;
            int full_height = Factory.NativeProxy.DrawFont(IntPtr.Zero, _item.ID, 0, 0, width, 0, 0);
            if (full_height <= 0)
            {
                SetPreviewHeight(0);
                return; // something went wrong when calculating needed height
            }

            SetPreviewHeight(full_height);

            int scroll_y = -fontViewPanel.AutoScrollPosition.Y;
            bool hdcReleased = false;
            try
            {
                Factory.NativeProxy.DrawFont(g.GetHdc(), _item.ID, 0, 0, width, height, scroll_y);
                g.ReleaseHdc();
                hdcReleased = true;
            }
            catch (Exception)
            {
            }
            finally
            {
                if (!hdcReleased)
                    g.ReleaseHdc();
            }
        }

        private void SetPreviewHeight(int height)
        {
            _previewHeight = height;
            fontViewPanel.AutoScrollMinSize = new Size(0, height);
        }

        private void fontViewPanel_Paint(object sender, PaintEventArgs e)
        {
            PaintFont(e.Graphics);
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
            fontViewPanel.Invalidate();
        }

        private void imagePanel_Scroll(object sender, ScrollEventArgs e)
        {
            fontViewPanel.Invalidate();
        }

        private void tbTextPreview_TextChanged(object sender, EventArgs e)
        {
            textPreviewPanel.Invalidate();
        }

        private void splitContainer1_SplitterMoved(object sender, SplitterEventArgs e)
        {
            fontViewPanel.Invalidate();
            textPreviewPanel.Invalidate();
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
