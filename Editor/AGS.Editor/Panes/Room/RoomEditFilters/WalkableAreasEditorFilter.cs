using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class WalkableAreasEditorFilter : BaseAreasEditorFilter
    {
        public WalkableAreasEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room, IRoomController roomController)
            : base(displayPanel, editor, room, roomController)
        {
        }

        public override string Name { get { return "WalkableAreas"; } }
        public override string DisplayName { get { return "Walkable areas"; } }

        public override RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.WalkableAreas; }
        }

        public override int ItemCount
        {
            get { return _room.WalkableAreaCount; }
        }

        protected override void SelectedAreaChanged(int areaNumber)
        {
            SetPropertyGridObject(_room.WalkableAreas[areaNumber]);
        }

        protected override string GetItemName(int id)
        {
            return _room.WalkableAreas[id].PropertyGridTitle;
        }

        protected override SortedDictionary<string, int> InitItemRefs()
        {
            SortedDictionary<string, int> items = new SortedDictionary<string, int>();
            foreach (RoomWalkableArea area in _room.WalkableAreas)
            {
                items.Add(GetItemID(area.ID), area.ID);
            }
            return items;
        }

        protected override Dictionary<string, object> GetPropertyGridList()
        {
            var list = new Dictionary<string, object>();
            list.Add(_room.PropertyGridTitle, _room);
            foreach (RoomWalkableArea area in _room.WalkableAreas)
            {
                list.Add(area.PropertyGridTitle, area);
            }
            return list;
        }

        protected override void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is RoomWalkableArea)
            {
                SelectedArea = ((RoomWalkableArea)newPropertyObject).ID;
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
