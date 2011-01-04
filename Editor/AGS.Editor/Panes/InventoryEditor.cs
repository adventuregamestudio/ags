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
        private int IMAGE_SCALE_FACTOR = 2;
        private InventoryItem _item;

        public InventoryEditor()
        {
            InitializeComponent();
        }

        public InventoryEditor(InventoryItem itemToEdit) : this()
        {
            ItemToEdit = itemToEdit;
        }

        protected override string OnGetHelpKeyword()
        {
            return "Inventory";
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
        }

        private void pnlCursorImage_Paint(object sender, PaintEventArgs e)
        {
            if (_item != null)
            {
                IMAGE_SCALE_FACTOR = Factory.AGSEditor.CurrentGame.GUIScaleFactor;

                IntPtr hdc = e.Graphics.GetHdc();
                Factory.NativeProxy.DrawSprite(hdc, 0, 0, _item.CursorImage);
                e.Graphics.ReleaseHdc();
                if ((_item.HotspotX > 0) && (_item.HotspotY > 0))
                {
                    e.Graphics.DrawLine(Pens.LightGreen, (_item.HotspotX - 2) * IMAGE_SCALE_FACTOR, _item.HotspotY * IMAGE_SCALE_FACTOR, (_item.HotspotX + 2) * IMAGE_SCALE_FACTOR, _item.HotspotY * IMAGE_SCALE_FACTOR);
                    e.Graphics.DrawLine(Pens.Blue, _item.HotspotX * IMAGE_SCALE_FACTOR, (_item.HotspotY - 2) * IMAGE_SCALE_FACTOR, _item.HotspotX * IMAGE_SCALE_FACTOR, (_item.HotspotY + 2) * IMAGE_SCALE_FACTOR);
                }
            }
        }

        private void pnlCursorImage_MouseDown(object sender, MouseEventArgs e)
        {
            if (_item != null)
            {
                int spriteWidth, spriteHeight;
                Utilities.GetSizeSpriteWillBeRenderedInGame(_item.CursorImage, out spriteWidth, out spriteHeight);

                IMAGE_SCALE_FACTOR = Factory.AGSEditor.CurrentGame.GUIScaleFactor;
                int newHotspotX = e.X / IMAGE_SCALE_FACTOR;
                int newHotspotY = e.Y / IMAGE_SCALE_FACTOR;
                if ((newHotspotX > 0) && (newHotspotY > 0) &&
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
                Factory.NativeProxy.DrawSprite(hdc, 0, 0, _item.Image);
                e.Graphics.ReleaseHdc();
            }
        }

    }
}
