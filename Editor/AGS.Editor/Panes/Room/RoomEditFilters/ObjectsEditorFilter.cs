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
        private int _mouseOffsetX, _mouseOffsetY;
        private int _menuClickX, _menuClickY;
        private List<RoomObject> _objectBaselines = new List<RoomObject>();

        public ObjectsEditorFilter(Panel displayPanel, Room room)
        {
            _room = room;
            _panel = displayPanel;
            _selectedObject = null;
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
            VisibleItems = new List<string>();
            LockedItems = new List<string>();
        }

        public string DisplayName { get { return "Objects"; } }

        public bool VisibleByDefault { get { return true; } }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public bool SupportVisibleItems { get { return true; } }

        public List<string> VisibleItems { get; private set; }
        public List<string> LockedItems { get; private set; }

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
                if (!VisibleItems.Contains(GetUniqueName(obj))) continue;
                int height = GetSpriteHeightForGameResolution(obj.Image);
                int ypos = AdjustYCoordinateForWindowScroll(obj.StartY, state) - (height * state.ScaleFactor);
				Factory.NativeProxy.DrawSpriteToBuffer(obj.Image, AdjustXCoordinateForWindowScroll(obj.StartX, state), ypos, state.ScaleFactor);
            }
            
        }

        // CLNUP same as CharctersEditorFilters.cs
        private int GetSpriteHeightForGameResolution(int spriteSlot)
        {
            return Factory.NativeProxy.GetSpriteHeight(spriteSlot);
        }

        // CLNUP same as CharctersEditorFilters.cs
        private int GetSpriteWidthForGameResolution(int spriteSlot)
        {
            return Factory.NativeProxy.GetSpriteWidth(spriteSlot);
        }

        public virtual void Paint(Graphics graphics, RoomEditorState state)
        {
            int xPos;
            int yPos;

            if (_selectedObject != null && VisibleItems.Contains(GetUniqueName(_selectedObject)))
            {
                int width = GetSpriteWidthForGameResolution(_selectedObject.Image);
				int height = GetSpriteHeightForGameResolution(_selectedObject.Image);
				xPos = AdjustXCoordinateForWindowScroll(_selectedObject.StartX, state);
				yPos = AdjustYCoordinateForWindowScroll(_selectedObject.StartY, state) - (height * state.ScaleFactor);
                Pen pen = new Pen(Color.Goldenrod);
                pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;
                graphics.DrawRectangle(pen, xPos, yPos, width * state.ScaleFactor, height * state.ScaleFactor);

                if (_movingObjectWithMouse)
                {
                    System.Drawing.Font font = new System.Drawing.Font("Arial", 10.0f);
                    string toDraw = String.Format("X:{0}, Y:{1}", _selectedObject.StartX, _selectedObject.StartY);

                    int scaledx = xPos + (width * state.ScaleFactor / 2) - ((int)graphics.MeasureString(toDraw, font).Width / 2);
                    int scaledy = yPos - (int)graphics.MeasureString(toDraw, font).Height;
                    if (scaledx < 0) scaledx = 0;
                    if (scaledy < 0) scaledy = 0;

                    graphics.DrawString(toDraw, font, pen.Brush, (float)scaledx, (float)scaledy);
                }
                else
                    if (_selectedObject.Locked)
                    {
                        pen = new Pen(Color.Goldenrod, 2);
                        xPos = AdjustXCoordinateForWindowScroll(_selectedObject.StartX, state) + (GetSpriteWidthForGameResolution(_selectedObject.Image) / 2 * state.ScaleFactor);
                        yPos = AdjustYCoordinateForWindowScroll(_selectedObject.StartY, state) - (GetSpriteHeightForGameResolution(_selectedObject.Image) / 2 * state.ScaleFactor);
                        Point center = new Point(xPos, yPos);

                        graphics.DrawLine(pen, center.X - 3, center.Y - 3, center.X + 3, center.Y + 3);
                        graphics.DrawLine(pen, center.X - 3, center.Y + 3, center.X + 3, center.Y - 3);

                    }
            }
        }

		private int AdjustXCoordinateForWindowScroll(int x, RoomEditorState state)
		{
			return (x - (state.ScrollOffsetX / state.ScaleFactor)) * state.ScaleFactor;
		}

		private int AdjustYCoordinateForWindowScroll(int y, RoomEditorState state)
		{
			return (y - (state.ScrollOffsetY / state.ScaleFactor)) * state.ScaleFactor;
		}

        public void MouseDownAlways(MouseEventArgs e, RoomEditorState state)
        {
            _selectedObject = null;
        }

        public virtual bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            int x = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
            int y = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;
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
                string name = GetUniqueName(obj);
                if (!VisibleItems.Contains(name) || LockedItems.Contains(name)) continue;
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

            string textToCopy = tempx.ToString() + ", " + tempy.ToString();
            Utilities.CopyTextToClipboard(textToCopy);
        }

        private void ShowCoordMenu(MouseEventArgs e, RoomEditorState state)
        {
            EventHandler onClick = new EventHandler(CoordMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            menu.Items.Add(new ToolStripMenuItem("Copy mouse coordinates to clipboard", null, onClick, MENU_ITEM_COPY_COORDS));

            _menuClickX = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
            _menuClickY = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;

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
                    foreach (RoomObject obj in _room.Objects)
                    {
                        if (obj.ID >= _selectedObject.ID)
                        {
                            obj.ID--;
                        }
                    }
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
                newObj.StartX = SetObjectCoordinate(_menuClickX);
                newObj.StartY = SetObjectCoordinate(_menuClickY);
                _room.Objects.Add(newObj);
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
            _menuClickX = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
            _menuClickY = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;

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
            int realX = (x + state.ScrollOffsetX) / state.ScaleFactor;
            int realY = (y + state.ScrollOffsetY) / state.ScaleFactor;

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

        public List<string> GetItemsNames()
        {
            List<string> names = new List<string>(_room.Objects.Count);
            foreach (RoomObject obj in _room.Objects)
            {
                names.Add(GetUniqueName(obj));
            }
            return names;
        }

        public void SelectItem(string name)
        {
            if (name != null)
            {
                foreach (RoomObject obj in _room.Objects)
                {
                    if (GetUniqueName(obj) != name) continue;
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
            x = (x + state.ScrollOffsetX) / state.ScaleFactor;
            y = (y + state.ScrollOffsetY) / state.ScaleFactor;
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
                OnSelectedItemChanged(this, new SelectedRoomItemEventArgs(GetUniqueName(roomObject)));
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

        private string GetUniqueName(RoomObject obj)
        {
            return obj.PropertyGridTitle;
        }

    }

}
