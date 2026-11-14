using System.Collections.Generic;
using System.Linq;
using AGS.Types;
using NUnit.Framework;

namespace AGS.Editor.Panes.Room
{
    [TestFixture]
    public class RoomLayerTreeTests
    {
        private FakeRoomLayer _objects;
        private FakeRoomLayer _hotspots;
        private FakeRoomLayer _walkBehinds;
        private RoomLayerTree _tree;

        [SetUp]
        public void SetUp()
        {
            _objects = new FakeRoomLayer("Objects", "Objects", RoomAreaMaskType.None, true, "0000", "0001");
            _hotspots = new FakeRoomLayer("Hotspots", "Hotspots", RoomAreaMaskType.Hotspots, false,
                "0000", "0001", "0002", "0003");
            _walkBehinds = new FakeRoomLayer("WalkBehinds", "Walk-behinds", RoomAreaMaskType.WalkBehinds, false,
                "0000", "0001");
            _tree = new RoomLayerTree(new IRoomLayer[] { _objects, _hotspots, _walkBehinds });
        }

        private static string[] Keys(IEnumerable<RoomLayerNode> nodes)
        {
            return nodes.Select(n => n.Key).ToArray();
        }

        [Test]
        public void BuildsRootLayersAndItemsInOrder()
        {
            Assert.That(_tree.Root.Kind, Is.EqualTo(RoomLayerNodeKind.Room));
            Assert.That(_tree.Root.Key, Is.EqualTo("Room"));
            Assert.That(Keys(_tree.Root.Children), Is.EqualTo(new[] { "Objects", "Hotspots", "WalkBehinds" }));
            RoomLayerNode hotspots = _tree.Find("Hotspots");
            Assert.That(hotspots.Kind, Is.EqualTo(RoomLayerNodeKind.Layer));
            Assert.That(hotspots.Layer, Is.SameAs(_hotspots));
            Assert.That(Keys(hotspots.Children),
                Is.EqualTo(new[] { "Hotspots/0000", "Hotspots/0001", "Hotspots/0002", "Hotspots/0003" }));
            RoomLayerNode item = hotspots.Children[2];
            Assert.That(item.Kind, Is.EqualTo(RoomLayerNodeKind.Item));
            Assert.That(item.ItemID, Is.EqualTo("0002"));
            Assert.That(item.Text, Is.EqualTo("Item 0002"));
            Assert.That(item.Parent, Is.SameAs(hotspots));
        }

        [Test]
        public void LayerAndItemTextComeFromTheLayer()
        {
            Assert.That(_tree.Find("WalkBehinds").Text, Is.EqualTo("Walk-behinds"));
            _objects.ItemNames.Remove("0001");
            _tree.Rebuild();
            Assert.That(_tree.Find("Objects/0001").Text, Is.EqualTo("0001"), "an item without a name shows its ID");
        }

        [Test]
        public void UnusedItemsGoInAGroupAtTheEndOfTheirLayer()
        {
            _hotspots.ItemStates["0001"] = RoomLayerItemState.Unused;
            _hotspots.ItemStates["0003"] = RoomLayerItemState.Unused;
            _hotspots.ItemStates["0002"] = RoomLayerItemState.NothingDrawn;
            _tree.Rebuild();

            RoomLayerNode hotspots = _tree.Find("Hotspots");
            Assert.That(Keys(hotspots.Children),
                Is.EqualTo(new[] { "Hotspots/0000", "Hotspots/0002", "Hotspots/#unused" }));
            Assert.That(hotspots.Children[1].ItemState, Is.EqualTo(RoomLayerItemState.NothingDrawn));
            RoomLayerNode group = hotspots.Children[2];
            Assert.That(group.Kind, Is.EqualTo(RoomLayerNodeKind.UnusedGroup));
            Assert.That(group.Layer, Is.SameAs(_hotspots));
            Assert.That(group.Text, Is.EqualTo("Unused (2)"));
            Assert.That(Keys(group.Children), Is.EqualTo(new[] { "Hotspots/0001", "Hotspots/0003" }));
            Assert.That(group.Children.All(n => n.ItemState == RoomLayerItemState.Unused), Is.True);
        }

        [Test]
        public void NoUnusedGroupWithoutUnusedItems()
        {
            Assert.That(_tree.Find("Hotspots/#unused"), Is.Null);
            Assert.That(_tree.Find("Hotspots").Children.Any(n => n.Kind == RoomLayerNodeKind.UnusedGroup), Is.False);
        }

        [Test]
        public void ItemKeepsItsKeyWhenItLeavesTheUnusedGroupOrIsRenamed()
        {
            _hotspots.ItemStates["0003"] = RoomLayerItemState.Unused;
            _tree.Rebuild();
            Assert.That(_tree.Find("Hotspots/0003").Parent.Kind, Is.EqualTo(RoomLayerNodeKind.UnusedGroup));

            _hotspots.ItemStates.Remove("0003");
            _hotspots.ItemNames["0003"] = "hDoor (ID 3)";
            _tree.Rebuild();
            RoomLayerNode item = _tree.Find("Hotspots/0003");
            Assert.That(item.Parent.Kind, Is.EqualTo(RoomLayerNodeKind.Layer));
            Assert.That(item.Text, Is.EqualTo("hDoor (ID 3)"));
            Assert.That(_tree.Find("Hotspots/#unused"), Is.Null);
        }

        [Test]
        public void FindNearestFallsBackToTheLayerThenTheRoot()
        {
            Assert.That(_tree.FindNearest("Objects/0001").Key, Is.EqualTo("Objects/0001"));
            _objects.DesignItems.Remove("0001");
            _tree.Rebuild();
            Assert.That(_tree.FindNearest("Objects/0001").Key, Is.EqualTo("Objects"));
            Assert.That(_tree.FindNearest("Gone/0001"), Is.SameAs(_tree.Root));
            Assert.That(_tree.FindNearest("Gone"), Is.SameAs(_tree.Root));
            Assert.That(_tree.FindNearest(null), Is.SameAs(_tree.Root));
            Assert.That(_tree.Find(null), Is.Null);
        }

        [Test]
        public void FindItem()
        {
            _hotspots.ItemStates["0003"] = RoomLayerItemState.Unused;
            _tree.Rebuild();
            Assert.That(_tree.FindItem(_hotspots, "0001").Key, Is.EqualTo("Hotspots/0001"));
            Assert.That(_tree.FindItem(_hotspots, "0003").Key, Is.EqualTo("Hotspots/0003"), "in the unused group");
            Assert.That(_tree.FindItem(_objects, "0003"), Is.Null);
            Assert.That(_tree.FindItem(_objects, null), Is.Null);
        }

        [Test]
        public void GetLayerNode()
        {
            _hotspots.ItemStates["0003"] = RoomLayerItemState.Unused;
            _tree.Rebuild();
            Assert.That(_tree.GetLayerNode(_tree.Find("Hotspots/0003")).Key, Is.EqualTo("Hotspots"));
            Assert.That(_tree.GetLayerNode(_tree.Find("Hotspots/#unused")).Key, Is.EqualTo("Hotspots"));
            Assert.That(_tree.GetLayerNode(_tree.Find("Hotspots")).Key, Is.EqualTo("Hotspots"));
            Assert.That(_tree.GetLayerNode(_tree.Root), Is.Null);
        }

        [Test]
        public void TogglesFollowTheNodeKindAndTheLayer()
        {
            _hotspots.ItemStates["0003"] = RoomLayerItemState.Unused;
            _tree.Rebuild();
            Assert.That(_tree.Root.HasVisibleToggle, Is.False);
            Assert.That(_tree.Root.HasLockToggle, Is.False);
            Assert.That(_tree.Find("Hotspots").HasVisibleToggle, Is.True);
            Assert.That(_tree.Find("Hotspots").HasLockToggle, Is.True);
            Assert.That(_tree.Find("Objects/0000").HasVisibleToggle, Is.True);
            Assert.That(_tree.Find("Objects/0000").HasLockToggle, Is.True);
            Assert.That(_tree.Find("Hotspots/0001").HasVisibleToggle, Is.False, "areas cannot be hidden one by one");
            Assert.That(_tree.Find("Hotspots/0001").HasLockToggle, Is.True);
            Assert.That(_tree.Find("Hotspots/0003").HasLockToggle, Is.True, "an unused area can be locked too");
            Assert.That(_tree.Find("Hotspots/#unused").HasVisibleToggle, Is.False);
            Assert.That(_tree.Find("Hotspots/#unused").HasLockToggle, Is.False);
        }

        [Test]
        public void SetVisibleOnALayer()
        {
            RoomLayerNode objects = _tree.Find("Objects");
            Assert.That(_tree.SetVisible(objects, true), Is.True);
            Assert.That(_objects.Visible, Is.True);
            Assert.That(_objects.Modified, Is.True);
            Assert.That(_tree.IsVisible(objects), Is.True);

            _objects.Modified = false;
            Assert.That(_tree.SetVisible(objects, true), Is.False, "no change");
            Assert.That(_objects.Modified, Is.False);
        }

        [Test]
        public void ShowingAMaskLayerHidesTheOtherMaskLayersOnly()
        {
            _objects.Visible = true;
            _walkBehinds.Visible = true;
            Assert.That(_tree.SetVisible(_tree.Find("Hotspots"), true), Is.True);
            Assert.That(_hotspots.Visible, Is.True);
            Assert.That(_walkBehinds.Visible, Is.False);
            Assert.That(_walkBehinds.Modified, Is.True);
            Assert.That(_objects.Visible, Is.True, "a layer without a mask stays visible");
            Assert.That(_objects.Modified, Is.False);

            Assert.That(_tree.SetVisible(_tree.Find("Hotspots"), false), Is.True);
            Assert.That(_hotspots.Visible, Is.False);
            Assert.That(_walkBehinds.Visible, Is.False, "hiding a mask layer shows no other");
        }

        [Test]
        public void SelectionShowsAHiddenLayerUntilItLeaves()
        {
            Assert.That(_tree.ShowForSelection(_tree.Find("Objects/0001")), Is.True);
            Assert.That(_objects.Visible, Is.True);
            Assert.That(_tree.ShowForSelection(_tree.Find("Objects")), Is.False, "the same layer");
            Assert.That(_objects.Visible, Is.True);
            Assert.That(_tree.ShowForSelection(_tree.Find("Hotspots/0001")), Is.True);
            Assert.That(_objects.Visible, Is.False, "hidden again");
            Assert.That(_hotspots.Visible, Is.True);
            Assert.That(_tree.ShowForSelection(_tree.Root), Is.True);
            Assert.That(_hotspots.Visible, Is.False);
        }

        [Test]
        public void SelectionLeavesAVisibleLayerVisible()
        {
            _objects.Visible = true;
            Assert.That(_tree.ShowForSelection(_tree.Find("Objects")), Is.False);
            _tree.ShowForSelection(_tree.Find("Hotspots"));
            Assert.That(_objects.Visible, Is.True);
        }

        [Test]
        public void MaskLayersHiddenBySelectionComeBack()
        {
            _walkBehinds.Visible = true;
            _tree.ShowForSelection(_tree.Find("Hotspots"));
            Assert.That(_hotspots.Visible, Is.True);
            Assert.That(_walkBehinds.Visible, Is.False, "one mask at a time");
            Assert.That(_tree.IsVisibleByUser(_walkBehinds), Is.True);
            Assert.That(_tree.IsVisibleByUser(_hotspots), Is.False);
            _tree.ShowForSelection(_tree.Find("Objects"));
            Assert.That(_hotspots.Visible, Is.False);
            Assert.That(_walkBehinds.Visible, Is.True);
        }

        [Test]
        public void SelectingTheShownMaskLayerKeepsIt()
        {
            _walkBehinds.Visible = true;
            _tree.ShowForSelection(_tree.Find("Hotspots"));
            _tree.ShowForSelection(_tree.Find("WalkBehinds"));
            Assert.That(_hotspots.Visible, Is.False);
            Assert.That(_walkBehinds.Visible, Is.True);
            _tree.ShowForSelection(_tree.Root);
            Assert.That(_walkBehinds.Visible, Is.True, "it was the user's");
        }

        [Test]
        public void TogglingALayerTheSelectionShowedMakesItTheUsers()
        {
            _tree.ShowForSelection(_tree.Find("Objects"));
            _tree.SetVisible(_tree.Find("Objects"), false);
            _tree.SetVisible(_tree.Find("Objects"), true);
            Assert.That(_tree.IsVisibleByUser(_objects), Is.True);
            _tree.ShowForSelection(_tree.Find("Hotspots"));
            Assert.That(_objects.Visible, Is.True);
        }

        [Test]
        public void ShowingAnotherMaskLayerKeepsTheUsersChoice()
        {
            _walkBehinds.Visible = true;
            _tree.ShowForSelection(_tree.Find("Hotspots"));
            _tree.SetVisible(_tree.Find("WalkBehinds"), true); // the user shows it again
            Assert.That(_hotspots.Visible, Is.False);
            _tree.ShowForSelection(_tree.Find("Objects"));
            Assert.That(_walkBehinds.Visible, Is.True);
            Assert.That(_hotspots.Visible, Is.False);
        }

        [Test]
        public void TogglingAnotherLayerKeepsTheSelectionsLayer()
        {
            _tree.ShowForSelection(_tree.Find("Objects"));
            _tree.SetVisible(_tree.Find("Hotspots"), true);
            _tree.ShowForSelection(_tree.Root);
            Assert.That(_objects.Visible, Is.False);
            Assert.That(_hotspots.Visible, Is.True);
        }

        [Test]
        public void SetVisibleOnAnItem()
        {
            RoomLayerNode item = _tree.Find("Objects/0001");
            Assert.That(_tree.IsVisible(item), Is.True);
            Assert.That(_tree.SetVisible(item, false), Is.True);
            Assert.That(_objects.DesignItems["0001"].Visible, Is.False);
            Assert.That(_objects.DesignItems["0000"].Visible, Is.True);
            Assert.That(_objects.Modified, Is.True);
            Assert.That(_tree.IsVisible(item), Is.False);
        }

        [Test]
        public void SetVisibleWithoutAToggleChangesNothing()
        {
            Assert.That(_tree.SetVisible(_tree.Find("Hotspots/0001"), false), Is.False);
            Assert.That(_hotspots.DesignItems["0001"].Visible, Is.True);
            Assert.That(_tree.SetVisible(_tree.Root, false), Is.False);
            Assert.That(_hotspots.Modified, Is.False);
        }

        [Test]
        public void SetLockedOnALayerAndAnItem()
        {
            Assert.That(_tree.SetLocked(_tree.Find("Hotspots"), true), Is.True);
            Assert.That(_hotspots.Locked, Is.True);
            Assert.That(_tree.IsLocked(_tree.Find("Hotspots")), Is.True);

            _hotspots.Modified = false;
            RoomLayerNode item = _tree.Find("Hotspots/0002");
            Assert.That(_tree.SetLocked(item, true), Is.True);
            Assert.That(_hotspots.DesignItems["0002"].Locked, Is.True);
            Assert.That(_hotspots.Modified, Is.True);
            Assert.That(_tree.IsLocked(item), Is.True);
            Assert.That(_tree.SetLocked(item, true), Is.False, "no change");
            Assert.That(_tree.SetLocked(_tree.Root, true), Is.False);
        }

        [Test]
        public void StateOfARemovedItemIsNotWritten()
        {
            RoomLayerNode item = _tree.Find("Objects/0001");
            _objects.DesignItems.Remove("0001");
            Assert.That(_tree.SetVisible(item, false), Is.False);
            Assert.That(_tree.SetLocked(item, true), Is.False);
            Assert.That(_objects.Modified, Is.False);
        }

        [Test]
        public void PathUsesTheLayerDisplayNameAndTheItemID()
        {
            _hotspots.ItemStates["0003"] = RoomLayerItemState.Unused;
            _tree.Rebuild();
            Assert.That(_tree.GetPath(_tree.Root), Is.EqualTo("Room"));
            Assert.That(_tree.GetPath(_tree.Find("WalkBehinds")), Is.EqualTo("Room/Walk-behinds"));
            Assert.That(_tree.GetPath(_tree.Find("WalkBehinds/0001")), Is.EqualTo("Room/Walk-behinds/0001"));
            Assert.That(_tree.GetPath(_tree.Find("Hotspots/0003")), Is.EqualTo("Room/Hotspots/0003"));
            Assert.That(_tree.GetPath(_tree.Find("Hotspots/#unused")), Is.EqualTo("Room/Hotspots"));
        }

        [Test]
        public void FindByPathRoundTripsEveryNode()
        {
            _hotspots.ItemStates["0003"] = RoomLayerItemState.Unused;
            _tree.Rebuild();
            var stack = new Stack<RoomLayerNode>();
            stack.Push(_tree.Root);
            while (stack.Count > 0)
            {
                RoomLayerNode node = stack.Pop();
                foreach (RoomLayerNode child in node.Children)
                    stack.Push(child);
                RoomLayerNode expected = node.Kind == RoomLayerNodeKind.UnusedGroup ? node.Parent : node;
                Assert.That(_tree.FindByPath(_tree.GetPath(node)), Is.SameAs(expected), node.Key);
            }
        }

        [Test]
        public void FindByPathReturnsTheDeepestNodeFound()
        {
            Assert.That(_tree.FindByPath("Room/WalkBehinds/0001").Key, Is.EqualTo("WalkBehinds/0001"),
                "a layer matches by name too");
            Assert.That(_tree.FindByPath("Room/Walk-behinds/0042").Key, Is.EqualTo("WalkBehinds"));
            Assert.That(_tree.FindByPath("Room/Nothing/0001"), Is.SameAs(_tree.Root));
            Assert.That(_tree.FindByPath("Hotspots/0001"), Is.Null, "not from the root");
            Assert.That(_tree.FindByPath(""), Is.Null);
            Assert.That(_tree.FindByPath(null), Is.Null);
        }
    }
}
