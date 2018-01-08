using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class CursorEditor : EditorContentPanel
    {
        private int IMAGE_SCALE_FACTOR = 2;

        public CursorEditor()
        {
            InitializeComponent();
            Factory.GUIController.ColorThemes.Load(LoadColorTheme);
        }

        public CursorEditor(MouseCursor cursorToEdit) : this()
        {
            _item = cursorToEdit;
        }

        private MouseCursor _item;

        public MouseCursor ItemToEdit
        {
            get { return _item; }
            set { _item = value; }
        }

        protected override string OnGetHelpKeyword()
        {
            return "Cursors";
        }
        
        public void SaveData()
        {
        }

        public void RefreshData()
        {
        }

        private void imagePanel_Paint(object sender, PaintEventArgs e)
        {
            if (_item != null)
            {
                IMAGE_SCALE_FACTOR = Factory.AGSEditor.CurrentGame.GUIScaleFactor;

                if (_item.Image > 0)
				{
					IntPtr hdc = e.Graphics.GetHdc();
					Factory.NativeProxy.DrawSprite(hdc, 0, 0, _item.Image);
					e.Graphics.ReleaseHdc();
				}
                if ((_item.HotspotX >= 0) && (_item.HotspotY >= 0))
                {
                    e.Graphics.DrawLine(Pens.LightGreen, (_item.HotspotX - 2) * IMAGE_SCALE_FACTOR, _item.HotspotY * IMAGE_SCALE_FACTOR, (_item.HotspotX + 2) * IMAGE_SCALE_FACTOR, _item.HotspotY * IMAGE_SCALE_FACTOR);
                    e.Graphics.DrawLine(Pens.Blue, _item.HotspotX * IMAGE_SCALE_FACTOR, (_item.HotspotY - 2) * IMAGE_SCALE_FACTOR, _item.HotspotX * IMAGE_SCALE_FACTOR, (_item.HotspotY + 2) * IMAGE_SCALE_FACTOR);
                }
            }
        }

        private void imagePanel_MouseDown(object sender, MouseEventArgs e)
        {
            if (_item != null)
            {
                int spriteWidth, spriteHeight;
                Utilities.GetSizeSpriteWillBeRenderedInGame(_item.Image, out spriteWidth, out spriteHeight);

                IMAGE_SCALE_FACTOR = Factory.AGSEditor.CurrentGame.GUIScaleFactor;
                int newHotspotX = e.X / IMAGE_SCALE_FACTOR;
                int newHotspotY = e.Y / IMAGE_SCALE_FACTOR;
                if ((newHotspotX >= 0) && (newHotspotY >= 0) && 
                    (newHotspotX < spriteWidth) && (newHotspotY < spriteHeight))
                {
                    _item.HotspotX = newHotspotX;
                    _item.HotspotY = newHotspotY;
                    imagePanel.Invalidate();
                    Factory.GUIController.SetPropertyGridObject(_item);
                }
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("cursor-editor/background");
            ForeColor = t.GetColor("cursor-editor/foreground");
            currentItemGroupBox.BackColor = t.GetColor("cursor-editor/box/background");
            currentItemGroupBox.ForeColor = t.GetColor("cursor-editor/box/foreground");
        }
    }
}
