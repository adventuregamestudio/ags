using System;
using System.Collections.Generic;
using AGS.Types;

namespace AGS.Editor.Panes.Room
{
    public enum RoomLayerNodeKind
    {
        Room,
        Layer,
        Item,
        UnusedGroup
    }

    /// <summary>
    /// A node of the room editor's layer tree. Visibility and lock are kept by the layer.
    /// </summary>
    public sealed class RoomLayerNode
    {
        private readonly List<RoomLayerNode> _children = new List<RoomLayerNode>();

        internal RoomLayerNode(RoomLayerNodeKind kind, string key, string text, IRoomLayer layer,
            string itemID, RoomLayerItemState itemState, RoomLayerNode parent)
        {
            Kind = kind;
            Key = key;
            Text = text;
            Layer = layer;
            ItemID = itemID;
            ItemState = itemState;
            Parent = parent;
            parent?._children.Add(this);
        }

        public RoomLayerNodeKind Kind { get; }
        /// <summary>
        /// The layer's Name, plus the item ID for an item; the same across rebuilds.
        /// </summary>
        public string Key { get; }
        public string Text { get; }
        public IRoomLayer Layer { get; }
        public string ItemID { get; }
        public RoomLayerItemState ItemState { get; }
        public RoomLayerNode Parent { get; }
        public IReadOnlyList<RoomLayerNode> Children { get { return _children; } }

        public bool HasVisibleToggle
        {
            get
            {
                return Kind == RoomLayerNodeKind.Layer ||
                    (Kind == RoomLayerNodeKind.Item && Layer.SupportVisibleItems);
            }
        }

        public bool HasLockToggle
        {
            get { return Kind == RoomLayerNodeKind.Layer || Kind == RoomLayerNodeKind.Item; }
        }

        public override string ToString()
        {
            return Text;
        }
    }

    /// <summary>
    /// The room editor's layer tree without UI: the room, its layers and their items,
    /// their visibility and lock, and the rule that one layer with a mask is visible at a time.
    /// </summary>
    public sealed class RoomLayerTree
    {
        public const string ROOT_KEY = "Room";
        private const char KEY_SEPARATOR = '/';
        private const string UNUSED_GROUP_ID = "#unused";

        private readonly List<IRoomLayer> _layers;
        private readonly Dictionary<string, RoomLayerNode> _nodes = new Dictionary<string, RoomLayerNode>();
        // A hidden layer shown while it is selected, and the layers with a mask it hid
        private IRoomLayer _shownForSelection;
        private readonly List<IRoomLayer> _hiddenForSelection = new List<IRoomLayer>();

        public RoomLayerTree(IEnumerable<IRoomLayer> layers)
        {
            _layers = new List<IRoomLayer>(layers);
            Rebuild();
        }

        public RoomLayerNode Root { get; private set; }

        /// <summary>
        /// Builds the nodes again. Nodes of an earlier build are not updated: find them again by key.
        /// </summary>
        public void Rebuild()
        {
            _nodes.Clear();
            Root = Add(new RoomLayerNode(RoomLayerNodeKind.Room, ROOT_KEY, ROOT_KEY, null, null,
                RoomLayerItemState.Normal, null));
            foreach (IRoomLayer layer in _layers)
            {
                RoomLayerNode layerNode = Add(new RoomLayerNode(RoomLayerNodeKind.Layer, layer.Name,
                    layer.DisplayName, layer, null, RoomLayerItemState.Normal, Root));
                List<string> unused = new List<string>();
                foreach (string id in layer.DesignItems.Keys)
                {
                    RoomLayerItemState state = layer.GetItemState(id);
                    if (state == RoomLayerItemState.Unused)
                        unused.Add(id);
                    else
                        AddItem(layer, id, state, layerNode);
                }
                if (unused.Count == 0)
                    continue;
                RoomLayerNode group = Add(new RoomLayerNode(RoomLayerNodeKind.UnusedGroup,
                    MakeKey(layer, UNUSED_GROUP_ID), UnusedGroupText(unused.Count), layer, null,
                    RoomLayerItemState.Normal, layerNode));
                foreach (string id in unused)
                    AddItem(layer, id, RoomLayerItemState.Unused, group);
            }
        }

        public static string UnusedGroupText(int count)
        {
            return string.Format("Unused ({0})", count);
        }

        public RoomLayerNode Find(string key)
        {
            RoomLayerNode node;
            if (key != null && _nodes.TryGetValue(key, out node))
                return node;
            return null;
        }

        public RoomLayerNode FindItem(IRoomLayer layer, string itemID)
        {
            return itemID == null ? null : Find(MakeKey(layer, itemID));
        }

        /// <summary>
        /// Finds the node with this key, else its layer's node, else the root.
        /// </summary>
        public RoomLayerNode FindNearest(string key)
        {
            RoomLayerNode node = Find(key);
            if (node != null)
                return node;
            if (key != null)
            {
                int separator = key.IndexOf(KEY_SEPARATOR);
                if (separator > 0)
                    node = Find(key.Substring(0, separator));
            }
            return node ?? Root;
        }

        public RoomLayerNode GetLayerNode(RoomLayerNode node)
        {
            return node.Layer == null ? null : Find(node.Layer.Name);
        }

        public bool IsVisible(RoomLayerNode node)
        {
            if (node.Kind == RoomLayerNodeKind.Layer)
                return node.Layer.Visible;
            DesignTimeProperties props = GetItemProperties(node);
            return props == null || props.Visible;
        }

        public bool IsLocked(RoomLayerNode node)
        {
            if (node.Kind == RoomLayerNodeKind.Layer)
                return node.Layer.Locked;
            DesignTimeProperties props = GetItemProperties(node);
            return props != null && props.Locked;
        }

        /// <summary>
        /// Shows or hides a layer or item by the user's choice. Showing a layer with a mask
        /// hides the other layers with a mask. Returns whether anything changed.
        /// </summary>
        public bool SetVisible(RoomLayerNode node, bool visible)
        {
            if (!node.HasVisibleToggle || IsVisible(node) == visible)
                return false;
            if (node.Kind == RoomLayerNodeKind.Layer)
            {
                // A layer the selection showed or hid is the user's from now on
                if (node.Layer == _shownForSelection || _hiddenForSelection.Contains(node.Layer) ||
                    (visible && node.Layer.MaskToDraw != RoomAreaMaskType.None &&
                     _shownForSelection != null && _shownForSelection.MaskToDraw != RoomAreaMaskType.None))
                {
                    _shownForSelection = null;
                    _hiddenForSelection.Clear();
                }
                SetLayerVisible(node.Layer, visible, null);
                return true;
            }
            DesignTimeProperties props = GetItemProperties(node);
            if (props == null)
                return false;
            props.Visible = visible;
            node.Layer.Modified = true;
            return true;
        }

        /// <summary>
        /// Shows a hidden layer while it is selected. When the selection leaves it, it is hidden
        /// again and the layers it hid are shown again, unless the user toggled them.
        /// Returns whether a layer's visibility changed.
        /// </summary>
        public bool ShowForSelection(RoomLayerNode node)
        {
            IRoomLayer layer = node.Layer;
            bool changed = false;
            if (_shownForSelection != null && _shownForSelection != layer)
            {
                SetLayerVisible(_shownForSelection, false, null);
                foreach (IRoomLayer hidden in _hiddenForSelection)
                    SetLayerVisible(hidden, true, null);
                _shownForSelection = null;
                _hiddenForSelection.Clear();
                changed = true;
            }
            if (layer != null && !layer.Visible)
            {
                _hiddenForSelection.Clear();
                SetLayerVisible(layer, true, _hiddenForSelection);
                _shownForSelection = layer;
                changed = true;
            }
            return changed;
        }

        /// <summary>
        /// Tells whether a layer is visible by the user's choice, ignoring what the selection
        /// showed or hid; this is what the room's user file keeps.
        /// </summary>
        public bool IsVisibleByUser(IRoomLayer layer)
        {
            if (layer == _shownForSelection)
                return false;
            return layer.Visible || _hiddenForSelection.Contains(layer);
        }

        private void SetLayerVisible(IRoomLayer layer, bool visible, List<IRoomLayer> hiddenMasks)
        {
            if (layer.Visible != visible)
            {
                layer.Visible = visible;
                layer.Modified = true;
            }
            if (!visible || layer.MaskToDraw == RoomAreaMaskType.None)
                return;
            foreach (IRoomLayer other in _layers)
            {
                if (other == layer || other.MaskToDraw == RoomAreaMaskType.None || !other.Visible)
                    continue;
                other.Visible = false;
                other.Modified = true;
                hiddenMasks?.Add(other);
            }
        }

        public bool SetLocked(RoomLayerNode node, bool locked)
        {
            if (!node.HasLockToggle || IsLocked(node) == locked)
                return false;
            if (node.Kind == RoomLayerNodeKind.Layer)
            {
                node.Layer.Locked = locked;
            }
            else
            {
                DesignTimeProperties props = GetItemProperties(node);
                if (props == null)
                    return false;
                props.Locked = locked;
            }
            node.Layer.Modified = true;
            return true;
        }

        /// <summary>
        /// Gets the selection path the room's user file keeps: "Room[/layer display name[/item ID]]".
        /// </summary>
        public string GetPath(RoomLayerNode node)
        {
            if (node.Layer == null)
                return ROOT_KEY;
            string path = ROOT_KEY + KEY_SEPARATOR + node.Layer.DisplayName;
            if (node.Kind == RoomLayerNodeKind.Item)
                path += KEY_SEPARATOR + node.ItemID;
            return path;
        }

        /// <summary>
        /// Finds the deepest node of a path from GetPath or from the older breadcrumb bar,
        /// matching a layer by display name or name; null if the path does not start at the room.
        /// </summary>
        public RoomLayerNode FindByPath(string path)
        {
            if (string.IsNullOrEmpty(path))
                return null;
            string[] parts = path.Split(KEY_SEPARATOR);
            if (parts[0] != ROOT_KEY)
                return null;
            if (parts.Length < 2)
                return Root;
            IRoomLayer layer = _layers.Find(l => l.DisplayName == parts[1]) ??
                _layers.Find(l => l.Name == parts[1]);
            if (layer == null)
                return Root;
            RoomLayerNode layerNode = Find(layer.Name);
            if (parts.Length < 3)
                return layerNode;
            return Find(MakeKey(layer, parts[2])) ?? layerNode;
        }

        private RoomLayerNode Add(RoomLayerNode node)
        {
            _nodes.Add(node.Key, node);
            return node;
        }

        private void AddItem(IRoomLayer layer, string id, RoomLayerItemState state, RoomLayerNode parent)
        {
            Add(new RoomLayerNode(RoomLayerNodeKind.Item, MakeKey(layer, id), layer.GetItemName(id) ?? id,
                layer, id, state, parent));
        }

        private static string MakeKey(IRoomLayer layer, string id)
        {
            return layer.Name + KEY_SEPARATOR + id;
        }

        private static DesignTimeProperties GetItemProperties(RoomLayerNode node)
        {
            if (node.Kind != RoomLayerNodeKind.Item)
                return null;
            DesignTimeProperties props;
            node.Layer.DesignItems.TryGetValue(node.ItemID, out props);
            return props;
        }
    }
}
