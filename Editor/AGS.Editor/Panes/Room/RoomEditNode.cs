using System;
using System.Collections.Generic;
using System.Text;
using AddressBarExt;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor.Panes.Room
{
    public class RoomEditNode : IAddressNode
    {
        private RoomNodeControl _control;
        private bool _initVisible, _initLocked, _shouldHideCheckboxes;

        /// <summary>
        /// Constructor creates a node, associated with a namespace rather than particular room item.
        /// </summary>
        /// <param name="uniqueID"></param>
        /// <param name="children"></param>
        /// <param name="visibleByDefault"></param>
        public RoomEditNode(string uniqueID, IAddressNode[] children, bool visible, bool locked)
            :this(uniqueID, uniqueID, null, children, visible, locked, false)
        { 
        }

        /// <summary>
        /// Constructor creates a node, associated with particular room item.
        /// </summary>
        /// <param name="uniqueID"></param>
        /// <param name="displayName"></param>
        /// <param name="roomItemID"></param>
        /// <param name="children"></param>
        /// <param name="visibleByDefault"></param>
        /// <param name="shouldHideCheckboxes"></param>
        public RoomEditNode(string uniqueID, string displayName, string roomItemID, IAddressNode[] children,
            bool visible, bool locked, bool shouldHideCheckboxes)
        {
            UniqueID = uniqueID;
            DisplayName = displayName;
            RoomItemID = roomItemID;
            Children = children;
            _initVisible = visible;
            _initLocked = locked;
            _shouldHideCheckboxes = shouldHideCheckboxes;
        }

        public IAddressNode Parent { get; set; }

        public string DisplayName { get; private set; }

        public Icon Icon { get { return null; } }

        public object UniqueID { get; private set; }

        /// <summary>
        /// Gets the ID of the room item, associated with this node.
        /// </summary>
        public string RoomItemID { get { return Tag as string; } private set { Tag = value; } }

        public object Tag { get; set; }

        public IAddressNode[] Children { get; private set; }

        public IRoomEditorFilter Layer { get; set; }

        public bool IsVisible 
        { 
            get { return _control == null ? true : _control.IsVisible; }
            set { if (_control != null) _control.IsVisible = true; }
        }

        public bool IsLocked { get { return _control == null ? false : _control.IsLocked; } }

        public VisibleLayerRadioGroup VisibleGroup { get; set; }

        public void UpdateNode()
        {            
        }

        public IAddressNode GetChild(object uniqueID, bool recursive)
        {
            foreach (IAddressNode child in Children)
            {
                if (child.UniqueID.Equals(uniqueID)) return child;
                if (recursive)
                {
                    IAddressNode found = child.GetChild(uniqueID, recursive);
                    if (found != null) return found;
                }
            }
            return null;
        }

        public ToolStripItem CreateNodeUI(EventHandler nodeClicked)
        {
            RoomNodeControl control = new RoomNodeControl 
            { 
                DisplayName = DisplayName, 
                IsVisible = _initVisible,
                IsLocked = _initLocked
            };
            if (_shouldHideCheckboxes) control.HideCheckBoxes(true);
            if (_control != null)
            {
                _control.OnNodeSelected -= nodeClicked;
                _control.OnIsVisibleChanged -= control_OnVisibleChanged;
                _control.OnIsLockedChanged -= control_OnLockedChanged;
            }
            control.OnNodeSelected += nodeClicked;
            control.OnIsVisibleChanged += control_OnVisibleChanged;
            control.OnIsLockedChanged += control_OnLockedChanged;
            ToolStripControlHost host = new ToolStripControlHost(control);
            control.Host = host;
            _control = control;
            if (VisibleGroup != null) VisibleGroup.Register(_control);
            if (Layer == null)
            {
                IRoomEditorFilter parentFilter = FindFilter();
                if (parentFilter != null && !parentFilter.SupportVisibleItems) control.HideCheckBoxes(false);
            }
            return host;
        }

        private void control_OnVisibleChanged(object sender, EventArgs e)
        {
            IRoomEditorFilter parentFilter = FindFilter();
            if (parentFilter != null)
            {
                if (Layer == null)
                    parentFilter.DesignItems[RoomItemID].Visible = _control.IsVisible;
                else
                    Layer.Visible = _control.IsVisible;
                parentFilter.Modified = true;
                parentFilter.Invalidate(); // repaint, since visibility changed
            }
        }

        private void control_OnLockedChanged(object sender, EventArgs e)
        {
            IRoomEditorFilter parentFilter = FindFilter();
            if (parentFilter != null)
            {
                if (Layer == null)
                    parentFilter.DesignItems[RoomItemID].Locked = _control.IsLocked;
                else
                    Layer.Locked = _control.IsLocked;
                parentFilter.Modified = true;
            }
        }

        private IRoomEditorFilter FindFilter()
        {
            RoomEditNode node = this;
            while (node != null)
            {
                if (node.Layer != null) return node.Layer;
                node = node.Parent as RoomEditNode;
            }
            return null;
        }

        public override string ToString()
        {
            return DisplayName;
        }

        public override bool Equals(object obj)
        {
            if (obj == null) return false;
            RoomEditNode other = obj as RoomEditNode;
            if (other == null) return false;
            return UniqueID.Equals(other.UniqueID);
        }

        public override int GetHashCode()
        {
            return UniqueID.GetHashCode();
        }
    }
}
