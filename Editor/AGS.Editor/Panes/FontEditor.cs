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
        private bool _updatingCharCode = false;

        public FontEditor()
        {
            InitializeComponent();
            udCharCode.TextChanged += udCharCode_TextChanged;
            fontViewPanel.CharacterSelected += fontViewPanel_CharacterSelected;
        }

        public FontEditor(AGS.Types.Font selectedFont) : this()
        {
            _item = selectedFont;
            fontViewPanel.GameFontNumber = _item.ID;
            fontViewPanel.Scaling = Factory.AGSEditor.CurrentGame.GUIScaleFactor;
        }

        public delegate void ImportFont(AGS.Types.Font font);

        private AGS.Types.Font _item;

        public AGS.Types.Font ItemToEdit
        {
            get { return _item; }
            set
            {
                _item = value;
                fontViewPanel.GameFontNumber = _item.ID;
            }
        }

        public ImportFont ImportOverFont { get; set; }

        public void OnFontUpdated(bool fontStyle = true, bool fontGlyphPosition = true)
        {
            Factory.GUIController.RefreshPropertyGrid();

            UpdateCharInput();

            if (fontStyle)
                fontViewPanel.UpdateAndRepaint();
            if (fontStyle || fontGlyphPosition)
                textPreviewPanel.Invalidate();
        }

        public void UpdatePreviewScaling()
        {
            fontViewPanel.Scaling = Factory.AGSEditor.CurrentGame.GUIScaleFactor;
        }

        protected override string OnGetHelpKeyword()
        {
            return "Font Preview";
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

        private void textPreviewPanel_Paint(object sender, PaintEventArgs e)
        {
            if (_item == null)
                return;

            if (textPreviewPanel.ClientSize.Width <= 0 || textPreviewPanel.ClientSize.Height <= 0)
                return; // sometimes occurs during automatic rearrangement of controls

            Graphics g = e.Graphics;
            g.Clear(Color.Black);
            int scaling = Factory.AGSEditor.CurrentGame.GUIScaleFactor;
            int g_width = (int)e.Graphics.ClipBounds.Width;
            int g_height = (int)e.Graphics.ClipBounds.Height;
            bool hdcReleased = false;
            try
            {
                Factory.NativeProxy.DrawTextUsingFont(g.GetHdc(), tbTextPreview.Text, _item.ID,
                    0, 0, g_width, g_height, 5, 5, g_width / scaling - 5,
                    scaling);
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

        private void tbTextPreview_TextChanged(object sender, EventArgs e)
        {
            textPreviewPanel.Invalidate();
        }

        private void textPreviewPanel_SizeChanged(object sender, EventArgs e)
        {
            textPreviewPanel.Invalidate();
        }

        private void rbUnicode_CheckedChanged(object sender, EventArgs e)
        {
            fontViewPanel.ANSIMode = false;
            lblCharCode.Text = "Code: U+";
            udCharCode.Hexadecimal = true;
        }

        private void rbANSI_CheckedChanged(object sender, EventArgs e)
        {
            fontViewPanel.ANSIMode = true;
            lblCharCode.Text = "Code:";
            udCharCode.Hexadecimal = false;
        }

        private void fontViewPanel_CharacterSelected(object sender, FontPreviewGrid.CharacterSelectedEventArgs args)
        {
            udCharCode.Value = args.CharacterCode;
        }

        private void UpdateCharCode()
        {
            if (!_updatingCharCode)
            {
                _updatingCharCode = true;
                int code = 0;
                if (tbCharInput.Text.Length > 0)
                {
                    if (fontViewPanel.ANSIMode)
                    {
                        var ansiBytes = Encoding.Default.GetBytes(tbCharInput.Text);
                        code = ansiBytes[0];
                    }
                    else
                    {
                        code = tbCharInput.Text[0];
                    }
                }
                udCharCode.Value = (code >= udCharCode.Minimum && code <= udCharCode.Maximum) ? code : 0;

                // Automatically scroll the preview to the selected character
                fontViewPanel.SelectedCharCode = (int)udCharCode.Value;

                _updatingCharCode = false;
            }
        }

        private void UpdateCharInput()
        {
            if (!_updatingCharCode)
            {
                _updatingCharCode = true;
                tbCharInput.Text = ((char)(udCharCode.Value)).ToString();

                // Automatically scroll the preview to the selected character
                fontViewPanel.SelectedCharCode = (int)udCharCode.Value;

                _updatingCharCode = false;
            }
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

        private void udCharCode_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                fontViewPanel.SelectedCharCode = (int)udCharCode.Value;
                e.Handled = e.SuppressKeyPress = true;
            }
        }

        private void tbCharInput_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
            {
                fontViewPanel.SelectedCharCode = (int)udCharCode.Value;
                e.Handled = e.SuppressKeyPress = true;
            }
        }

        private void btnGotoChar_Click(object sender, EventArgs e)
        {
            fontViewPanel.SelectedCharCode = (int)udCharCode.Value;
        }

        private void chkDisplayCodes_CheckedChanged(object sender, EventArgs e)
        {
            fontViewPanel.DisplayCodes = chkDisplayCodes.Checked;
        }

        private void splitContainer1_SplitterMoved(object sender, SplitterEventArgs e)
        {
            fontViewPanel.Invalidate();
            textPreviewPanel.Invalidate();
        }

        private void splitContainer1_Resize(object sender, EventArgs e)
        {
            // HACK: Fix the size of the preview panel.
            // For some reason, when done automatically, the preview panel's bottom gets too far down.
            fontViewPanel.SetBounds(fontViewPanel.Left, fontViewPanel.Top, fontViewPanel.Width, splitContainer1.Panel2.Height - splitContainer1.Panel2.Padding.Bottom - fontViewPanel.Top);
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "font-editor");
            t.GroupBoxHelper(currentItemGroupBox, "font-editor/box");
            t.ButtonHelper(btnImportFont, "font-editor/btn-import");
            t.ButtonHelper(btnGotoChar, "font-editor/btn-gotochar");
            t.TextBoxHelper(tbTextPreview, "font-editor/text-box-preview");
            // FIXME: color theme is not applied to the "character code" and "character" textboxes atm,
            // because there's no implementation for "up-down-control", so no way to color them consistently.
        }

        private void FontEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);

                if (Factory.AGSEditor.CurrentGame.UnicodeMode)
                    rbUnicode.Checked = true;
                else
                    rbANSI.Checked = false;
            }
        }
    }
}
