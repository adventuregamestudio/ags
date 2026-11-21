using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// Tells which room areas are in use, for the layer tree's "Unused" group.
    /// </summary>
    public static class RoomAreaUsage
    {
        public const int MASK_VALUES = 256;

        /// <summary>
        /// Tells for each 8-bit value whether the mask has a pixel of it.
        /// </summary>
        public static bool[] FindDrawnAreas(byte[] pixels, int width, int height, int stride)
        {
            if (width < 0 || height < 0 || stride < width || (long)stride * height > pixels.Length)
                throw new ArgumentException("The pixel buffer is smaller than its dimensions.");
            bool[] drawn = new bool[MASK_VALUES];
            for (int y = 0; y < height; ++y)
            {
                int row = y * stride;
                for (int x = 0; x < width; ++x)
                    drawn[pixels[row + x]] = true;
            }
            return drawn;
        }

        /// <summary>
        /// Tells whether an area's properties equal a new area's. They are compared as XML,
        /// so that properties added later are compared too.
        /// </summary>
        public static bool HasDefaultProperties(Room room, RoomAreaMaskType mask, int area)
        {
            IToXml item;
            IToXml fresh;
            Interactions freshInteractions = null;
            switch (mask)
            {
                case RoomAreaMaskType.Hotspots:
                    RoomHotspot hotspot = room.Hotspots[area];
                    RoomHotspot freshHotspot = new RoomHotspot(room) { ID = area };
                    // Older editors gave every hotspot these names
                    if (hotspot.ScriptName == LegacyHotspotScriptName(area))
                        freshHotspot.ScriptName = hotspot.ScriptName;
                    if (IsLegacyHotspotDisplayName(hotspot.DisplayName, area))
                        freshHotspot.DisplayName = hotspot.DisplayName;
                    item = hotspot;
                    fresh = freshHotspot;
                    freshInteractions = freshHotspot.Interactions;
                    break;
                case RoomAreaMaskType.WalkableAreas:
                    item = room.WalkableAreas[area];
                    fresh = new RoomWalkableArea(room) { ID = area };
                    break;
                case RoomAreaMaskType.WalkBehinds:
                    item = room.WalkBehinds[area];
                    fresh = new RoomWalkBehind(room) { ID = area };
                    break;
                case RoomAreaMaskType.Regions:
                    item = room.Regions[area];
                    RoomRegion freshRegion = new RoomRegion(room) { ID = area };
                    fresh = freshRegion;
#pragma warning disable 0612
                    freshInteractions = freshRegion.Interactions;
#pragma warning restore 0612
                    break;
                default:
                    throw new ArgumentException($"Illegal mask type, mask {mask} has no areas");
            }
            try
            {
                return item.ToXmlDocument().OuterXml == fresh.ToXmlDocument().OuterXml;
            }
            finally
            {
                // The new area's Interactions subscribed to the global schema
                if (freshInteractions != null)
                    freshInteractions.Schema = null;
            }
        }

        public static string LegacyHotspotScriptName(int area)
        {
            return "hHotspot" + area.ToString(System.Globalization.CultureInfo.InvariantCulture);
        }

        /// <summary>
        /// Tells whether a description is "Hotspot n", in any case, with or without the space.
        /// </summary>
        public static bool IsLegacyHotspotDisplayName(string displayName, int area)
        {
            if (string.IsNullOrEmpty(displayName))
                return false;
            string number = area.ToString(System.Globalization.CultureInfo.InvariantCulture);
            return string.Equals(displayName, "Hotspot " + number, StringComparison.OrdinalIgnoreCase) ||
                string.Equals(displayName, "Hotspot" + number, StringComparison.OrdinalIgnoreCase);
        }

        public static RoomLayerItemState GetItemState(int area, bool drawn, bool defaultProperties)
        {
            if (area == 0 || drawn)
                return RoomLayerItemState.Normal;
            return defaultProperties ? RoomLayerItemState.Unused : RoomLayerItemState.NothingDrawn;
        }
    }
}
