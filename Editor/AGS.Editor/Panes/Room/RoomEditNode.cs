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
        private bool _visibleByDefault, _shouldHideCheckboxes;

        public RoomEditNode(string uniqueID, IAddressNode[] children, bool visibleByDefault)
            :this(uniqueID, uniqueID, children, visibleByDefault, false)
        { 
        }

        public RoomEditNode(string uniqueID, string displayName, IAddressNode[] children, bool visibleByDefault,
            bool shouldHideCheckboxes)
        {
            UniqueID = uniqueID;
            DisplayName = displayName;
            Children = children;
            _visibleByDefault = visibleByDefault;
            _shouldHideCheckboxes = shouldHideCheckboxes;
        }

        public IAddressNode Parent { get; set; }

        public string DisplayName { get; private set; }

        public Icon Icon { get { return null; } }

        public object UniqueID { get; private set; }

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
                IsVisible = _visibleByDefault
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
            else
            {
                Layer.VisibleItems.Clear();
                Layer.VisibleItems.AddRange(Layer.GetItemsNames());
            }
            return host;
            //return new ToolStripMenuItem(DisplayName, null, nodeClicked);
        }

        private void control_OnVisibleChanged(object sender, EventArgs e)
        {
            IRoomEditorFilter parentFilter = FindFilter();
            if (parentFilter != null)
            {
                if (Layer == null) UpdateList(parentFilter.VisibleItems, DisplayName, _control.IsVisible);
                parentFilter.Invalidate();
            }
        }

        private void control_OnLockedChanged(object sender, EventArgs e)
        {
            if (Layer != null) return;
            IRoomEditorFilter parentFilter = FindFilter();
            if (parentFilter != null) UpdateList(parentFilter.LockedItems, DisplayName, _control.IsLocked);            
        }

        private void UpdateList(List<string> list, string name, bool add)
        {
            if (add)
            {
                if (list.Contains(name)) return;
                list.Add(name);
            }
            else list.Remove(name);
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
