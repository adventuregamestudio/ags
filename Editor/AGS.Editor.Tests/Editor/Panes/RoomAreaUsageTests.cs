using System;
using System.Linq;
using System.Reflection;
using AGS.Types;
using NUnit.Framework;

namespace AGS.Editor
{
    [TestFixture]
    public class RoomAreaUsageTests
    {
        private static readonly RoomAreaMaskType[] AreaMasks =
        {
            RoomAreaMaskType.Hotspots, RoomAreaMaskType.WalkableAreas, RoomAreaMaskType.WalkBehinds,
            RoomAreaMaskType.Regions
        };

        [Test]
        public void FindDrawnAreasSkipsRowPadding()
        {
            // 3x2 pixels, stride 4: the padding byte holds 9, which is not a pixel
            byte[] pixels = { 0, 2, 2, 9, 5, 0, 2, 9 };
            bool[] drawn = RoomAreaUsage.FindDrawnAreas(pixels, 3, 2, 4);
            Assert.That(drawn.Length, Is.EqualTo(RoomAreaUsage.MASK_VALUES));
            Assert.That(Enumerable.Range(0, drawn.Length).Where(i => drawn[i]), Is.EqualTo(new[] { 0, 2, 5 }));
        }

        [Test]
        public void FindDrawnAreasSeesValue255AndAnEmptyMask()
        {
            Assert.That(RoomAreaUsage.FindDrawnAreas(new byte[] { 255 }, 1, 1, 1)[255], Is.True);
            Assert.That(RoomAreaUsage.FindDrawnAreas(new byte[0], 0, 0, 0).Any(d => d), Is.False);
        }

        [Test]
        public void FindDrawnAreasRefusesAShortBuffer()
        {
            Assert.Throws<ArgumentException>(() => RoomAreaUsage.FindDrawnAreas(new byte[7], 3, 2, 4));
            Assert.Throws<ArgumentException>(() => RoomAreaUsage.FindDrawnAreas(new byte[8], 4, 2, 3));
        }

        [Test]
        public void EveryAreaOfANewRoomHasDefaultProperties()
        {
            Room room = new Room(1);
            foreach (RoomAreaMaskType mask in AreaMasks)
            {
                for (int area = 0; area < Room.GetMaskMaxColor(mask); ++area)
                    Assert.That(RoomAreaUsage.HasDefaultProperties(room, mask, area), Is.True, $"{mask} {area}");
            }
        }

        [Test]
        public void AreasKeepDefaultPropertiesThroughSaveAndLoad()
        {
            Room saved = new Room(1);
            Room room = new Room(saved.ToXmlDocument().SelectSingleNode("Room"));
            foreach (RoomAreaMaskType mask in AreaMasks)
            {
                for (int area = 0; area < Room.GetMaskMaxColor(mask); ++area)
                    Assert.That(RoomAreaUsage.HasDefaultProperties(room, mask, area), Is.True, $"{mask} {area}");
            }
        }

        [Test]
        public void ChangedHotspotHasNoDefaultProperties()
        {
            Room room = new Room(1);
            room.Hotspots[3].ScriptName = "hDoor";
            room.Hotspots[4].DisplayName = "Door";
            room.Hotspots[5].OnAnyClick = "hDoor_AnyClick";
            room.Hotspots[6].WalkToPoint = new System.Drawing.Point(10, 20);
            room.Hotspots[7].Properties.PropertyValues.Add("Weight", new CustomProperty("Weight", "3"));
            for (int area = 3; area <= 7; ++area)
                Assert.That(RoomAreaUsage.HasDefaultProperties(room, RoomAreaMaskType.Hotspots, area), Is.False, area.ToString());
            Assert.That(RoomAreaUsage.HasDefaultProperties(room, RoomAreaMaskType.Hotspots, 2), Is.True);
        }

        [Test]
        public void LegacyHotspotNamesCountAsDefault()
        {
            Room room = new Room(1);
            room.Hotspots[9].ScriptName = "hHotspot9";
            room.Hotspots[9].DisplayName = "Hotspot 9";
            room.Hotspots[10].ScriptName = "hHotspot10";
            room.Hotspots[11].DisplayName = "Hotspot 11";
            room.Hotspots[12].ScriptName = "hHotspot12";
            room.Hotspots[12].DisplayName = "HOTSPOT12";
            for (int area = 9; area <= 12; ++area)
                Assert.That(RoomAreaUsage.HasDefaultProperties(room, RoomAreaMaskType.Hotspots, area), Is.True, area.ToString());
        }

        [TestCase("Hotspot 5", 5, true)]
        [TestCase("HOTSPOT5", 5, true)]
        [TestCase("hotspot 5", 5, true)]
        [TestCase("Hotspot 5", 6, false)]
        [TestCase("Hotspot 50", 5, false)]
        [TestCase("Hotspot  5", 5, false)]
        [TestCase("Door", 5, false)]
        [TestCase("", 5, false)]
        [TestCase(null, 5, false)]
        public void IsLegacyHotspotDisplayName(string displayName, int area, bool expected)
        {
            Assert.That(RoomAreaUsage.IsLegacyHotspotDisplayName(displayName, area), Is.EqualTo(expected));
        }

        [Test]
        public void LegacyHotspotNamesOfAnotherSlotAreNotDefault()
        {
            Room room = new Room(1);
            room.Hotspots[9].ScriptName = "hHotspot8";
            room.Hotspots[10].DisplayName = "Hotspot 8";
            room.Hotspots[11].ScriptName = "hHotspot11";
            room.Hotspots[11].OnAnyClick = "hHotspot11_AnyClick";
            Assert.That(RoomAreaUsage.HasDefaultProperties(room, RoomAreaMaskType.Hotspots, 9), Is.False);
            Assert.That(RoomAreaUsage.HasDefaultProperties(room, RoomAreaMaskType.Hotspots, 10), Is.False);
            Assert.That(RoomAreaUsage.HasDefaultProperties(room, RoomAreaMaskType.Hotspots, 11), Is.False,
                "a legacy name with an event set is in use");
        }

        [Test]
        public void ChangedAreasOfOtherTypesHaveNoDefaultProperties()
        {
            Room room = new Room(1);
            room.WalkableAreas[1].ScalingLevel = 50;
            room.WalkBehinds[2].Baseline = 120;
            room.Regions[3].LightLevel = 10;
            room.Regions[4].OnWalksOnto = "region4_WalksOnto";
            Assert.That(RoomAreaUsage.HasDefaultProperties(room, RoomAreaMaskType.WalkableAreas, 1), Is.False);
            Assert.That(RoomAreaUsage.HasDefaultProperties(room, RoomAreaMaskType.WalkBehinds, 2), Is.False);
            Assert.That(RoomAreaUsage.HasDefaultProperties(room, RoomAreaMaskType.Regions, 3), Is.False);
            Assert.That(RoomAreaUsage.HasDefaultProperties(room, RoomAreaMaskType.Regions, 4), Is.False);
        }

        [Test]
        public void HasDefaultPropertiesLeavesNoSubscriberOnTheInteractionSchema()
        {
            Room room = new Room(1);
            FieldInfo changed = typeof(InteractionSchema).GetField("Changed",
                BindingFlags.Instance | BindingFlags.NonPublic);
            Assert.That(changed, Is.Not.Null, "the event's backing field");
            Func<int> subscribers = () =>
            {
                Delegate handlers = (Delegate)changed.GetValue(InteractionSchema.Instance);
                return handlers == null ? 0 : handlers.GetInvocationList().Length;
            };
            int before = subscribers();
            foreach (RoomAreaMaskType mask in AreaMasks)
            {
                for (int area = 0; area < Room.GetMaskMaxColor(mask); ++area)
                    RoomAreaUsage.HasDefaultProperties(room, mask, area);
            }
            Assert.That(subscribers(), Is.EqualTo(before));
        }

        [Test]
        public void HasDefaultPropertiesRefusesAMaskWithoutAreas()
        {
            Assert.Throws<ArgumentException>(() =>
                RoomAreaUsage.HasDefaultProperties(new Room(1), RoomAreaMaskType.None, 0));
        }

        [TestCase(0, false, true, RoomLayerItemState.Normal)]
        [TestCase(0, false, false, RoomLayerItemState.Normal)]
        [TestCase(1, true, true, RoomLayerItemState.Normal)]
        [TestCase(1, true, false, RoomLayerItemState.Normal)]
        [TestCase(1, false, false, RoomLayerItemState.NothingDrawn)]
        [TestCase(1, false, true, RoomLayerItemState.Unused)]
        public void GetItemState(int area, bool drawn, bool defaultProperties, RoomLayerItemState expected)
        {
            Assert.That(RoomAreaUsage.GetItemState(area, drawn, defaultProperties), Is.EqualTo(expected));
        }
    }
}
