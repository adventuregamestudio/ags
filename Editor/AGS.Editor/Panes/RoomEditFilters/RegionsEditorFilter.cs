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
        public RegionsEditorFilter(Panel displayPanel, Room room)
            : base(displayPanel, room)
        {
        }

        public override RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.Regions; }
        }

        protected override void SelectedAreaChanged(int areaNumber)
        {
            Factory.GUIController.SetPropertyGridObject(_room.Regions[areaNumber]);
        }

		protected override void FilterActivated()
		{
			Factory.GUIController.ShowCuppit("Regions define what happens when the player walks around different parts of the room. You can set up lighting and scripts to run as the player walks onto different regions.", "Regions introduction");
		}

		protected override void SetPropertyGridList()
        {
            Dictionary<string, object> defaultPropertyObjectList = new Dictionary<string, object>();
            defaultPropertyObjectList.Add(_room.PropertyGridTitle, _room);
            foreach (RoomRegion area in _room.Regions)
            {
                defaultPropertyObjectList.Add(area.PropertyGridTitle, area);
            }

            Factory.GUIController.SetPropertyGridObjectList(defaultPropertyObjectList);
        }

        protected override void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is RoomRegion)
            {
                _selectedArea = ((RoomRegion)newPropertyObject).ID;
                _panel.Invalidate();
            }
            else if (newPropertyObject is Room)
            {
                _selectedArea = 0;
                _panel.Invalidate();
            }
        }
    }

}
