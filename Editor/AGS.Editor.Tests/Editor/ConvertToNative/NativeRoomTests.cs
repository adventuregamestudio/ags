using AGS.Types;
using NUnit.Framework;
using System;

namespace AGS.Editor
{
    public class NativeRoomTests
    {
        private Room ConvertRoomToNativeAndBack(Room srcRoom)
        {
            Native.NativeRoom nativeRoom = null;
            try
            {
                nativeRoom = new Native.NativeRoom(srcRoom);
            }
            catch (Exception)
            {
            }

            Assert.That(nativeRoom != null);

            if (nativeRoom == null)
                return null;

            Room dstRoom = null;
            try
            {
                dstRoom = nativeRoom.ConvertToManagedRoom(0, null);
            }
            catch (Exception)
            {
            }

            Assert.That(dstRoom != null);

            return dstRoom;
        }

        [Test]
        public void ConvertBetweenEditorAndNative_Test()
        {
            InteractionSchema.Instance.Events = new InteractionEvent[2]
            {
                new InteractionEvent(0, "Look", "Look", "Look", "Look"),
                new InteractionEvent(1, "Interact", "Interact", "Interact", "Interact"),
            };

            Room srcRoom = new Room(0);
            srcRoom.BackgroundAnimationDelay = 4;
            srcRoom.BackgroundAnimationEnabled = true;
            srcRoom.BackgroundCount = 3;
            srcRoom.BottomEdgeY = 160;
            srcRoom.ColorDepth = 32;
            srcRoom.Description = "This is a test room"; // NOTE: cannot be tested, is not present in the Native Room
            srcRoom.FaceDirectionRatio = 0.5f;
            srcRoom.GameID = 12345;
            srcRoom.Height = 220;
            srcRoom.LeftEdgeX = 40;
            srcRoom.MaskResolution = 2;
            srcRoom.OnAfterFadeIn = "Room_AfterFadeIn";
            srcRoom.OnFirstTimeEnter = "Room_OnFirstTimeEnter";
            srcRoom.OnLeave = "Room_OnLeave";
            srcRoom.OnLeaveBottom = "Room_OnLeaveBottom";
            srcRoom.OnLeaveLeft = "Room_OnLeaveLeft";
            srcRoom.OnLeaveRight = "Room_OnLeaveRight";
            srcRoom.OnLeaveTop = "Room_OnLeaveTop";
            srcRoom.OnLoad = "Room_OnLoad";
            srcRoom.OnRepExec = "Room_OnRepExec";
            srcRoom.OnUnload = "Room_OnUnload";
            srcRoom.PlayerCharacterView = 16;
            srcRoom.RightEdgeX = 300;
            srcRoom.ShowPlayerCharacter = false;
            srcRoom.TopEdgeY = 20;
            srcRoom.Width = 340;

            srcRoom.Properties.PropertyValues.Add("RoomProperty", new CustomProperty("RoomProperty", "readme"));
            srcRoom.Properties.PropertyValues.Add("AnotherRoomProperty", new CustomProperty("AnotherRoomProperty", "also readme"));

            RoomHotspot h = new RoomHotspot(srcRoom);
            h.Description = "This is some hotspot";
            h.ID = 0;
            h.Interactions.ScriptFunctionNames.Add("Look", "hSomeHotspot_Look");
            h.Interactions.ScriptFunctionNames.Add("Interact", "hSomeHotspot_Interact");
            h.Name = "hSomeHotspot";
            h.OnAnyClick = "hSomeHotspot_OnAnyClick";
            h.OnMouseMove = "hSomeHotspot_OnMouseMove";
            h.OnWalkOn = "hSomeHotspot_OnWalkOn";
            h.Properties.PropertyValues.Add("MyProperty", new CustomProperty("MyProperty", "value"));
            h.Properties.PropertyValues.Add("MySecondProperty", new CustomProperty("MySecondProperty", "value2"));
            h.WalkToPoint = new System.Drawing.Point(11, 22);
            srcRoom.Hotspots[0] = h;
            h = new RoomHotspot(srcRoom);
            h.Description = "Absolutely different hotspot";
            h.ID = 1;
            h.Interactions.ScriptFunctionNames.Add("Look", "hDifferentHotspot_Look");
            h.Interactions.ScriptFunctionNames.Add("Interact", "hDifferentHotspot_Interact");
            h.Name = "hDifferentHotspot";
            h.OnAnyClick = "hDifferentHotspot_OnAnyClick";
            h.OnMouseMove = "hDifferentHotspot_OnMouseMove";
            h.OnWalkOn = "hDifferentHotspot_OnWalkOn";
            h.Properties.PropertyValues.Add("MyProperty", new CustomProperty("MyProperty", "different value"));
            h.Properties.PropertyValues.Add("MySecondProperty", new CustomProperty("MySecondProperty", "different value2"));
            h.WalkToPoint = new System.Drawing.Point(100, 200);
            srcRoom.Hotspots[1] = h;

            RoomObject o = new RoomObject(srcRoom);
            o.Baseline = 40;
            o.BaselineOverridden = true;
            o.BlendMode = BlendMode.Darken;
            o.BlockingRectangle = new System.Drawing.Rectangle(1, 4, 12, 11);
            o.Clickable = true;
            o.Description = "The table";
            o.Enabled = false;
            o.GraphicAnchor = new GraphicAnchor(FrameAlignment.BottomRight);
            o.GraphicOffset = new System.Drawing.Point(1, 4);
            o.ID = 0;
            o.Image = 33;
            o.Interactions.ScriptFunctionNames.Add("Look", "oTable_Look");
            o.Interactions.ScriptFunctionNames.Add("Interact", "oTable_Interact");
            o.Name = "oTable";
            o.OnAnyClick = "oTable_AnyClick";
            o.OnFrameEvent = "oTable_OnFrameEvent";
            o.Properties.PropertyValues.Add("MyProperty", new CustomProperty("MyProperty", "xxx"));
            o.Properties.PropertyValues.Add("MySecondProperty", new CustomProperty("MySecondProperty", "yyy"));
            o.Solid = true;
            o.StartX = 10;
            o.StartY = 20;
            o.Transparency = 50;
            o.UseRoomAreaLighting = true;
            o.UseRoomAreaScaling = false;
            o.Visible = true;
            srcRoom.Objects.Add(o);
            o = new RoomObject(srcRoom);
            o.Baseline = 0;
            o.BaselineOverridden = false;
            o.BlendMode = BlendMode.CopyAlpha;
            o.BlockingRectangle = new System.Drawing.Rectangle(-5, 0, 8, 16);
            o.Clickable = false;
            o.Description = "The chair";
            o.Enabled = true;
            o.GraphicAnchor = new GraphicAnchor(FrameAlignment.TopCenter);
            o.GraphicOffset = new System.Drawing.Point(-5, 1);
            o.ID = 1;
            o.Image = 44;
            o.Interactions.ScriptFunctionNames.Add("Look", "oChair_Look");
            o.Interactions.ScriptFunctionNames.Add("Interact", "oChair_Interact");
            o.Name = "oChair";
            o.OnAnyClick = "oChair_AnyClick";
            o.OnFrameEvent = "oChair_OnFrameEvent";
            o.Properties.PropertyValues.Add("MyProperty", new CustomProperty("MyProperty", "aaa"));
            o.Properties.PropertyValues.Add("MySecondProperty", new CustomProperty("MySecondProperty", "bbb"));
            o.Solid = false;
            o.StartX = 100;
            o.StartY = -20;
            o.Transparency = 10;
            o.UseRoomAreaLighting = false;
            o.UseRoomAreaScaling = true;
            o.Visible = false;
            srcRoom.Objects.Add(o);

            RoomRegion r = new RoomRegion(srcRoom);
            r.BlueTint = 10;
            r.GreenTint = 20;
            r.ID = 0;
            r.LightLevel = 100; // default
            r.OnStanding = "Region1_StandOn";
            r.OnWalksOff = "Region1_WalkOff";
            r.OnWalksOnto = "Region1_WalkOn";
            r.Properties.PropertyValues.Add("RegionProperty", new CustomProperty("RegionProperty", "Region1value"));
            r.RedTint = 40;
            r.TintLuminance = 50;
            r.TintSaturation = 60;
            r.UseColourTint = true;
            srcRoom.Regions[0] = r;
            r = new RoomRegion(srcRoom);
            r.BlueTint = 0; // default
            r.GreenTint = 0;// default
            r.ID = 1;
            r.LightLevel = 80;
            r.OnStanding = "Region2_StandOn";
            r.OnWalksOff = "Region2_WalkOff";
            r.OnWalksOnto = "Region2_WalkOn";
            r.Properties.PropertyValues.Add("RegionProperty", new CustomProperty("RegionProperty", "Region2value"));
            r.RedTint = 0; // default
            r.TintLuminance = 100; // default
            r.TintSaturation = 50; // default
            r.UseColourTint = false;
            srcRoom.Regions[1] = r;

            RoomWalkableArea area = new RoomWalkableArea(srcRoom);
            area.AreaSpecificView = 4;
            area.FaceDirectionRatio = 1.2f;
            area.ID = 0;
            area.MaxScalingLevel = 200;
            area.MinScalingLevel = 50;
            area.Properties.PropertyValues.Add("WalkableProperty", new CustomProperty("WalkableProperty", "Walkable1value"));
            area.ScalingLevel = 50; // with continuous scaling = MinScalingLevel
            area.UseContinuousScaling = true;
            srcRoom.WalkableAreas[0] = area;
            area = new RoomWalkableArea(srcRoom);
            area.AreaSpecificView = 10;
            area.FaceDirectionRatio = 0.3f;
            area.ID = 1;
            area.MaxScalingLevel = 70; // non-continuous scaling
            area.MinScalingLevel = 70; // non-continuous scaling
            area.Properties.PropertyValues.Add("WalkableProperty", new CustomProperty("WalkableProperty", "Walkable2value"));
            area.ScalingLevel = 70;
            area.UseContinuousScaling = false;
            srcRoom.WalkableAreas[1] = area;

            RoomWalkBehind wb = new RoomWalkBehind(srcRoom);
            wb.Baseline = 77;
            wb.ID = 0;
            srcRoom.WalkBehinds[0] = wb;
            wb = new RoomWalkBehind(srcRoom);
            wb.Baseline = 33;
            wb.ID = 1;
            srcRoom.WalkBehinds[1] = wb;

            Room dstRoom = ConvertRoomToNativeAndBack(srcRoom);
            if (dstRoom == null)
                return;

            Assert.That(dstRoom.BackgroundAnimationDelay, Is.EqualTo(4));
            Assert.That(dstRoom.BackgroundAnimationEnabled, Is.EqualTo(true));
            Assert.That(dstRoom.BackgroundCount, Is.EqualTo(3));
            Assert.That(dstRoom.BottomEdgeY, Is.EqualTo(160));
            Assert.That(dstRoom.ColorDepth, Is.EqualTo(32));
            // Room.Description cannot be tested, because it's not part of the Native Room
            //Assert.That(dstRoom.Description, Is.EqualTo("This is a test room"));
            Assert.That(dstRoom.FaceDirectionRatio, Is.EqualTo(0.5f));
            Assert.That(dstRoom.GameID, Is.EqualTo(12345));
            Assert.That(dstRoom.Height, Is.EqualTo(220));
            Assert.That(dstRoom.LeftEdgeX, Is.EqualTo(40));
            Assert.That(dstRoom.MaskResolution, Is.EqualTo(2));
            Assert.That(dstRoom.OnAfterFadeIn, Is.EqualTo("Room_AfterFadeIn"));
            Assert.That(dstRoom.OnFirstTimeEnter, Is.EqualTo("Room_OnFirstTimeEnter"));
            Assert.That(dstRoom.OnLeave, Is.EqualTo("Room_OnLeave"));
            Assert.That(dstRoom.OnLeaveBottom, Is.EqualTo("Room_OnLeaveBottom"));
            Assert.That(dstRoom.OnLeaveLeft, Is.EqualTo("Room_OnLeaveLeft"));
            Assert.That(dstRoom.OnLeaveRight, Is.EqualTo("Room_OnLeaveRight"));
            Assert.That(dstRoom.OnLeaveTop, Is.EqualTo("Room_OnLeaveTop"));
            Assert.That(dstRoom.OnLoad, Is.EqualTo("Room_OnLoad"));
            Assert.That(dstRoom.OnRepExec, Is.EqualTo("Room_OnRepExec"));
            Assert.That(dstRoom.OnUnload, Is.EqualTo("Room_OnUnload"));
            Assert.That(dstRoom.PlayerCharacterView, Is.EqualTo(16));
            Assert.That(dstRoom.RightEdgeX, Is.EqualTo(300));
            Assert.That(dstRoom.ShowPlayerCharacter, Is.EqualTo(false));
            Assert.That(dstRoom.TopEdgeY, Is.EqualTo(20));
            Assert.That(dstRoom.Width, Is.EqualTo(340));

            Assert.That(dstRoom.Properties.PropertyValues.ContainsKey("RoomProperty"));
            Assert.That(dstRoom.Properties.PropertyValues["RoomProperty"].Value, Is.EqualTo("readme"));
            Assert.That(dstRoom.Properties.PropertyValues.ContainsKey("AnotherRoomProperty"));
            Assert.That(dstRoom.Properties.PropertyValues["AnotherRoomProperty"].Value, Is.EqualTo("also readme"));

            Assert.That(dstRoom.HotspotCount, Is.EqualTo(50)); // Hotspots count is always fixed
            h = dstRoom.Hotspots[0];
            Assert.That(h.Description, Is.EqualTo("This is some hotspot"));
            Assert.That(h.ID, Is.EqualTo(0));
            Assert.That(h.Name, Is.EqualTo("hSomeHotspot"));
            Assert.That(h.OnAnyClick, Is.EqualTo("hSomeHotspot_OnAnyClick"));
            Assert.That(h.OnMouseMove, Is.EqualTo("hSomeHotspot_OnMouseMove"));
            Assert.That(h.OnWalkOn, Is.EqualTo("hSomeHotspot_OnWalkOn"));
            Assert.That(h.WalkToPoint, Is.EqualTo(new System.Drawing.Point(11, 22)));
            Assert.That(h.Interactions.ScriptFunctionNames.ContainsKey("Look"));
            Assert.That(h.Interactions.ScriptFunctionNames["Look"], Is.EqualTo("hSomeHotspot_Look"));
            Assert.That(h.Interactions.ScriptFunctionNames.ContainsKey("Interact"));
            Assert.That(h.Interactions.ScriptFunctionNames["Interact"], Is.EqualTo("hSomeHotspot_Interact"));
            Assert.That(h.Properties.PropertyValues.ContainsKey("MyProperty"));
            Assert.That(h.Properties.PropertyValues["MyProperty"].Value, Is.EqualTo("value"));
            Assert.That(h.Properties.PropertyValues.ContainsKey("MySecondProperty"));
            Assert.That(h.Properties.PropertyValues["MySecondProperty"].Value, Is.EqualTo("value2"));

            h = dstRoom.Hotspots[1];
            Assert.That(h.Description, Is.EqualTo("Absolutely different hotspot"));
            Assert.That(h.ID, Is.EqualTo(1));
            Assert.That(h.Name, Is.EqualTo("hDifferentHotspot"));
            Assert.That(h.OnAnyClick, Is.EqualTo("hDifferentHotspot_OnAnyClick"));
            Assert.That(h.OnMouseMove, Is.EqualTo("hDifferentHotspot_OnMouseMove"));
            Assert.That(h.OnWalkOn, Is.EqualTo("hDifferentHotspot_OnWalkOn"));
            Assert.That(h.WalkToPoint, Is.EqualTo(new System.Drawing.Point(100, 200)));
            Assert.That(h.Interactions.ScriptFunctionNames.ContainsKey("Look"));
            Assert.That(h.Interactions.ScriptFunctionNames["Look"], Is.EqualTo("hDifferentHotspot_Look"));
            Assert.That(h.Interactions.ScriptFunctionNames.ContainsKey("Interact"));
            Assert.That(h.Interactions.ScriptFunctionNames["Interact"], Is.EqualTo("hDifferentHotspot_Interact"));
            Assert.That(h.Properties.PropertyValues.ContainsKey("MyProperty"));
            Assert.That(h.Properties.PropertyValues["MyProperty"].Value, Is.EqualTo("different value"));
            Assert.That(h.Properties.PropertyValues.ContainsKey("MySecondProperty"));
            Assert.That(h.Properties.PropertyValues["MySecondProperty"].Value, Is.EqualTo("different value2"));

            Assert.That(dstRoom.Objects.Count, Is.EqualTo(2));
            o = dstRoom.Objects[0];
            Assert.That(o.Baseline, Is.EqualTo(40));
            Assert.That(o.BaselineOverridden, Is.EqualTo(true));
            Assert.That(o.BlendMode, Is.EqualTo(BlendMode.Darken));
            Assert.That(o.BlockingRectangle, Is.EqualTo(new System.Drawing.Rectangle(1, 4, 12, 11)));
            Assert.That(o.Clickable, Is.EqualTo(true));
            Assert.That(o.Description, Is.EqualTo("The table"));
            Assert.That(o.Enabled, Is.EqualTo(false));
            Assert.That(o.GraphicAnchor, Is.EqualTo(new GraphicAnchor(FrameAlignment.BottomRight)));
            Assert.That(o.GraphicOffset, Is.EqualTo(new System.Drawing.Point(1, 4)));
            Assert.That(o.ID, Is.EqualTo(0));
            Assert.That(o.Image, Is.EqualTo(33));
            Assert.That(o.Name, Is.EqualTo("oTable"));
            Assert.That(o.OnAnyClick, Is.EqualTo("oTable_AnyClick"));
            Assert.That(o.OnFrameEvent, Is.EqualTo("oTable_OnFrameEvent"));
            Assert.That(o.Solid, Is.EqualTo(true));
            Assert.That(o.StartX, Is.EqualTo(10));
            Assert.That(o.StartY, Is.EqualTo(20));
            Assert.That(o.Transparency, Is.EqualTo(50));
            Assert.That(o.UseRoomAreaLighting, Is.EqualTo(true));
            Assert.That(o.UseRoomAreaScaling, Is.EqualTo(false));
            Assert.That(o.Visible, Is.EqualTo(true));
            Assert.That(o.Interactions.ScriptFunctionNames.ContainsKey("Look"));
            Assert.That(o.Interactions.ScriptFunctionNames["Look"], Is.EqualTo("oTable_Look"));
            Assert.That(o.Interactions.ScriptFunctionNames.ContainsKey("Interact"));
            Assert.That(o.Interactions.ScriptFunctionNames["Interact"], Is.EqualTo("oTable_Interact"));
            Assert.That(o.Properties.PropertyValues.ContainsKey("MyProperty"));
            Assert.That(o.Properties.PropertyValues["MyProperty"].Value, Is.EqualTo("xxx"));
            Assert.That(o.Properties.PropertyValues.ContainsKey("MySecondProperty"));
            Assert.That(o.Properties.PropertyValues["MySecondProperty"].Value, Is.EqualTo("yyy"));

            o = dstRoom.Objects[1];
            Assert.That(o.Baseline, Is.EqualTo(0));
            Assert.That(o.BaselineOverridden, Is.EqualTo(false));
            Assert.That(o.BlendMode, Is.EqualTo(BlendMode.CopyAlpha));
            Assert.That(o.BlockingRectangle, Is.EqualTo(new System.Drawing.Rectangle(-5, 0, 8, 16)));
            Assert.That(o.Clickable, Is.EqualTo(false));
            Assert.That(o.Description, Is.EqualTo("The chair"));
            Assert.That(o.Enabled, Is.EqualTo(true));
            Assert.That(o.GraphicAnchor, Is.EqualTo(new GraphicAnchor(FrameAlignment.TopCenter)));
            Assert.That(o.GraphicOffset, Is.EqualTo(new System.Drawing.Point(-5, 1)));
            Assert.That(o.ID, Is.EqualTo(1));
            Assert.That(o.Image, Is.EqualTo(44));
            Assert.That(o.Name, Is.EqualTo("oChair"));
            Assert.That(o.OnAnyClick, Is.EqualTo("oChair_AnyClick"));
            Assert.That(o.OnFrameEvent, Is.EqualTo("oChair_OnFrameEvent"));
            Assert.That(o.Solid, Is.EqualTo(false));
            Assert.That(o.StartX, Is.EqualTo(100));
            Assert.That(o.StartY, Is.EqualTo(-20));
            Assert.That(o.Transparency, Is.EqualTo(10));
            Assert.That(o.UseRoomAreaLighting, Is.EqualTo(false));
            Assert.That(o.UseRoomAreaScaling, Is.EqualTo(true));
            Assert.That(o.Visible, Is.EqualTo(false));
            Assert.That(o.Interactions.ScriptFunctionNames.ContainsKey("Look"));
            Assert.That(o.Interactions.ScriptFunctionNames["Look"], Is.EqualTo("oChair_Look"));
            Assert.That(o.Interactions.ScriptFunctionNames.ContainsKey("Interact"));
            Assert.That(o.Interactions.ScriptFunctionNames["Interact"], Is.EqualTo("oChair_Interact"));
            Assert.That(o.Properties.PropertyValues.ContainsKey("MyProperty"));
            Assert.That(o.Properties.PropertyValues["MyProperty"].Value, Is.EqualTo("aaa"));
            Assert.That(o.Properties.PropertyValues.ContainsKey("MySecondProperty"));
            Assert.That(o.Properties.PropertyValues["MySecondProperty"].Value, Is.EqualTo("bbb"));

            Assert.That(dstRoom.RegionCount, Is.EqualTo(16));  // Regions count is always fixed
            r = dstRoom.Regions[0];
            Assert.That(r.BlueTint, Is.EqualTo(10));
            Assert.That(r.GreenTint, Is.EqualTo(20));
            Assert.That(r.ID, Is.EqualTo(0));
            Assert.That(r.LightLevel, Is.EqualTo(100));
            Assert.That(r.OnStanding, Is.EqualTo("Region1_StandOn"));
            Assert.That(r.OnWalksOff, Is.EqualTo("Region1_WalkOff"));
            Assert.That(r.OnWalksOnto, Is.EqualTo("Region1_WalkOn"));
            Assert.That(r.RedTint, Is.EqualTo(40));
            Assert.That(r.TintLuminance, Is.EqualTo(50));
            Assert.That(r.TintSaturation, Is.EqualTo(60));
            Assert.That(r.UseColourTint, Is.EqualTo(true));
            Assert.That(r.Properties.PropertyValues.ContainsKey("RegionProperty"));
            Assert.That(r.Properties.PropertyValues["RegionProperty"].Value, Is.EqualTo("Region1value"));

            r = dstRoom.Regions[1];
            Assert.That(r.BlueTint, Is.EqualTo(0));
            Assert.That(r.GreenTint, Is.EqualTo(0));
            Assert.That(r.ID, Is.EqualTo(1));
            Assert.That(r.LightLevel, Is.EqualTo(80));
            Assert.That(r.OnStanding, Is.EqualTo("Region2_StandOn"));
            Assert.That(r.OnWalksOff, Is.EqualTo("Region2_WalkOff"));
            Assert.That(r.OnWalksOnto, Is.EqualTo("Region2_WalkOn"));
            Assert.That(r.RedTint, Is.EqualTo(0));
            Assert.That(r.TintLuminance, Is.EqualTo(100));
            Assert.That(r.TintSaturation, Is.EqualTo(50));
            Assert.That(r.UseColourTint, Is.EqualTo(false));
            Assert.That(r.Properties.PropertyValues.ContainsKey("RegionProperty"));
            Assert.That(r.Properties.PropertyValues["RegionProperty"].Value, Is.EqualTo("Region2value"));

            Assert.That(dstRoom.WalkableAreaCount, Is.EqualTo(16));  // Walkable areas count is always fixed
            area = dstRoom.WalkableAreas[0];
            Assert.That(area.AreaSpecificView, Is.EqualTo(4));
            Assert.That(area.FaceDirectionRatio, Is.EqualTo(1.2f));
            Assert.That(area.ID, Is.EqualTo(0));
            Assert.That(area.MaxScalingLevel, Is.EqualTo(200));
            Assert.That(area.MinScalingLevel, Is.EqualTo(50));
            Assert.That(area.ScalingLevel, Is.EqualTo(50));
            Assert.That(area.UseContinuousScaling, Is.EqualTo(true));
            Assert.That(area.Properties.PropertyValues.ContainsKey("WalkableProperty"));
            Assert.That(area.Properties.PropertyValues["WalkableProperty"].Value, Is.EqualTo("Walkable1value"));

            area = dstRoom.WalkableAreas[1];
            Assert.That(area.AreaSpecificView, Is.EqualTo(10));
            Assert.That(area.FaceDirectionRatio, Is.EqualTo(0.3f));
            Assert.That(area.ID, Is.EqualTo(1));
            Assert.That(area.MaxScalingLevel, Is.EqualTo(70));
            Assert.That(area.MinScalingLevel, Is.EqualTo(70));
            Assert.That(area.ScalingLevel, Is.EqualTo(70));
            Assert.That(area.UseContinuousScaling, Is.EqualTo(false));
            Assert.That(area.Properties.PropertyValues.ContainsKey("WalkableProperty"));
            Assert.That(area.Properties.PropertyValues["WalkableProperty"].Value, Is.EqualTo("Walkable2value"));

            Assert.That(dstRoom.WalkBehindCount, Is.EqualTo(16));  // Walk-behind count is always fixed
            wb = dstRoom.WalkBehinds[0];
            Assert.That(wb.Baseline, Is.EqualTo(77));
            Assert.That(wb.ID, Is.EqualTo(0));
            wb = dstRoom.WalkBehinds[1];
            Assert.That(wb.Baseline, Is.EqualTo(33));
            Assert.That(wb.ID, Is.EqualTo(1));
        }
    }
}
