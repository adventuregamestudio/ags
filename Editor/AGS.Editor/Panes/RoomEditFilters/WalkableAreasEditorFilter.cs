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

		protected override void FilterActivated()
		{
			Factory.GUIController.ShowCuppit("Walkable areas tell AGS where the player is allowed to go within the room. You can also set up scaling so that the player gets smaller or bigger as he walks around different areas.", "Walkable areas introduction");
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
                _selectedArea = ((RoomWalkableArea)newPropertyObject).ID;
                _panel.Invalidate();
            }
            else if (newPropertyObject is Room)
            {
                _selectedArea = 0;
                _panel.Invalidate();
            }
        }

        protected override void Room_OnRegionCountChanged(RoomAreaMaskType maskType)
        {
            if (maskType == RoomAreaMaskType.WalkableAreas)
            {
                SetPropertyGridList();
            }
        }
    }

}
