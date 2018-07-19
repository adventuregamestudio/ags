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
        public WalkableAreasEditorFilter(Panel displayPanel, Room room)
            : base(displayPanel, room, false)
        {
        }

        public override string DisplayName { get { return "Walkable areas"; } }

        public override bool VisibleByDefault { get { return true; } }

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
            Factory.GUIController.SetPropertyGridObject(_room.WalkableAreas[areaNumber]);
        }

        protected override Dictionary<string, int> GetItems()
        {
            Dictionary<string, int> items = new Dictionary<string, int>(_room.WalkableAreas.Count);
            foreach (RoomWalkableArea area in _room.WalkableAreas)
            {
                items.Add(GetItemName(area.ID, area.PropertyGridTitle), area.ID);
            }
            return items;
        }

		protected override void SetPropertyGridList()
        {
            Dictionary<string, object> defaultPropertyObjectList = new Dictionary<string, object>();
            defaultPropertyObjectList.Add(_room.PropertyGridTitle, _room);
            foreach (RoomWalkableArea area in _room.WalkableAreas)
            {
                defaultPropertyObjectList.Add(area.PropertyGridTitle, area);
            }

            Factory.GUIController.SetPropertyGridObjectList(defaultPropertyObjectList);
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
