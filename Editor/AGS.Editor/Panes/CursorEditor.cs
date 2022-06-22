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
        private int _previousCursorImage;
        public CursorEditor()
        {
            InitializeComponent();
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
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

        protected override void OnPropertyChanged(string propertyName, object oldValue)
        {
            imagePanel.Invalidate(true);
        }

        protected override string OnGetHelpKeyword()
        {
            return "Cursor Editor";
        }
        
        public void SaveData()
        {
        }

        public void RefreshData()
        {
        }

        private int Scale(int value)
        {
            float scale = sldZoomLevel.ZoomScale;
            int scaled_value = (int)(value * scale);
            if (value != 0 && scaled_value == 0) scaled_value = 1;
            return scaled_value;
        }

        private int InverseScale(int value)
        {
            float scale = sldZoomLevel.ZoomScale;
            return (int)(value / scale);
        }

        void UpdatePanelSize()
        {
            if (_item != null)
            {
                Size cursorSpriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(_item.Image);
                cursorSpriteSize.Width = Scale(cursorSpriteSize.Width);
                cursorSpriteSize.Height = Scale(cursorSpriteSize.Height);

                imagePanel.AutoSize = false;
                imagePanel.MaximumSize = cursorSpriteSize;
                imagePanel.MinimumSize = cursorSpriteSize;
                imagePanel.Size = cursorSpriteSize;
                imagePanel.AutoScroll = true;
                _previousCursorImage = _item.Image;
            }
        }

        private void imagePanel_Paint(object sender, PaintEventArgs e)
        {
            if (_item != null)
            {
                if (_item.Image > 0)
                {
                    IntPtr hdc = e.Graphics.GetHdc();
                    if (_previousCursorImage != _item.Image) UpdatePanelSize();
                    Size spriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(_item.Image);

                    int scaled_width = (int)(sldZoomLevel.ZoomScale * (float)spriteSize.Width);
                    int scaled_height = (int)(sldZoomLevel.ZoomScale * (float)spriteSize.Height);

                    Factory.NativeProxy.DrawSprite(hdc, 0, 0, scaled_width, scaled_height, _item.Image);
                    e.Graphics.ReleaseHdc();
                }
                if ((_item.HotspotX >= 0) && (_item.HotspotY >= 0))
                {
                    Pen penGreen = new Pen(Color.LightGreen, 1);
                    Pen penBlue = new Pen(Color.Blue, 1);

                    // Create rectangle.
                    Rectangle rectH = new Rectangle(
                        x: Scale(_item.HotspotX - 1),
                        y: Scale(_item.HotspotY),
                        width: Scale(3),
                        height: Scale(1));

                    Rectangle rectV = new Rectangle(
                         x: Scale(_item.HotspotX),
                         y: Scale(_item.HotspotY - 1),
                         width: Scale(1),
                         height: Scale(3));

                    e.Graphics.DrawRectangle(penGreen, rectH);
                    e.Graphics.DrawRectangle(penBlue, rectV);
                }
            }
        }

        private void imagePanel_MouseDown(object sender, MouseEventArgs e)
        {
            if (_item != null)
            {
                int spriteWidth, spriteHeight;
                Utilities.GetSizeSpriteWillBeRenderedInGame(_item.Image, out spriteWidth, out spriteHeight);

                int newHotspotX = InverseScale(e.X);
                int newHotspotY = InverseScale(e.Y);
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

        private void sldZoomLevel_ValueChanged(object sender, EventArgs e)
        {
            UpdatePanelSize();
            imagePanel.Invalidate();
        }
    }
}
