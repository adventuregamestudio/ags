using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class HotspotsEditorFilter : BaseAreasEditorFilter
    {
        public HotspotsEditorFilter(Panel displayPanel, Room room) : base(displayPanel, room, false)
        {
        }

        public override string DisplayName { get { return "Hotspots"; } }

        public override bool VisibleByDefault { get { return false; } }

        public override RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.Hotspots; }
        }

        public override int ItemCount
        {
            get { return _room.HotspotCount; }
        }

        protected override void SelectedAreaChanged(int areaNumber)
        {
            Factory.GUIController.SetPropertyGridObject(_room.Hotspots[areaNumber]);
        }

		protected override void FilterActivated()
		{
			Factory.GUIController.ShowCuppit("Hotspots allow you to provide interaction with the background image. These are usually used to allow the player to look at parts of the background that can't be changed.", "Hotspots introduction");
		}

        public override void Paint(Graphics graphics, RoomEditorState state)
        {
            base.Paint(graphics, state);

            foreach (RoomHotspot hotspot in _room.Hotspots)
            {
                if ((hotspot.WalkToPoint.X > 0) && (hotspot.WalkToPoint.Y > 0))
                {
                    int x = (hotspot.WalkToPoint.X * state.ScaleFactor) - state.ScrollOffsetX;
                    int y = (hotspot.WalkToPoint.Y * state.ScaleFactor) - state.ScrollOffsetY;
                    graphics.DrawLine(Pens.Red, x - 4, y - 4, x + 4, y + 4);
                    graphics.DrawLine(Pens.RosyBrown, x - 4, y + 4, x + 4, y - 4);
                    graphics.DrawString(hotspot.ID.ToString(), new System.Drawing.Font(FontFamily.GenericSansSerif, 10, FontStyle.Bold), Brushes.Gold, x + 4, y - 7);
                }
            }
        }

        protected override Dictionary<string, int> GetItems()
        {
            Dictionary<string, int> items = new Dictionary<string, int>(_room.Hotspots.Count);
            foreach (RoomHotspot hotspot in _room.Hotspots)
            {
                items.Add(GetItemName(hotspot.ID, hotspot.Name), hotspot.ID);
            }
            return items;
        }

        protected override void SetPropertyGridList()
        {
            Dictionary<string, object> defaultPropertyObjectList = new Dictionary<string, object>();
            defaultPropertyObjectList.Add(_room.PropertyGridTitle, _room);
            foreach (RoomHotspot hotspot in _room.Hotspots)
            {
                defaultPropertyObjectList.Add(hotspot.PropertyGridTitle, hotspot);
            }

            Factory.GUIController.SetPropertyGridObjectList(defaultPropertyObjectList);
        }

        protected override void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is RoomHotspot)
            {
                SelectedArea = ((RoomHotspot)newPropertyObject).ID;
                _panel.Invalidate();
            }
            else if (newPropertyObject is Room)
            {
                DeselectArea();
                _panel.Invalidate();
            }
        }
    }

}
