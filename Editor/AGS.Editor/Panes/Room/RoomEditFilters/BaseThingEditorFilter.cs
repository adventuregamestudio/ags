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
        protected TThing _lastSelectedObject = null;
        // Object moving handling
        private bool _movingObjectWithMouse;
        // mouse offset in ROOM's coordinates
        private int _mouseOffsetX, _mouseOffsetY;
        // mouse click location in ROOM's coordinates
        private Point _menuClickPos = new Point();
        private bool _movingObjectWithKeyboard = false;
        private int _movingKeysDown = 0;
        private Timer _movingHintTimer = new Timer();

        public BaseThingEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room)
        {
            _room = room;
            _panel = displayPanel;
            _editor = editor;
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);

            RoomItemRefs = new SortedDictionary<string, TThing>();
            DesignItems = new SortedDictionary<string, DesignTimeProperties>();

            _movingHintTimer.Interval = 2000;
            _movingHintTimer.Tick += MovingHintTimer_Tick;
        }

        /// <summary>
        /// A lookup table for getting game object reference by their key.
        /// This is used in connection with the room navigation UI.
        /// </summary>
        private SortedDictionary<string, TThing> RoomItemRefs { get; set; }

        /// <summary>
        /// Tells whether the selected object is being moved, either with
        /// the mouse or keyboard (or else).
        /// </summary>
        protected bool IsMovingObject
        {
            get { return _movingObjectWithMouse || _movingObjectWithKeyboard; }
        }

        protected Point MenuClickPos
        {
            get { return _menuClickPos; }
        }

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
            ClearMovingState();
            FilterActivated();
        }

        public void FilterOff()
        {
            Factory.GUIController.OnPropertyObjectChanged -= _propertyObjectChangedDelegate;
            _isOn = false;
            ClearMovingState();
            FilterDeactivated();
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

        public bool KeyPressed(Keys key)
        {
            if (_selectedObject == null)
                return false;
            if (DesignItems[GetItemID(_selectedObject)].Locked)
                return false;

            if (HandleKeyPress(key))
                return true;
            return HandleMoveKeysPress(key);
        }

        public bool KeyReleased(Keys key)
        {
            if (HandleKeyRelease(key))
                return true;
            return HandleMoveKeysRelease(key);
        }

        public bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            if (e.Button == MouseButtons.Middle)
                return false;

            int roomX = state.WindowXToRoom(e.X);
            int roomY = state.WindowYToRoom(e.Y);

            TThing thing = GetObjectAtCoords(roomX, roomY, state);
            if (thing != null)
            {
                SelectObject(thing, roomX, roomY, state);
            }
            else
            {
                _selectedObject = null;
            }

            if (_selectedObject != null)
            {
                Factory.GUIController.SetPropertyGridObject(_selectedObject);
                return true;
            }
            return false;
        }

        public bool MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            if (e.Button == MouseButtons.Middle)
                return false;

            _movingObjectWithMouse = false;
            _lastSelectedObject = _selectedObject;

            // Upon releasing RMB - display a context menu
            if (e.Button == MouseButtons.Right)
            {
                _menuClickPos.X = state.WindowXToRoom(e.X);
                _menuClickPos.Y = state.WindowYToRoom(e.Y);
                ShowContextMenu(e, state);
                return true;
            }
            return false;
        }

        public bool MouseMove(int x, int y, RoomEditorState state)
        {
            if (!_movingObjectWithMouse)
                return false;

            int newX = state.WindowXToRoom(x) - _mouseOffsetX;
            int newY = state.WindowYToRoom(y) - _mouseOffsetY;
            return MoveObject(newX, newY);
        }

        public Cursor GetCursor(int x, int y, RoomEditorState state)
        {
            if (_movingObjectWithMouse)
                return Cursors.Hand;
            x = state.WindowXToRoom(x);
            y = state.WindowYToRoom(y);
            if (GetObjectAtCoords(x, y, state) != null)
                return Cursors.Default;
            return null;
        }

        // Following IRoomEditorFilter members are left for descendants to implement
        public abstract void Paint(Graphics graphics, RoomEditorState state);
        public abstract bool DoubleClick(RoomEditorState state);
        public abstract void CommandClick(string command);
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

        private void SelectObject(TThing obj, int roomX, int roomY, RoomEditorState state)
        {
            SetSelectedObject(obj);
            if (!DesignItems[GetItemID(obj)].Locked)
            {
                if (!state.DragFromCenter)
                {
                    int objX, objY;
                    GetObjectPosition(obj, out objX, out objY);
                    _mouseOffsetX = roomX - objX;
                    _mouseOffsetY = roomY - objY;
                }
                else
                {
                    _mouseOffsetX = 0;
                    _mouseOffsetY = 0;
                }
                _movingObjectWithMouse = true;
            }
        }

        private bool HandleMoveKeysPress(Keys key)
        {
            int curX, curY;
            GetObjectPosition(_selectedObject, out curX, out curY);
            int step = 1;
            switch (key)
            {
                case Keys.Left:
                    _movingKeysDown |= 1; _movingObjectWithKeyboard = true;
                    return MoveObject(curX - step, curY);
                case Keys.Right:
                    _movingKeysDown |= 2; _movingObjectWithKeyboard = true;
                    return MoveObject(curX + step, curY);
                case Keys.Up:
                    _movingKeysDown |= 4; _movingObjectWithKeyboard = true;
                    return MoveObject(curX, curY - step);
                case Keys.Down:
                    _movingKeysDown |= 8; _movingObjectWithKeyboard = true;
                    return MoveObject(curX, curY + step);
            }
            return false;
        }

        private bool HandleMoveKeysRelease(Keys key)
        {
            int moveKeys = _movingKeysDown;
            switch (key)
            {
                case Keys.Left: moveKeys &= ~1; break;
                case Keys.Right: moveKeys &= ~2; break;
                case Keys.Up: moveKeys &= ~4; break;
                case Keys.Down: moveKeys &= ~8; break;
            }
            if (moveKeys != _movingKeysDown)
            {
                _movingKeysDown = moveKeys;
                if (_movingKeysDown == 0)
                {
                    _movingHintTimer.Start();
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Moves currently selected object to the new coordinates
        /// </summary>
        private bool MoveObject(int newX, int newY)
        {
            if (_selectedObject == null)
            {
                ClearMovingState();
            }
            else
            {
                if (SetObjectPosition(_selectedObject, newX, newY))
                {
                    _room.Modified = true;
                }
                _movingHintTimer.Stop();
            }
            return true;
        }

        protected void ClearMovingState()
        {
            _movingObjectWithMouse = false;
            _movingObjectWithKeyboard = false;
            _movingKeysDown = 0;
            _movingHintTimer.Stop();
        }

        private void MovingHintTimer_Tick(object sender, EventArgs e)
        {
            ClearMovingState();
            Invalidate();
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

        /// <summary>
        /// React to the new object selected in the properties grid list.
        /// Override in the child class if necessary.
        /// </summary>
        protected virtual void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
        }

        /// <summary>
        /// React to this filter getting activated.
        /// Override in the child class if necessary.
        /// </summary>
        protected virtual void FilterActivated()
        {
        }

        /// <summary>
        /// React to this filter getting deactivated.
        /// Override in the child class if necessary.
        /// </summary>
        protected virtual void FilterDeactivated()
        {
        }

        /// <summary>
        /// Lets child classes to handle key press.
        /// This is called before the base class does any of its own handling.
        /// Returns if handled by the child class.
        /// </summary>
        protected virtual bool HandleKeyPress(Keys key)
        {
            return false;
        }

        /// <summary>
        /// Lets child classes to handle key release.
        /// This is called before the base class does any of its own handling.
        /// Returns if handled by the child class.
        /// </summary>
        protected virtual bool HandleKeyRelease(Keys key)
        {
            return false;
        }

        /// <summary>
        /// Prepares and shows context menu.
        /// Override in the child classes for the actual menu, or leave unimplemented for no menu.
        /// </summary>
        protected virtual void ShowContextMenu(MouseEventArgs e, RoomEditorState state)
        {
        }

        /// <summary>
        /// Tries to get an object under given coordinates.
        /// Returns null if no object was found.
        /// </summary>
        protected abstract TThing GetObjectAtCoords(int x, int y, RoomEditorState state);
        /// <summary>
        /// Gets current object's position.
        /// </summary>
        protected abstract void GetObjectPosition(TThing obj, out int curX, out int curY);
        /// <summary>
        /// Tries to assign new position in room for the given object.
        /// Returns if anything has changed as a result.
        /// </summary>
        protected abstract bool SetObjectPosition(TThing obj, int newX, int newY);
        /// <summary>
        /// Change object current selection.
        /// </summary>
        protected abstract void SetSelectedObject(TThing obj);

        #endregion
    }
}
