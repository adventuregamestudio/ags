using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// ObjectsEditorFilter manages RoomObjects.
    /// </summary>
    public class ObjectsEditorFilter : BaseThingEditorFilter<RoomObject>
    {
        private const string MENU_ITEM_DELETE = "DeleteObject";
        private const string MENU_ITEM_NEW = "NewObject";
        private const string MENU_ITEM_OBJECT_COORDS = "ObjectCoordinates";
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
            : base(displayPanel, editor, room)
        {
            // Init a starting list of item references for navigation UI
            InitRoomItemRefs(CollectItemRefs());

            _movingHintTimer.Interval = 2000;
            _movingHintTimer.Tick += MovingHintTimer_Tick;
        }

        public override string Name { get { return "Objects"; } }
        public override string DisplayName { get { return "Objects"; } }

        public override event EventHandler OnItemsChanged;
        public override event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;
        public override event EventHandler<RoomFilterContextMenuArgs> OnContextMenu;

		public override string HelpKeyword
		{
			get { return string.Empty; }
		}

        public override bool KeyPressed(Keys key)
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

        public override bool KeyReleased(Keys key)
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

        public override void PaintToHDC(IntPtr hDC, RoomEditorState state)
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
                int width, height;
                Utilities.GetSizeSpriteWillBeRenderedInGame(obj.Image, out width, out height);
                int ypos = state.RoomYToWindow(obj.StartY - height);
				Factory.NativeProxy.DrawSpriteToBuffer(obj.Image, state.RoomXToWindow(obj.StartX), ypos, state.Scale);
            }
            
        }

        public override void Paint(Graphics graphics, RoomEditorState state)
        {
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

        public override bool MouseDown(MouseEventArgs e, RoomEditorState state)
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
                RoomEditorState.AdjustCoordsToMatchEngine(_room, ref tempx, ref tempy);
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

        public override bool MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            _movingObjectWithMouse = false;
            _lastSelectedObject = _selectedObject;
            return false;
        }

		public override bool DoubleClick(RoomEditorState state)
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

        public override bool MouseMove(int x, int y, RoomEditorState state)
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

        private int GetArrowMoveStepSize()
        {
            return RoomEditorState.IsHighResRoomWithLowResScript(_room) ? 2 : 1;
        }

        private int SetObjectCoordinate(int newCoord)
        {
            if (RoomEditorState.IsHighResRoomWithLowResScript(_room))
            {
                // Round co-ordinate to nearest even number to reflect what
                // will happen in the engine
                newCoord = (newCoord / 2) * 2;
            }
            return newCoord;
        }

        protected override void FilterActivated()
        {
            ClearMovingState();
        }

        protected override void FilterDeactivated()
        {
            ClearMovingState();
        }

        public override void CommandClick(string command)
        {
        }

        /// <summary>
        /// Initialize dictionary of current item references.
        /// </summary>
        protected SortedDictionary<string, RoomObject> CollectItemRefs()
        {
            SortedDictionary<string, RoomObject> items = new SortedDictionary<string, RoomObject>();
            foreach (RoomObject obj in _room.Objects)
            {
                items.Add(GetItemID(obj), obj);
            }
            return items;
        }

        /// <summary>
        /// Gets this object's script name.
        /// </summary>
        protected override string GetItemScriptName(RoomObject obj)
        {
            return obj.Name;
        }

        /// <summary>
        /// Forms a PropertyGrid's entry title for this object.
        /// </summary>
        protected override string GetPropertyGridItemTitle(RoomObject obj)
        {
            return obj.PropertyGridTitle;
        }

        public override Cursor GetCursor(int x, int y, RoomEditorState state)
        {
            if (_movingObjectWithMouse) return Cursors.Hand;
            x = state.WindowXToRoom(x);
            y = state.WindowYToRoom(y);
            if (GetObject(x, y) != null) return Cursors.Default;
            return null;
        }

        public override bool AllowClicksInterception()
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

        protected override void GUIController_OnPropertyObjectChanged(object newPropertyObject)
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

        protected override string GetItemID(RoomObject obj)
        {
            // Use numeric object's ID as a "unique identifier", for now (script name is optional!)
            return obj.ID.ToString("D4");
        }
    }
}
