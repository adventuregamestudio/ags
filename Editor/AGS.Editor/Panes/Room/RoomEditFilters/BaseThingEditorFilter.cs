using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// BaseThingEditorFilter is a parent filter for room layers, which manage
    /// individual "things": something that has exact coordinates, simple visual
    /// representation, and may be moved around.
    /// </summary>
    public abstract class BaseThingEditorFilter<TThing> : IRoomEditorFilter
        where TThing : class
    {
        private bool _isOn = false;
        protected Room _room;
        protected Panel _panel;
        protected RoomSettingsEditor _editor;
        private GUIController.PropertyObjectChangedHandler _propertyObjectChangedDelegate;
        protected TThing _selectedObject = null;

        public BaseThingEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room)
        {
            _room = room;
            _panel = displayPanel;
            _editor = editor;
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);

            RoomItemRefs = new SortedDictionary<string, TThing>();
            DesignItems = new SortedDictionary<string, DesignTimeProperties>();
        }

        /// <summary>
        /// A lookup table for getting game object reference by their key.
        /// This is used in connection with the room navigation UI.
        /// </summary>
        private SortedDictionary<string, TThing> RoomItemRefs { get; set; }

        #region IDisposable implementation

        public void Dispose()
        {
        }

        #endregion // IDisposable

        #region IRoomEditorFilter implementation

        public abstract string Name { get; }
        public abstract string DisplayName { get; }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public int ItemCount
        {
            get { return RoomItemRefs.Count; }
        }

        public int SelectedArea
        {
            get { return 0; } // not supported
        }

        public abstract string HelpKeyword { get; }

        public bool ShowTransparencySlider
        {
            get { return false; }
        }

        public bool SupportVisibleItems
        {
            get { return true; }
        }

        public bool Modified { get; set; }
        public bool Enabled { get { return _isOn; } }
        public bool Visible { get; set; }
        public bool Locked { get; set; }

        public SortedDictionary<string, DesignTimeProperties> DesignItems { get; protected set; }

        public void Invalidate()
        {
            _panel.Invalidate();
        }

        public void FilterOn()
        {
            SetPropertyGridList();
            Factory.GUIController.OnPropertyObjectChanged += _propertyObjectChangedDelegate;
            _isOn = true;
            FilterActivated();
        }

        public void FilterOff()
        {
            FilterDeactivated();
            Factory.GUIController.OnPropertyObjectChanged -= _propertyObjectChangedDelegate;
            _isOn = false;
        }

        public string GetItemName(string id)
        {
            TThing thing;
            if (id != null && RoomItemRefs.TryGetValue(id, out thing))
                return GetPropertyGridItemTitle(thing); // NOTE: ScriptName is not obligatory at the moment!
            return null;
        }

        public void SelectItem(string id)
        {
            if (id != null)
            {
                TThing obj;
                if (RoomItemRefs.TryGetValue(id, out obj))
                {
                    _selectedObject = obj;
                    SetPropertyGridObject(obj);
                    return;
                }
            }

            _selectedObject = null;
            SetPropertyGridObject(_room);
        }

        // Following IRoomEditorFilter members are left for descendants to implement
        public abstract void PaintToHDC(IntPtr hdc, RoomEditorState state);
        public abstract void Paint(Graphics graphics, RoomEditorState state);
        public abstract bool MouseDown(MouseEventArgs e, RoomEditorState state);
        public abstract bool MouseUp(MouseEventArgs e, RoomEditorState state);
        public abstract bool DoubleClick(RoomEditorState state);
        public abstract bool MouseMove(int x, int y, RoomEditorState state);
        public abstract void CommandClick(string command);
        public abstract bool KeyPressed(Keys keyData);
        public abstract bool KeyReleased(Keys keyData);
        public abstract Cursor GetCursor(int x, int y, RoomEditorState state);
        public abstract bool AllowClicksInterception();

        public abstract event EventHandler OnItemsChanged;
        public abstract event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;
        public abstract event EventHandler<RoomFilterContextMenuArgs> OnContextMenu;

        #endregion // IRoomEditorFilter

        /// <summary>
        /// Initializes object references for navigation UI.
        /// This method supposed to be called by the child classes on startup.
        /// </summary>
        protected void InitRoomItemRefs(SortedDictionary<string, TThing> roomItemRefs)
        {
            // Initialize item reference
            RoomItemRefs = roomItemRefs;
            // Initialize design-time properties
            DesignItems.Clear();
            foreach (var item in RoomItemRefs)
                DesignItems.Add(item.Key, new DesignTimeProperties());
        }

        protected void SetPropertyGridList()
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(_room.PropertyGridTitle, _room);
            // TODO: cache the property grid item titles?
            foreach (var item in RoomItemRefs)
            {
                list.Add(GetPropertyGridItemTitle(item.Value), item.Value);
            }
            Factory.GUIController.SetPropertyGridObjectList(list, _editor.ContentDocument, _room);
        }

        protected void SetPropertyGridObject(object obj)
        {
            Factory.GUIController.SetPropertyGridObject(obj, _editor.ContentDocument);
        }

        /// <summary>
        /// Adds object reference for navigation UI.
        /// </summary>
        protected void AddObjectRef(TThing obj)
        {
            string id = GetItemID(obj);
            if (RoomItemRefs.ContainsKey(id))
                return;
            RoomItemRefs.Add(id, obj);
            DesignItems.Add(id, new DesignTimeProperties());
        }

        /// <summary>
        /// Removes object reference, disconnect from navigation UI.
        /// </summary>
        protected void RemoveObjectRef(TThing obj)
        {
            string id = GetItemID(obj);
            RoomItemRefs.Remove(id);
            DesignItems.Remove(id);
        }

        /// <summary>
        /// Updates existing object's reference whenever its ID changes;
        /// this may be needed e.g. when another object gets deleted in the middle,
        /// or when their numeric IDs swap.
        /// </summary>
        protected void UpdateObjectRef(TThing obj, string oldID)
        {
            if (!RoomItemRefs.ContainsKey(oldID))
                return;
            string newID = GetItemID(obj);
            if (newID == oldID)
                return;

            // If the new key is also present that means we are swapping two items
            if (RoomItemRefs.ContainsKey(newID))
            {
                var obj2 = RoomItemRefs[newID];
                RoomItemRefs.Remove(newID);
                RoomItemRefs.Remove(oldID);
                RoomItemRefs.Add(newID, obj);
                RoomItemRefs.Add(oldID, obj2);
                // We must keep DesignTimeProperties!
                var obj1Item = DesignItems[oldID];
                var obj2Item = DesignItems[newID];
                DesignItems.Remove(newID);
                DesignItems.Remove(oldID);
                DesignItems.Add(newID, obj1Item);
                DesignItems.Add(oldID, obj2Item);
            }
            else
            {
                RoomItemRefs.Remove(oldID);
                RoomItemRefs.Add(newID, obj);
                // We must keep DesignTimeProperties!
                DesignItems.Add(newID, DesignItems[oldID]);
                DesignItems.Remove(oldID);
            }
        }

        #region Derived classes interface

        /// <summary>
        /// Gets this object's universal ID, used in Room Editor's navigation UI.
        /// Unfortunately, this cannot be "ScriptName" at the moment, because
        /// currently AGS does not require objects to have script names...
        /// CHECKME: later...
        /// </summary>
        protected abstract string GetItemID(TThing obj);
        /// <summary>
        /// Gets this object's script name.
        /// </summary>
        protected abstract string GetItemScriptName(TThing obj);
        /// <summary>
        /// Forms a PropertyGrid's entry title for this object.
        /// </summary>
        protected abstract string GetPropertyGridItemTitle(TThing obj);

        protected virtual void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
        }

        protected virtual void FilterActivated()
        {
        }

        protected virtual void FilterDeactivated()
        {
        }

        #endregion
    }
}
