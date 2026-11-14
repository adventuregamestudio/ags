using System.Collections.Generic;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// How an item is listed in the room editor's layer tree.
    /// </summary>
    public enum RoomLayerItemState
    {
        Normal,
        /// <summary>
        /// An area with properties set but nothing drawn in its mask.
        /// </summary>
        NothingDrawn,
        /// <summary>
        /// An area with nothing drawn and default properties, listed in the "Unused" group.
        /// </summary>
        Unused
    }

    /// <summary>
    /// The part of a room editor layer that the layer tree uses, without UI types.
    /// </summary>
    public interface IRoomLayer
    {
        /// <summary>
        /// Internal ID of the layer.
        /// </summary>
        string Name { get; }
        /// <summary>
        /// Displayed name of the layer.
        /// </summary>
        string DisplayName { get; }
        /// <summary>
        /// Which room's mask is associated with this layer.
        /// </summary>
        RoomAreaMaskType MaskToDraw { get; }
        /// <summary>
        /// Whether layer items may be shown/hidden in the editor.
        /// </summary>
        bool SupportVisibleItems { get; }
        /// <summary>
        /// Gets/sets if this layer is visible.
        /// </summary>
        bool Visible { get; set; }
        /// <summary>
        /// Gets/sets if this layer is locked (cannot be modified).
        /// </summary>
        bool Locked { get; set; }
        /// <summary>
        /// Gets/sets whether the design-time properties of the layer or items were modified.
        /// </summary>
        bool Modified { get; set; }
        /// <summary>
        /// The dictionary that maps an object ID to its design-time properties.
        /// </summary>
        SortedDictionary<string, DesignTimeProperties> DesignItems { get; }
        /// <summary>
        /// Gets a human-readable room item's name.
        /// </summary>
        string GetItemName(string id);
        RoomLayerItemState GetItemState(string id);
    }
}
