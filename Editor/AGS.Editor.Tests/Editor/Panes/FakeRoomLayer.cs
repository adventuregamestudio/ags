using System.Collections.Generic;
using AGS.Types;

namespace AGS.Editor.Panes.Room
{
    /// <summary>
    /// A room layer for the layer tree's tests: items named "Item &lt;id&gt;", states set by hand.
    /// </summary>
    internal class FakeRoomLayer : IRoomLayer
    {
        public FakeRoomLayer(string name, string displayName, RoomAreaMaskType mask, bool supportVisibleItems,
            params string[] itemIDs)
        {
            Name = name;
            DisplayName = displayName;
            MaskToDraw = mask;
            SupportVisibleItems = supportVisibleItems;
            DesignItems = new SortedDictionary<string, DesignTimeProperties>();
            foreach (string id in itemIDs)
            {
                DesignItems.Add(id, new DesignTimeProperties());
                ItemNames[id] = "Item " + id;
            }
        }

        public string Name { get; }
        public string DisplayName { get; }
        public RoomAreaMaskType MaskToDraw { get; }
        public bool SupportVisibleItems { get; }
        public bool Visible { get; set; }
        public bool Locked { get; set; }
        public bool Modified { get; set; }
        public SortedDictionary<string, DesignTimeProperties> DesignItems { get; }
        public Dictionary<string, string> ItemNames { get; } = new Dictionary<string, string>();
        public Dictionary<string, RoomLayerItemState> ItemStates { get; } = new Dictionary<string, RoomLayerItemState>();

        public string GetItemName(string id)
        {
            string name;
            return ItemNames.TryGetValue(id, out name) ? name : null;
        }

        public RoomLayerItemState GetItemState(string id)
        {
            RoomLayerItemState state;
            return ItemStates.TryGetValue(id, out state) ? state : RoomLayerItemState.Normal;
        }
    }
}
