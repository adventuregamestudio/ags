using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class ObjectsEditorFilter : IRoomEditorFilter
    {
        private const string MENU_ITEM_DELETE = "DeleteObject";
        private const string MENU_ITEM_NEW = "NewObject";
        private const string MENU_ITEM_COPY_COORDS = "CopyCoordinates";
        private const string MENU_ITEM_OBJECT_COORDS = "ObjectCoordinates";
        protected Room _room;
        protected Panel _panel;

        private GUIController.PropertyObjectChangedHandler _propertyObjectChangedDelegate;
        private RoomObject _selectedObject;
		private RoomObject _lastSelectedObject;
        private bool _movingObjectWithMouse;
        // mouse offset in ROOM's coordinates
        private int _mouseOffsetX, _mouseOffsetY;
        // mouse click location in ROOM's coordinates
        private int _menuClickX, _menuClickY;
        private List<RoomObject> _objectBaselines = new List<RoomObject>();

        public ObjectsEditorFilter(Panel displayPanel, Room room)
        {
            _room = room;
            _panel = displayPanel;
            _selectedObject = null;
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
            RoomItemRefs = new SortedDictionary<string, RoomObject>();
            DesignItems = new SortedDictionary<string, DesignTimeProperties>();
            InitGameEntities();
        }

        public string Name { get { return "Objects"; } }
        public string DisplayName { get { return "Objects"; } }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public bool SupportVisibleItems { get { return true; } }
        public bool Visible { get; set; }
        public bool Locked { get; set; }

        public SortedDictionary<string, DesignTimeProperties> DesignItems { get; private set; }
        /// <summary>
        /// A lookup table for getting game object reference by they key.
        /// </summary>
        private SortedDictionary<string, RoomObject> RoomItemRefs { get; set; }

        public event EventHandler OnItemsChanged;
        public event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;

        public int ItemCount
        {
            get { return _objectBaselines.Count; }
        }

        public int SelectedArea
		{
			get { return 0; }
		}

		public bool ShowTransparencySlider
		{
			get { return false; }
		}

		public string HelpKeyword
		{
			get { return string.Empty; }
		}

        public bool KeyPressed(Keys key)
		{
            if (_selectedObject == null) return false;
            if (!_selectedObject.Locked)
            {
                int step = GetArrowMoveStepSize();
                switch (key)
                {
                    case Keys.Right:
                        return MoveObject(_selectedObject.StartX + step, _selectedObject.StartY);
                    case Keys.Left:
                        return MoveObject(_selectedObject.StartX - step, _selectedObject.StartY);
                    case Keys.Down:
                        return MoveObject(_selectedObject.StartX, _selectedObject.StartY + step);
                    case Keys.Up:
                        return MoveObject(_selectedObject.StartX, _selectedObject.StartY - step);
                }
            }
            return false;
		}

        public void Invalidate() { _panel.Invalidate(); }

        public virtual void PaintToHDC(IntPtr hDC, RoomEditorState state)
        {
            _objectBaselines.Clear();
            foreach (RoomObject obj in _room.Objects)
            {
                if (obj.Baseline <= 0)
                {
                    obj.EffectiveBaseline = obj.StartY;
                }
                else
                {
                    obj.EffectiveBaseline = obj.Baseline;
                }
                _objectBaselines.Add(obj);
            }
            _objectBaselines.Sort();

            foreach (RoomObject obj in _objectBaselines)
            {
                if (!DesignItems[GetItemID(obj)].Visible) continue;
                int height = GetSpriteHeightForGameResolution(obj.Image);
                int ypos = state.RoomYToWindow(obj.StartY - height);
				Factory.NativeProxy.DrawSpriteToBuffer(obj.Image, state.RoomXToWindow(obj.StartX), ypos, state.Scale);
            }
            
        }

        private int GetSpriteHeightForGameResolution(int spriteSlot)
        {
            int height;
            if (Factory.AGSEditor.CurrentGame.IsHighResolution)
            {
                height = Factory.NativeProxy.GetSpriteResolutionMultiplier(spriteSlot) *
                         Factory.NativeProxy.GetActualSpriteHeight(spriteSlot);
            }
            else
            {
                height = Factory.NativeProxy.GetRelativeSpriteHeight(spriteSlot);
            }
            return height;
        }

        private int GetSpriteWidthForGameResolution(int spriteSlot)
        {
            int width;
            if (Factory.AGSEditor.CurrentGame.IsHighResolution)
            {
                width = Factory.NativeProxy.GetSpriteResolutionMultiplier(spriteSlot) *
                         Factory.NativeProxy.GetActualSpriteWidth(spriteSlot);
            }
            else
            {
                width = Factory.NativeProxy.GetRelativeSpriteWidth(spriteSlot);
            }
            return width;
        }

        public virtual void Paint(Graphics graphics, RoomEditorState state)
        {
            int xPos;
            int yPos;

            if (_selectedObject != null && DesignItems[GetItemID(_selectedObject)].Visible)
            {
                int width = state.RoomSizeToWindow(GetSpriteWidthForGameResolution(_selectedObject.Image));
				int height = state.RoomSizeToWindow(GetSpriteHeightForGameResolution(_selectedObject.Image));
				xPos = state.RoomXToWindow(_selectedObject.StartX);
				yPos = state.RoomYToWindow(_selectedObject.StartY) - height;
                Pen pen = new Pen(Color.Goldenrod);
                pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;
                graphics.DrawRectangle(pen, xPos, yPos, width, height);

                if (_movingObjectWithMouse)
                {
                    System.Drawing.Font font = new System.Drawing.Font("Arial", 10.0f);
                    string toDraw = String.Format("X:{0}, Y:{1}", _selectedObject.StartX, _selectedObject.StartY);

                    int scaledx = xPos + (width / 2) - ((int)graphics.MeasureString(toDraw, font).Width / 2);
                    int scaledy = yPos - (int)graphics.MeasureString(toDraw, font).Height;
                    if (scaledx < 0) scaledx = 0;
                    if (scaledy < 0) scaledy = 0;

                    graphics.DrawString(toDraw, font, pen.Brush, (float)scaledx, (float)scaledy);
                }
                else
                    if (_selectedObject.Locked)
                    {
                        pen = new Pen(Color.Goldenrod, 2);
                        xPos = state.RoomXToWindow(_selectedObject.StartX) + (width / 2);
                        yPos = state.RoomYToWindow(_selectedObject.StartY) - (height / 2);
                        Point center = new Point(xPos, yPos);

                        graphics.DrawLine(pen, center.X - 3, center.Y - 3, center.X + 3, center.Y + 3);
                        graphics.DrawLine(pen, center.X - 3, center.Y + 3, center.X + 3, center.Y - 3);

                    }
            }
        }

        public void MouseDownAlways(MouseEventArgs e, RoomEditorState state)
        {
            _selectedObject = null;
        }

        public virtual bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            int x = state.WindowXToRoom(e.X);
            int y = state.WindowYToRoom(e.Y);
            RoomObject obj = GetObject(x, y);
            
            if (obj != null)
            {                
                SetSelectedObject(obj);
                Factory.GUIController.SetPropertyGridObject(obj);
                if (e.Button == MouseButtons.Right)
                {
                    ShowContextMenu(e, state);
                }
                else
                        if (!obj.Locked)
                        {
                            _movingObjectWithMouse = true;
                            _mouseOffsetX = x - obj.StartX;
                            _mouseOffsetY = y - obj.StartY;
                        }
            }
            if (_selectedObject == null)
            {                
                if (e.Button == MouseButtons.Right)
                {
                    ShowContextMenu(e, state);
                    return true;
                }
                return false;
            }
            return true;
        }

        private RoomObject GetObject(int x, int y)
        {
            for (int i = _objectBaselines.Count - 1; i >= 0; i--)
            {
                RoomObject obj = _objectBaselines[i];
                DesignTimeProperties p = DesignItems[GetItemID(obj)];
                if (!p.Visible || p.Locked) continue;
                if (HitTest(obj, x, y)) return obj;
            }
            return null;
        }

        private bool HitTest(RoomObject obj, int x, int y)
        { 
            int width = GetSpriteWidthForGameResolution(obj.Image);
            int height = GetSpriteHeightForGameResolution(obj.Image);
            return ((x >= obj.StartX) && (x < obj.StartX + width) &&
                (y >= obj.StartY - height) && (y < obj.StartY));
        }

        private void CoordMenuEventHandler(object sender, EventArgs e)
        {
            int tempx = _menuClickX;
            int tempy = _menuClickY;

            if ((Factory.AGSEditor.CurrentGame.Settings.UseLowResCoordinatesInScript) &&
             (_room.Resolution == RoomResolution.HighRes))
            {
                tempx /= 2;
                tempy /= 2;
            }

            string textToCopy = tempx.ToString() + ", " + tempy.ToString();
            Utilities.CopyTextToClipboard(textToCopy);
        }

        private void ShowCoordMenu(MouseEventArgs e, RoomEditorState state)
        {
            EventHandler onClick = new EventHandler(CoordMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            menu.Items.Add(new ToolStripMenuItem("Copy mouse coordinates to clipboard", null, onClick, MENU_ITEM_COPY_COORDS));

            _menuClickX = state.WindowXToRoom(e.X);
            _menuClickY = state.WindowYToRoom(e.Y);

            menu.Show(_panel, e.X, e.Y);
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Name == MENU_ITEM_DELETE)
            {
                if (Factory.GUIController.ShowQuestion("Are you sure you want to delete this object?") == DialogResult.Yes)
                {
                    _room.Objects.Remove(_selectedObject);
                    RemoveObjectRef(_selectedObject);
                    foreach (RoomObject obj in _room.Objects)
                    {
                        if (obj.ID >= _selectedObject.ID)
                        {
                            string oldID = GetItemID(obj);
                            obj.ID--;
                            UpdateObjectRef(obj, oldID);
                        }
                    }
                    _selectedObject = null;
                    Factory.GUIController.SetPropertyGridObject(_room);
                    SetPropertyGridList();
                    _room.Modified = true;
                    OnItemsChanged(this, null);
                    _panel.Invalidate();
                }
            }
            else if (item.Name == MENU_ITEM_NEW)
            {
                if (_room.Objects.Count >= Room.MAX_OBJECTS)
                {
                    Factory.GUIController.ShowMessage("This room already has the maximum " + Room.MAX_OBJECTS + " objects.", MessageBoxIcon.Information);
                    return;
                }
                RoomObject newObj = new RoomObject(_room);
                newObj.ID = _room.Objects.Count;
                newObj.StartX = SetObjectCoordinate(_menuClickX);
                newObj.StartY = SetObjectCoordinate(_menuClickY);
                _room.Objects.Add(newObj);
                AddObjectRef(newObj);
                SetSelectedObject(newObj);
                SetPropertyGridList();
                Factory.GUIController.SetPropertyGridObject(newObj);
                _room.Modified = true;
                OnItemsChanged(this, null);
                _panel.Invalidate();                
            }
            else if (item.Name == MENU_ITEM_OBJECT_COORDS)
            {
                int tempx = _selectedObject.StartX;
                int tempy = _selectedObject.StartY;

                if ((Factory.AGSEditor.CurrentGame.Settings.UseLowResCoordinatesInScript) &&
                	(_room.Resolution == RoomResolution.HighRes))
                {
                    tempx = tempx / 2;
                    tempy = tempy / 2;
                }

                string textToCopy = tempx.ToString() + ", " + tempy.ToString();
                Utilities.CopyTextToClipboard(textToCopy);
            }
        }

        private void ShowContextMenu(MouseEventArgs e, RoomEditorState state)
        {
            EventHandler onClick = new EventHandler(ContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            if (_selectedObject != null)
            {
                menu.Items.Add(new ToolStripMenuItem("Delete", null, onClick, MENU_ITEM_DELETE));
                menu.Items.Add(new ToolStripSeparator());
            }
            menu.Items.Add(new ToolStripMenuItem("Place New Object Here", null, onClick, MENU_ITEM_NEW));
            if (_selectedObject != null)
            {
                menu.Items.Add(new ToolStripMenuItem("Copy Object Coordinates to Clipboard", null, onClick, MENU_ITEM_OBJECT_COORDS));
            }
            _menuClickX = state.WindowXToRoom(e.X);
            _menuClickY = state.WindowYToRoom(e.Y);

            menu.Show(_panel, e.X, e.Y);
        }

        public virtual bool MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            _movingObjectWithMouse = false;
			_lastSelectedObject = _selectedObject;

            if (e.Button == MouseButtons.Middle)
            {
                ShowCoordMenu(e, state);
            }
            return false;
        }

		public bool DoubleClick(RoomEditorState state)
		{
			if (_lastSelectedObject != null)
			{
				Sprite chosenSprite = SpriteChooser.ShowSpriteChooser(_lastSelectedObject.Image);
				if (chosenSprite != null)
				{
					_lastSelectedObject.Image = chosenSprite.Number;
				}
                return true;
			}
            return false;
		}

        public virtual bool MouseMove(int x, int y, RoomEditorState state)
        {
            if (!_movingObjectWithMouse) return false;
            int realX = state.WindowXToRoom(x);
            int realY = state.WindowYToRoom(y);

            if ((_movingObjectWithMouse) && (realY < _room.Height) &&
                (realX < _room.Width) && (realY >= 0) && (realX >= 0))
            {
                int newX = realX - _mouseOffsetX;
                int newY = realY - _mouseOffsetY;
                return MoveObject(newX, newY);
            }
            return false;
        }

        private bool MoveObject(int newX, int newY)
        {            
            if (_selectedObject == null)
            {
                _movingObjectWithMouse = false;
            }
            else
            {
                if ((newX != _selectedObject.StartX) ||
                    (newY != _selectedObject.StartY))
                {
                    _selectedObject.StartX = SetObjectCoordinate(newX);
                    _selectedObject.StartY = SetObjectCoordinate(newY);
                    _room.Modified = true;
                }
            }
            return true;            
        }

        private bool IsHighResGameWithLowResScript()
        {
            return (Factory.AGSEditor.CurrentGame.IsHighResolution) &&
                (Factory.AGSEditor.CurrentGame.Settings.UseLowResCoordinatesInScript);
        }

        private int GetArrowMoveStepSize()
        {
            return IsHighResGameWithLowResScript() ? 2 : 1;
        }

        private int SetObjectCoordinate(int newCoord)
        {
            if (IsHighResGameWithLowResScript())
            {
                // Round co-ordinate to nearest even number to reflect what
                // will happen in the engine
                newCoord = (newCoord / 2) * 2;
            }
            return newCoord;
        }

        public void FilterOn()
        {
            SetPropertyGridList();
            Factory.GUIController.OnPropertyObjectChanged += _propertyObjectChangedDelegate;
        }

        public void FilterOff()
        {
            Factory.GUIController.OnPropertyObjectChanged -= _propertyObjectChangedDelegate;
        }

        public void Dispose()
        {
        }

        public void CommandClick(string command)
        {
        }

        /// <summary>
        /// Initialize dictionary of current item references.
        /// </summary>
        /// <returns></returns>
        private SortedDictionary<string, RoomObject> InitItemRefs()
        {
            SortedDictionary<string, RoomObject> items = new SortedDictionary<string, RoomObject>();
            foreach (RoomObject obj in _room.Objects)
            {
                items.Add(GetItemID(obj), obj);
            }
            return items;
        }

        public string GetItemName(string id)
        {
            RoomObject obj;
            if (id != null && RoomItemRefs.TryGetValue(id, out obj))
                return obj.PropertyGridTitle;
            return null;
        }

        public void SelectItem(string id)
        {
            if (id != null)
            {
                RoomObject obj;
                if (RoomItemRefs.TryGetValue(id, out obj))
                {
                    _selectedObject = obj;
                    Factory.GUIController.SetPropertyGridObject(obj);                    
                    return;
                }
            }

            _selectedObject = null;
            Factory.GUIController.SetPropertyGridObject(_room);            
        }

        public Cursor GetCursor(int x, int y, RoomEditorState state)
        {
            if (_movingObjectWithMouse) return Cursors.Hand;
            x = state.WindowXToRoom(x);
            y = state.WindowYToRoom(y);
            if (GetObject(x, y) != null) return Cursors.Default;
            return null;
        }

        public bool AllowClicksInterception()
        {
            return true;
        }

        private void SetSelectedObject(RoomObject roomObject)
        {
            _selectedObject = roomObject;
            if (OnSelectedItemChanged != null)
            {
                OnSelectedItemChanged(this, new SelectedRoomItemEventArgs(roomObject.PropertyGridTitle));
            }
        }

        private void SetPropertyGridList()
        {
            Dictionary<string, object> defaultPropertyObjectList = new Dictionary<string, object>();
            defaultPropertyObjectList.Add(_room.PropertyGridTitle, _room);
            foreach (RoomObject obj in _room.Objects)
            {
                defaultPropertyObjectList.Add(obj.PropertyGridTitle, obj);
            }

            Factory.GUIController.SetPropertyGridObjectList(defaultPropertyObjectList);
        }

        private void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is RoomObject)
            {
                SetSelectedObject((RoomObject)newPropertyObject);                
                _panel.Invalidate();
            }
            else if (newPropertyObject is Room)
            {
                _selectedObject = null;
                _panel.Invalidate();
            }
        }

        private string GetItemID(RoomObject obj)
        {
            // Use numeric object's ID as a "unique identifier", for now (script name is optional!)
            return obj.ID.ToString("D4");
        }

        private void InitGameEntities()
        {
            // Initialize item reference
            RoomItemRefs = InitItemRefs();
            // Initialize design-time properties
            // TODO: load last design settings
            DesignItems.Clear();
            foreach (var item in RoomItemRefs)
                DesignItems.Add(item.Key, new DesignTimeProperties());
        }

        private void AddObjectRef(RoomObject obj)
        {
            string id = GetItemID(obj);
            if (RoomItemRefs.ContainsKey(id))
                return;
            RoomItemRefs.Add(id, obj);
            DesignItems.Add(id, new DesignTimeProperties());
        }

        private void RemoveObjectRef(RoomObject obj)
        {
            string id = GetItemID(obj);
            RoomItemRefs.Remove(id);
            DesignItems.Remove(id);
        }

        private void UpdateObjectRef(RoomObject obj, string oldID)
        {
            if (!RoomItemRefs.ContainsKey(oldID))
                return;
            string newID = GetItemID(obj);
            RoomItemRefs.Remove(oldID);
            RoomItemRefs.Add(newID, obj);
            // We must keep DesignTimeProperties!
            DesignItems.Add(newID, DesignItems[oldID]);
            DesignItems.Remove(oldID);
        }
    }
}
