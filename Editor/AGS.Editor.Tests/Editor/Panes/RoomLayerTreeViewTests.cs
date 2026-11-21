using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;
using AGS.Types;
using NUnit.Framework;

namespace AGS.Editor.Panes.Room
{
    [TestFixture]
    [Apartment(ApartmentState.STA)]
    [Timeout(10000)] // a click the view passes on could wait in the native drag detection
    public class RoomLayerTreeViewTests
    {
        private const int WM_KEYDOWN = 0x0100;
        private const int WM_LBUTTONDOWN = 0x0201;
        private const int WM_LBUTTONUP = 0x0202;
        private const int VK_RETURN = 0x0D;
        // Inside the eye and the lock columns, from the right edge
        private const int EYE_X_FROM_RIGHT = 27;
        private const int LOCK_X_FROM_RIGHT = 9;

        private FakeRoomLayer _objects;
        private FakeRoomLayer _hotspots;
        private FakeRoomLayer _regions;
        private RoomLayerTree _tree;
        private RoomLayerTreeView _view;
        private List<string> _selected;
        private List<string> _toggled;

        [SetUp]
        public void SetUp()
        {
            _objects = new FakeRoomLayer("Objects", "Objects", RoomAreaMaskType.None, true, "0000", "0001");
            _hotspots = new FakeRoomLayer("Hotspots", "Hotspots", RoomAreaMaskType.Hotspots, false,
                "0000", "0001", "0002");
            _hotspots.ItemStates["0002"] = RoomLayerItemState.Unused;
            _regions = new FakeRoomLayer("Regions", "Regions", RoomAreaMaskType.Regions, false, "0000", "0001");
            _tree = new RoomLayerTree(new IRoomLayer[] { _objects, _hotspots, _regions });
            _view = new RoomLayerTreeView { Size = new Size(300, 400) };
            Assert.That(_view.Handle, Is.Not.EqualTo(IntPtr.Zero));
            _view.Tree = _tree;
            _selected = new List<string>();
            _toggled = new List<string>();
            _view.NodeSelected += (s, e) => _selected.Add(e.Node.Key);
            _view.NodeToggled += (s, e) => _toggled.Add(e.Node.Key);
        }

        [TearDown]
        public void TearDown()
        {
            _view.Dispose();
        }

        private TreeNode NodeOf(string key)
        {
            TreeNode[] found = _view.Nodes.Find(key, true);
            Assert.That(found.Length, Is.EqualTo(1), key);
            return found[0];
        }

        private void Click(string key, int xFromRight)
        {
            Rectangle row = NodeOf(key).Bounds;
            Assert.That(row.Height, Is.GreaterThan(0), key + " is shown");
            int x = _view.ClientSize.Width - xFromRight;
            int y = row.Top + row.Height / 2;
            // If the view passes the click on, the native tree finds the button released
            PostMessage(_view.Handle, WM_LBUTTONUP, IntPtr.Zero, (IntPtr)((y << 16) | x));
            SendMessage(_view.Handle, WM_LBUTTONDOWN, (IntPtr)1, (IntPtr)((y << 16) | x));
            Application.DoEvents();
        }

        [Test]
        public void ShowsTheRoomAndItsLayersAtTheTopWithTheRoomSelected()
        {
            Assert.That(_view.Nodes.Count, Is.EqualTo(4));
            TreeNode room = _view.Nodes[0];
            Assert.That(room.Text, Is.EqualTo("Room"));
            Assert.That(room.Nodes.Count, Is.EqualTo(0), "the room is a leaf");
            Assert.That(_view.Nodes[1].Name, Is.EqualTo("Objects"));
            Assert.That(_view.Nodes[3].Name, Is.EqualTo("Regions"));
            Assert.That(_view.Nodes[1].IsExpanded, Is.False, "layers start collapsed");
            Assert.That(_view.SelectedLayerNode.Key, Is.EqualTo("Room"));
            Assert.That(NodeOf("Hotspots/#unused").Text, Is.EqualTo("Unused (1)"));
            Assert.That(NodeOf("Hotspots/#unused").Parent, Is.SameAs(NodeOf("Hotspots")));
            Assert.That(NodeOf("Hotspots/0002").Parent, Is.SameAs(NodeOf("Hotspots/#unused")));
            Assert.That(_selected, Is.Empty, "showing the tree is no user selection");
        }

        [Test]
        public void EyeClickShowsAMaskLayerAndHidesTheOtherWithoutSelecting()
        {
            _regions.Visible = true;
            Click("Hotspots", EYE_X_FROM_RIGHT);
            Assert.That(_hotspots.Visible, Is.True);
            Assert.That(_regions.Visible, Is.False);
            Assert.That(_toggled, Is.EqualTo(new[] { "Hotspots" }));
            Assert.That(_selected, Is.Empty);
            Assert.That(_view.SelectedLayerNode.Key, Is.EqualTo("Room"));
            Assert.That(NodeOf("Hotspots").IsExpanded, Is.False, "the click does not expand");
        }

        [Test]
        public void LockClickOnAnItem()
        {
            NodeOf("Objects").Expand();
            Click("Objects/0001", LOCK_X_FROM_RIGHT);
            Assert.That(_objects.DesignItems["0001"].Locked, Is.True);
            Assert.That(_objects.DesignItems["0000"].Locked, Is.False);
            Assert.That(_toggled, Is.EqualTo(new[] { "Objects/0001" }));
            Click("Objects/0001", LOCK_X_FROM_RIGHT);
            Assert.That(_objects.DesignItems["0001"].Locked, Is.False);
        }

        [Test]
        public void EyeClickOnAnItemOfAnObjectLayer()
        {
            NodeOf("Objects").Expand();
            Click("Objects/0000", EYE_X_FROM_RIGHT);
            Assert.That(_objects.DesignItems["0000"].Visible, Is.False);
            Assert.That(_objects.Modified, Is.True);
        }

        [Test]
        public void AreaItemsHaveNoEye()
        {
            NodeOf("Hotspots").Expand();
            Click("Hotspots/0001", EYE_X_FROM_RIGHT);
            Assert.That(_hotspots.DesignItems["0001"].Visible, Is.True);
            Assert.That(_toggled, Is.Empty);
        }

        [Test]
        public void TheRootHasNoToggles()
        {
            Click("Room", EYE_X_FROM_RIGHT);
            Click("Room", LOCK_X_FROM_RIGHT);
            Assert.That(_toggled, Is.Empty);
        }

        [Test]
        public void SelectingANodeRaisesNodeSelected()
        {
            _view.SelectedNode = NodeOf("Regions");
            Assert.That(_selected, Is.EqualTo(new[] { "Regions" }));
        }

        [Test]
        public void SelectKeyRaisesNothing()
        {
            _view.SelectKey("Hotspots/0002");
            Assert.That(_view.SelectedLayerNode.Key, Is.EqualTo("Hotspots/0002"));
            Assert.That(_selected, Is.Empty);
        }

        [Test]
        public void ReloadKeepsTheSelectionAndTheExpandedNodes()
        {
            _view.SelectKey("Objects/0001");
            _objects.ItemNames["0001"] = "oDoor (ID 1)";
            _tree.Rebuild();
            RoomLayerNode selected = _view.Reload();
            Assert.That(selected.Key, Is.EqualTo("Objects/0001"));
            Assert.That(NodeOf("Objects").IsExpanded, Is.True);
            Assert.That(NodeOf("Objects/0001").Text, Is.EqualTo("oDoor (ID 1)"));
            Assert.That(_selected, Is.Empty);
        }

        [Test]
        public void ReloadShowsNewStatesAndModelNodes()
        {
            _hotspots.ItemStates["0001"] = RoomLayerItemState.NothingDrawn;
            _hotspots.ItemStates["0000"] = RoomLayerItemState.Unused;
            _tree.Rebuild();
            _view.Reload();
            TreeNode area = NodeOf("Hotspots/0001");
            Assert.That(area.Tag, Is.SameAs(_tree.Find("Hotspots/0001")), "the new model node");
            Assert.That(area.ToolTipText, Does.StartWith("Nothing drawn"));
            Assert.That(NodeOf("Hotspots/0000").Parent, Is.SameAs(NodeOf("Hotspots/#unused")));
            Assert.That(NodeOf("Hotspots/#unused").Text, Is.EqualTo("Unused (2)"));
        }

        [Test]
        public void ReloadSelectsTheLayerOfARemovedItem()
        {
            _view.SelectKey("Objects/0001");
            _objects.DesignItems.Remove("0001");
            _tree.Rebuild();
            Assert.That(_view.Reload().Key, Is.EqualTo("Objects"));
        }

        [Test]
        public void ReloadFollowsAnItemOutOfTheUnusedGroup()
        {
            _view.SelectKey("Hotspots/0002");
            _hotspots.ItemStates.Remove("0002");
            _tree.Rebuild();
            Assert.That(_view.Reload().Key, Is.EqualTo("Hotspots/0002"));
            Assert.That(NodeOf("Hotspots/0002").Parent, Is.SameAs(NodeOf("Hotspots")));
            Assert.That(_view.Nodes.Find("Hotspots/#unused", true), Is.Empty);
        }

        [Test]
        public void ExpandAndCollapse()
        {
            NodeOf("Hotspots").Expand();
            Assert.That(NodeOf("Hotspots").IsExpanded, Is.True);
            NodeOf("Hotspots").Collapse();
            Assert.That(NodeOf("Hotspots").IsExpanded, Is.False);
            _view.SelectKey("Hotspots/0002"); // expands two levels
            Assert.That(NodeOf("Hotspots/#unused").IsExpanded, Is.True);
        }

        [Test]
        public void EnterAsksToLeave()
        {
            int asked = 0;
            _view.LeaveRequested += (s, e) => ++asked;
            SendMessage(_view.Handle, WM_KEYDOWN, (IntPtr)VK_RETURN, IntPtr.Zero);
            Assert.That(asked, Is.EqualTo(1));
        }

        [DllImport("user32.dll")]
        private static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll")]
        private static extern bool PostMessage(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);
    }
}
