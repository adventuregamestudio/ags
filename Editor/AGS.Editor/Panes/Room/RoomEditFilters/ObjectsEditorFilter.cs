using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// TODO: it must be possible to pick out a base class from ObjectsEditorFilter and
    /// CharactersEditorFilter, there much common functionality here.
    /// </summary>
    public class ObjectsEditorFilter : IRoomEditorFilter
    {
        private const string MENU_ITEM_DELETE = "DeleteObject";
        private const string MENU_ITEM_NEW = "NewObject";
        private const string MENU_ITEM_OBJECT_COORDS = "ObjectCoordinates";
        protected Room _room;
        protected Panel _panel;
        protected RoomSettingsEditor _editor;
        private bool _isOn;
        private GUIController.PropertyObjectChangedHandler _propertyObjectChangedDelegate;
        private RoomObject _selectedObject;
		private RoomObject _lastSelectedObject;
        private bool _movingObjectWithMouse;
        // mouse offset in ROOM's coordinates
        private int _mouseOffsetX, _mouseOffsetY;
        // mouse click location in ROOM's coordinates
        private int _menuClickX, _menuClickY;
        private bool _movingObjectWithKeyboard = false;
        private int _movingKeysDown = 0;
        private Timer _movingHintTimer = new Timer();
        private List<RoomObject> _objectBaselines = new List<RoomObject>();

        public ObjectsEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room)
        {
            _room = room;
            _panel = displayPanel;
            _editor = editor;
            _selectedObject = null;
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
            RoomItemRefs = new SortedDictionary<string, RoomObject>();
            DesignItems = new SortedDictionary<string, DesignTimeProperties>();
            InitGameEntities();

            _movingHintTimer.Interval = 2000;
            _movingHintTimer.Tick += MovingHintTimer_Tick;
        }

        public string Name { get { return "Objects"; } }
        public string DisplayName { get { return "Objects"; } }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public bool SupportVisibleItems { get { return true; } }
        public bool Modified { get; set; }
        public bool Visible { get; set; }
        public bool Locked { get; set; }
        public bool Enabled { get { return _isOn; } }

        public SortedDictionary<string, DesignTimeProperties> DesignItems { get; private set; }
        /// <summary>
        /// A lookup table for getting game object reference by they key.
        /// </summary>
        private SortedDictionary<string, RoomObject> RoomItemRefs { get; set; }

        public event EventHandler OnItemsChanged;
        public event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;
        public event EventHandler<RoomFilterContextMenuArgs> OnContextMenu;

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
            if (_selectedObject == null)
                return false;
            if (DesignItems[GetItemID(_selectedObject)].Locked)
                return false;

            int step = GetArrowMoveStepSize();
            switch (key)
            {
                case Keys.Left:
                    _movingKeysDown |= 1; _movingObjectWithKeyboard = true;
                    return MoveObject(_selectedObject.StartX - step, _selectedObject.StartY);
                case Keys.Right:
                    _movingKeysDown |= 2; _movingObjectWithKeyboard = true;
                    return MoveObject(_selectedObject.StartX + step, _selectedObject.StartY);
                case Keys.Up:
                    _movingKeysDown |= 4; _movingObjectWithKeyboard = true;
                    return MoveObject(_selectedObject.StartX, _selectedObject.StartY - step);
                case Keys.Down:
                    _movingKeysDown |= 8; _movingObjectWithKeyboard = true;
                    return MoveObject(_selectedObject.StartX, _selectedObject.StartY + step);
            }
            return false;
        }

        public bool KeyReleased(Keys key)
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

        public void Invalidate() { _panel.Invalidate(); }

        public virtual void Paint(Graphics graphics, RoomEditorState state)
        {
            _objectBaselines.Clear();
            _objectBaselines.AddRange(_room.Objects.Select(o =>
            {
                o.EffectiveBaseline = o.Baseline <= 0 ? o.StartY : o.Baseline;
                return o;
            }));
            _objectBaselines.Sort();

            foreach (RoomObject obj in _objectBaselines.Where(o => DesignItems[GetItemID(o)].Visible))
            {
                Size spriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(obj.Image);
                spriteSize.Width = state.RoomSizeToWindow(spriteSize.Width);
                spriteSize.Height = state.RoomSizeToWindow(spriteSize.Height);
                int xpos = state.RoomXToWindow(obj.StartX);
                int ypos = state.RoomYToWindow(obj.StartY) - spriteSize.Height;

                using (Bitmap sprite = Factory.NativeProxy.GetBitmapForSprite(obj.Image))
                using (Bitmap sprite32bppAlpha = new Bitmap(sprite.Width, sprite.Height, PixelFormat.Format32bppArgb))
                {
                    sprite32bppAlpha.SetRawData(sprite.GetRawData());
                    graphics.DrawImage(sprite32bppAlpha, xpos, ypos, spriteSize.Width, spriteSize.Height);
                }
            }

            if (!Enabled || _selectedObject == null)
                return;

            DesignTimeProperties design = DesignItems[GetItemID(_selectedObject)];
            if (!design.Visible)
                return;

            int width, height;
            Utilities.GetSizeSpriteWillBeRenderedInGame(_selectedObject.Image, out width, out height);
            width = state.RoomSizeToWindow(width);
            height = state.RoomSizeToWindow(height);
            int xPos = state.RoomXToWindow(_selectedObject.StartX);
            int yPos = state.RoomYToWindow(_selectedObject.StartY) - height;
            Pen pen = new Pen(Color.Goldenrod);
            pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;
            graphics.DrawRectangle(pen, xPos, yPos, width, height);

            if (_movingObjectWithMouse || _movingObjectWithKeyboard)
            {
                Brush shadeBrush = new SolidBrush(Color.FromArgb(200, Color.Black));
                System.Drawing.Font font = new System.Drawing.Font("Arial", 10.0f);
                string toDraw = String.Format("X:{0}, Y:{1}", _selectedObject.StartX, _selectedObject.StartY);

                var textSize = graphics.MeasureString(toDraw, font);
                int scaledx = xPos + (width / 2) - ((int)textSize.Width / 2);
                int scaledy = yPos - (int)textSize.Height;
                if (scaledx < 0) scaledx = 0;
                if (scaledy < 0) scaledy = 0;
                if (scaledx + textSize.Width >= graphics.VisibleClipBounds.Width)
                    scaledx = (int)(graphics.VisibleClipBounds.Width - textSize.Width);
                if (scaledy + textSize.Height >= graphics.VisibleClipBounds.Height)
                    scaledy = (int)(graphics.VisibleClipBounds.Height - textSize.Height);

                graphics.FillRectangle(shadeBrush, scaledx, scaledy, textSize.Width, textSize.Height);
                graphics.DrawString(toDraw, font, pen.Brush, (float)scaledx, (float)scaledy);
            }
            else if (design.Locked)
            {
                pen = new Pen(Color.Goldenrod, 2);
                xPos = state.RoomXToWindow(_selectedObject.StartX) + (width / 2);
                yPos = state.RoomYToWindow(_selectedObject.StartY) - (height / 2);
                Point center = new Point(xPos, yPos);

                graphics.DrawLine(pen, center.X - 3, center.Y - 3, center.X + 3, center.Y + 3);
                graphics.DrawLine(pen, center.X - 3, center.Y + 3, center.X + 3, center.Y - 3);
            }
        }

        public virtual bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            if (e.Button == MouseButtons.Middle) return false;

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
                else if(!DesignItems[GetItemID(obj)].Locked)
                {
                    _movingObjectWithMouse = true;
                    _mouseOffsetX = x - obj.StartX;
                    _mouseOffsetY = y - obj.StartY;
                }
            }
            else
            {
                _selectedObject = null;
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
                if (!p.Visible) continue;
                if (HitTest(obj, x, y)) return obj;
            }
            return null;
        }

        private bool HitTest(RoomObject obj, int x, int y)
        {
            int width, height;
            Utilities.GetSizeSpriteWillBeRenderedInGame(obj.Image, out width, out height);
            return ((x >= obj.StartX) && (x < obj.StartX + width) &&
                (y >= obj.StartY - height) && (y < obj.StartY));
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Name == MENU_ITEM_DELETE)
            {
                if (Factory.GUIController.ShowQuestion("Are you sure you want to delete this object?") == DialogResult.Yes)
                {
                    _room.Objects.Remove(_selectedObject);
                    _objectBaselines.Remove(_selectedObject);
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
                    OnItemsChanged(this, null);
                    _selectedObject = null;
                    Factory.GUIController.SetPropertyGridObject(_room);
                    SetPropertyGridList();
                    _room.Modified = true;
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
                newObj.Name = Factory.AGSEditor.GetFirstAvailableScriptName("oObject", 0, _room);
                newObj.StartX = SetObjectCoordinate(_menuClickX);
                newObj.StartY = SetObjectCoordinate(_menuClickY);
                _room.Objects.Add(newObj);
                AddObjectRef(newObj);
                OnItemsChanged(this, null);
                SetSelectedObject(newObj);
                SetPropertyGridList();
                Factory.GUIController.SetPropertyGridObject(newObj);
                _room.Modified = true;
                _panel.Invalidate();                
            }
            else if (item.Name == MENU_ITEM_OBJECT_COORDS)
            {
                int tempx = _selectedObject.StartX;
                int tempy = _selectedObject.StartY;
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
            OnContextMenu?.Invoke(this, new RoomFilterContextMenuArgs(menu, e.X, e.Y));

            _menuClickX = state.WindowXToRoom(e.X);
            _menuClickY = state.WindowYToRoom(e.Y);

            menu.Show(_panel, e.X, e.Y);
        }

        public virtual bool MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            _movingObjectWithMouse = false;
            _lastSelectedObject = _selectedObject;
            return false;
        }

		public bool DoubleClick(RoomEditorState state)
		{
			if (_lastSelectedObject != null)
			{
				Sprite chosenSprite = SpriteChooser.ShowSpriteChooser(_lastSelectedObject.Image);
				if (chosenSprite != null && chosenSprite.Number != _lastSelectedObject.Image)
				{
					_lastSelectedObject.Image = chosenSprite.Number;
					_room.Modified = true;
				}
                return true;
			}
            return false;
		}

        public virtual bool MouseMove(int x, int y, RoomEditorState state)
        {
            if (!_movingObjectWithMouse) return false;

            int newX = state.WindowXToRoom(x) - _mouseOffsetX;
            int newY = state.WindowYToRoom(y) - _mouseOffsetY;
            return MoveObject(newX, newY);
        }

        private bool MoveObject(int newX, int newY)
        {            
            if (_selectedObject == null)
            {
                ClearMovingState();
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
                _movingHintTimer.Stop();
            }
            return true;
        }

        private void ClearMovingState()
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

        private int GetArrowMoveStepSize() // CLNUP probably remove
        {
            return 1;
        }

        private int SetObjectCoordinate(int newCoord)
        {
            return newCoord;
        }

        public void FilterOn()
        {
            SetPropertyGridList();
            Factory.GUIController.OnPropertyObjectChanged += _propertyObjectChangedDelegate;
            _isOn = true;
            ClearMovingState();
        }

        public void FilterOff()
        {
            Factory.GUIController.OnPropertyObjectChanged -= _propertyObjectChangedDelegate;
            _isOn = false;
            ClearMovingState();
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
                    SetPropertyGridObject(obj);                    
                    return;
                }
            }

            _selectedObject = null;
            SetPropertyGridObject(_room);            
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
            ClearMovingState();
        }

        private void SetPropertyGridList()
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(_room.PropertyGridTitle, _room);
            foreach (RoomObject obj in _room.Objects)
            {
                list.Add(obj.PropertyGridTitle, obj);
            }
            Factory.GUIController.SetPropertyGridObjectList(list, _editor.ContentDocument, _room);
        }

        protected void SetPropertyGridObject(object obj)
        {
            Factory.GUIController.SetPropertyGridObject(obj, _editor.ContentDocument);
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
