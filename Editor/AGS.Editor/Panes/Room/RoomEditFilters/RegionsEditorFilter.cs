using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class RegionsEditorFilter : BaseAreasEditorFilter
    {
        public RegionsEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room, IRoomController roomController)
            : base(displayPanel, editor, room, roomController)
        {
        }

        public override string Name { get { return "Regions"; } }
        public override string DisplayName { get { return "Regions"; } }

        public override RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.Regions; }
        }

        public override int ItemCount
        {
            get { return _room.RegionCount; }
        }

        protected override void SelectedAreaChanged(int areaNumber)
        {
            SetPropertyGridObject(_room.Regions[areaNumber]);
        }

        protected override string GetItemName(int id)
        {
            return _room.Regions[id].PropertyGridTitle;
        }

        protected override SortedDictionary<string, int> InitItemRefs()
        {
            SortedDictionary<string, int> items = new SortedDictionary<string, int>();
            foreach (RoomRegion area in _room.Regions)
            {
                items.Add(GetItemID(area.ID), area.ID);
            }
            return items;
        }

        protected override Dictionary<string, object> GetPropertyGridList()
        {
            var list = new Dictionary<string, object>();
            list.Add(_room.PropertyGridTitle, _room);
            foreach (RoomRegion area in _room.Regions)
            {
                list.Add(area.PropertyGridTitle, area);
            }
            return list;
        }

        protected override void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is RoomRegion)
            {
                SelectedArea = ((RoomRegion)newPropertyObject).ID;
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
