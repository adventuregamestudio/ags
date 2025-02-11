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
        public HotspotsEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room, IRoomController roomController)
            : base(displayPanel, editor, room, roomController)
        {
        }

        public override string Name { get { return "Hotspots"; } }
        public override string DisplayName { get { return "Hotspots"; } }

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
            SetPropertyGridObject(_room.Hotspots[areaNumber]);
        }

        public override void Paint(Graphics graphics, RoomEditorState state)
        {
            base.Paint(graphics, state);

            foreach (RoomHotspot hotspot in _room.Hotspots)
            {
                if ((hotspot.WalkToPoint.X > 0) && (hotspot.WalkToPoint.Y > 0))
                {
                    int x = state.RoomXToWindow(hotspot.WalkToPoint.X);
                    int y = state.RoomYToWindow(hotspot.WalkToPoint.Y);
                    graphics.DrawLine(Pens.Red, x - 4, y - 4, x + 4, y + 4);
                    graphics.DrawLine(Pens.RosyBrown, x - 4, y + 4, x + 4, y - 4);
                    graphics.DrawString(hotspot.ID.ToString(), new System.Drawing.Font(FontFamily.GenericSansSerif, 10, FontStyle.Bold), Brushes.Gold, x + 4, y - 7);
                }
            }
        }

        protected override string GetItemName(int id)
        {
            return MakeLayerItemName("Hotspot", _room.Hotspots[id].Name, _room.Hotspots[id].Description, id);
        }

        protected override SortedDictionary<string, int> InitItemRefs()
        {
            SortedDictionary<string, int> items = new SortedDictionary<string, int>();
            foreach (RoomHotspot hotspot in _room.Hotspots)
            {
                items.Add(GetItemID(hotspot.ID), hotspot.ID);
            }
            return items;
        }

        protected override Dictionary<string, object> GetPropertyGridList()
        {
            var list = new Dictionary<string, object>();
            list.Add(_room.PropertyGridTitle, _room);
            foreach (RoomHotspot hotspot in _room.Hotspots)
            {
                list.Add(hotspot.PropertyGridTitle, hotspot);
            }
            return list;
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
