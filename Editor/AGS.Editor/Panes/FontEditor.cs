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
            udCharCode.TextChanged += udCharCode_TextChanged;
        }

        public FontEditor(AGS.Types.Font selectedFont) : this()
        {
            _item = selectedFont;
            fontViewPanel.Invalidate();
        }

        public delegate void ImportFont(AGS.Types.Font font);

        private AGS.Types.Font _item;

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

            UpdateCharInput();
            PrecalculatePreviewGrid();

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
                    0, 0, g_width, g_height, 5, 5, g_width - 5,
                    Factory.AGSEditor.CurrentGame.GUIScaleFactor);
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

        struct PreviewGrid
        {
            public int GridWidth; // visible grid width
            public int GridHeight; // visible grid height
            public int Padding;
            public int CellWidth;
            public int CellHeight;
            public int CellSpaceX;
            public int CellSpaceY;
            public int CharsPerRow;

            public int PreviewHeight; // a virtual full preview height
        };

        PreviewGrid _previewGrid = new PreviewGrid();

        private void PrecalculatePreviewGrid()
        {
            if (_item == null)
                return;

            if (fontViewPanel.ClientSize.Width <= 0 || fontViewPanel.ClientSize.Height <= 0)
                return; // sometimes occurs during automatic rearrangement of controls

            int scaling = Factory.AGSEditor.CurrentGame.GUIScaleFactor;
            int width = fontViewPanel.ClientSize.Width;
            int height = fontViewPanel.ClientSize.Height;
            int grid_width = width / scaling;
            int grid_height = height / scaling;
            var fontMetrics = Factory.NativeProxy.GetFontMetrics(_item.ID);
            Rectangle bbox = fontMetrics.CharBBox;
            int padding = 5;
            int cell_w = Math.Max(10, bbox.Width);
            int cell_h = Math.Max(10, bbox.Height);
            int cell_space_x = Math.Max(4, cell_w / 4);
            int cell_space_y = Math.Max(4, cell_h / 4);

            // Precalculate full height
            int chars_per_row = Math.Max(1, (grid_width - (padding * 2)) / (cell_w + cell_space_x));
            int first_char = 0; // we draw starting with char 0 always, just as a convention
            int last_char = fontMetrics.LastCharCode;
            int char_count = (last_char - first_char + 1);
            int full_height = (char_count / chars_per_row + 1) * (cell_h + cell_space_y);

            _previewGrid = new PreviewGrid();
            _previewGrid.GridWidth = grid_width;
            _previewGrid.GridHeight = grid_height;
            _previewGrid.Padding = padding;
            _previewGrid.CellWidth = cell_w;
            _previewGrid.CellHeight = cell_h;
            _previewGrid.CellSpaceX = cell_space_x;
            _previewGrid.CellSpaceY = cell_space_y;
            _previewGrid.CharsPerRow = chars_per_row;
            _previewGrid.PreviewHeight = full_height;

            SetPreviewHeight(full_height);
        }

        private void PaintFont(Graphics g)
        {
            if (_item == null)
                return;

            if (fontViewPanel.ClientSize.Width <= 0 || fontViewPanel.ClientSize.Height <= 0)
                return; // sometimes occurs during automatic rearrangement of controls

            int width = fontViewPanel.ClientSize.Width;
            int height = fontViewPanel.ClientSize.Height;
            int scaling = Factory.AGSEditor.CurrentGame.GUIScaleFactor;

            int scroll_y = -fontViewPanel.AutoScrollPosition.Y;
            bool hdcReleased = false;
            try
            {
                Factory.NativeProxy.DrawFont(g.GetHdc(), _item.ID, 0, 0, width, height,
                    _previewGrid.Padding, _previewGrid.CellWidth, _previewGrid.CellHeight,
                    _previewGrid.CellSpaceX, _previewGrid.CellSpaceY, scaling, scroll_y);
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

        // Sets a virtual preview height (the virtual size of the contents within the font preview)
        private void SetPreviewHeight(int height)
        {
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

        private void fontViewPanel_SizeChanged(object sender, EventArgs e)
        {
            PrecalculatePreviewGrid();
            fontViewPanel.Invalidate();
        }

        private void fontViewPanel_Scroll(object sender, ScrollEventArgs e)
        {
            fontViewPanel.Invalidate();
        }

        private void tbTextPreview_TextChanged(object sender, EventArgs e)
        {
            textPreviewPanel.Invalidate();
        }

        private void UpdateCharCode()
        {
            int code = 0;
            if (tbCharInput.Text.Length > 0)
                code = tbCharInput.Text[0];
            udCharCode.Value = (code >= udCharCode.Minimum && code <= udCharCode.Maximum) ? code : 0;
        }

        private void UpdateCharInput()
        {
            tbCharInput.Text = ((char)(udCharCode.Value)).ToString();
        }

        private void udCharCode_TextChanged(object sender, EventArgs e)
        {
            UpdateCharInput();
        }

        private void udCharCode_ValueChanged(object sender, EventArgs e)
        {
            UpdateCharInput();
        }

        private void tbCharInput_TextChanged(object sender, EventArgs e)
        {
            UpdateCharCode();
        }

        private void GotoChar()
        {
            if (_item == null)
                return;

            if (_previewGrid.PreviewHeight == 0)
            {
                PrecalculatePreviewGrid();
            }

            // Try to calculate the necessary scroll Y which lets us see the wanted character
            int code = (int)udCharCode.Value;
            int row = _previewGrid.CharsPerRow > 0 ? (code / _previewGrid.CharsPerRow) : 0;
            int y = row * (_previewGrid.CellHeight + _previewGrid.CellSpaceY) /*+ _previewGrid.Padding*/;

            fontViewPanel.AutoScrollPosition = new Point(0, y);
            fontViewPanel.Invalidate();
        }

        private void udCharCode_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                GotoChar();
                e.Handled = true;
            }
        }

        private void tbCharInput_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                GotoChar();
                e.Handled = true;
            }
        }

        private void btnGotoChar_Click(object sender, EventArgs e)
        {
            GotoChar();
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
