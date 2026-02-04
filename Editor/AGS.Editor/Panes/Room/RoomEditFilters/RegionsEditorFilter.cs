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
        public RegionsEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room)
            : base(displayPanel, editor, room)
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
            return MakeLayerItemName("Region", null, null, id);
        }

        /// <summary>
        /// Tries to find and select a room item either matching its script name, or,
        /// if no such matching script name was found, then by its numeric ID,
        /// embedded in this name (e.g. Object0).
        /// </summary>
        public override bool TrySelectItemByName(string name)
        {
            int id;
            if (name.StartsWith("Region") && int.TryParse(name.Substring(6), out id)
                && (id >= 0) && (id < ItemCount))
            {
                SelectedArea = id;
                SelectedAreaChanged(id);
                return true;
            }
            return false;
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
