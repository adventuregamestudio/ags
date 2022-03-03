using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public partial class InventoryEditor : EditorContentPanel
    {
        private InventoryItem _item;
        private int _previousItemCursorImage;
        private int _previousItemImage;

        void UpdatePanelSizes()
        {
            if (_item != null)
            {
                Size itemSpriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(_item.Image);
                Size itemCursorSpriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(_item.CursorImage);
                itemSpriteSize.Width = Scale(itemSpriteSize.Width);
                itemSpriteSize.Height = Scale(itemSpriteSize.Height);
                itemCursorSpriteSize.Width = Scale(itemCursorSpriteSize.Width);
                itemCursorSpriteSize.Height = Scale(itemCursorSpriteSize.Height);

                pnlInvWindowImage.AutoSize = false;
                pnlInvWindowImage.MaximumSize = itemSpriteSize;
                pnlInvWindowImage.MinimumSize = itemSpriteSize;
                pnlInvWindowImage.Size = itemSpriteSize;
                panelScrollAreaCursor.AutoScroll = true;
                pnlCursorImage.AutoSize = false;
                pnlCursorImage.MaximumSize = itemCursorSpriteSize;
                pnlCursorImage.MinimumSize = itemCursorSpriteSize;
                pnlCursorImage.Size = itemCursorSpriteSize;
                panelScrollAreaImage.AutoScroll = true;

                _previousItemCursorImage = _item.CursorImage;
                _previousItemImage = _item.Image;
            }
        }

        public InventoryEditor()
        {
            InitializeComponent();
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            UpdatePanelSizes();
        }

        public InventoryEditor(InventoryItem itemToEdit) : this()
        {
            ItemToEdit = itemToEdit;
        }

        protected override string OnGetHelpKeyword()
        {
            return "Inventory Items Editor";
        }

        public InventoryItem ItemToEdit
        {
            get { return _item; }
            set { _item = value; UpdateControlsEnabled(); }
        }

        public void SaveData()
        {
        }

        public void RefreshData()
        {
            UpdateControlsEnabled();
        }

        private void UpdateControlsEnabled()
        {
            currentItemGroupBox.Visible = (_item != null);
            UpdatePanelSizes();
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

        private void pnlCursorImage_Paint(object sender, PaintEventArgs e)
        {
            if (_item != null)
            {
                
                IntPtr hdc = e.Graphics.GetHdc();
                if (_previousItemCursorImage != _item.CursorImage) UpdatePanelSizes();
                Size spriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(_item.CursorImage);
                Factory.NativeProxy.DrawSprite(hdc, 0, 0,
                    Scale(spriteSize.Width), Scale(spriteSize.Height),
                    _item.CursorImage);
                e.Graphics.ReleaseHdc();
                if ((_item.HotspotX > -1) && (_item.HotspotY > -1))
                {
                    Pen penGreen = new Pen(Color.LightGreen, 1);
                    Pen penBlue = new Pen(Color.Blue, 1);

                    // Create rectangle.
                    Rectangle rectH = new Rectangle(
                        x: Scale(_item.HotspotX - 1),
                        y: Scale(_item.HotspotY ),
                        width: Scale(3),
                        height: Scale(1));

                    Rectangle rectV = new Rectangle(
                         x: Scale(_item.HotspotX ),
                         y: Scale(_item.HotspotY - 1),
                         width: Scale(1),
                         height: Scale(3));

                    e.Graphics.DrawRectangle(penGreen, rectH);
                    e.Graphics.DrawRectangle(penBlue, rectV);
                }
            }
        }

        private void pnlCursorImage_MouseDown(object sender, MouseEventArgs e)
        {
            if (_item != null)
            {
                int spriteWidth, spriteHeight;
                Utilities.GetSizeSpriteWillBeRenderedInGame(_item.CursorImage, out spriteWidth, out spriteHeight);

                int newHotspotX = InverseScale(e.X);
                int newHotspotY = InverseScale(e.Y);
                if ((newHotspotX > -1) && (newHotspotY > -1) &&
                    (newHotspotX < spriteWidth) && (newHotspotY < spriteHeight))
                {
                    _item.HotspotX = newHotspotX;
                    _item.HotspotY = newHotspotY;
                    pnlCursorImage.Invalidate();
                    Factory.GUIController.SetPropertyGridObject(_item);
                }
            }
        }

        private void pnlInvWindowImage_Paint(object sender, PaintEventArgs e)
        {
            if (_item != null)
            {
                IntPtr hdc = e.Graphics.GetHdc();
                if (_previousItemImage != _item.Image) UpdatePanelSizes();
                Size spriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(_item.Image);
                Factory.NativeProxy.DrawSprite(hdc, 0, 0,
                    Scale(spriteSize.Width),
                    Scale(spriteSize.Height),
                    _item.Image);
                e.Graphics.ReleaseHdc();
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("inventory-editor/background");
            ForeColor = t.GetColor("inventory-editor/foreground");
            currentItemGroupBox.BackColor = t.GetColor("inventory-editor/current-item-box/background");
            currentItemGroupBox.ForeColor = t.GetColor("inventory-editor/current-item-box/foreground");
            groupBox1.BackColor = t.GetColor("inventory-editor/left-box/background");
            groupBox1.ForeColor= t.GetColor("inventory-editor/left-box/foreground");
            groupBox2.BackColor = t.GetColor("inventory-editor/right-box/background");
            groupBox2.ForeColor = t.GetColor("inventory-editor/right-box/foreground");
        }

        private void zoomSlider_ValueChanged(object sender, EventArgs e)
        {
            UpdatePanelSizes();
            pnlInvWindowImage.Invalidate();
            pnlCursorImage.Invalidate();
        }

        void mouseWheelZoom (object sender, MouseEventArgs e)
        {
            if (ModifierKeys.HasFlag(Keys.Control))
            {
                int movement = e.Delta;
                if (movement > 0)
                {
                    sldZoomLevel.Increase();
                }
                else
                {
                    sldZoomLevel.Decrease();
                }
                UpdatePanelSizes();
                pnlInvWindowImage.Invalidate();
                pnlCursorImage.Invalidate();
            }
        }

        private void pnlCursorImage_MouseWheel(object sender, MouseEventArgs e)
        {
            mouseWheelZoom(sender, e);
        }

        private void pnlInvWindowImage_MouseWheel(object sender, MouseEventArgs e)
        {
            mouseWheelZoom(sender, e);
        }
    }
}
