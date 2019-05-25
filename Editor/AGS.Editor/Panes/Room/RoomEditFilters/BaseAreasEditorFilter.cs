using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public abstract class BaseAreasEditorFilter : IRoomEditorFilter
    {
        protected const string SELECT_AREA_COMMAND = "SelectArea";
        protected const string DRAW_LINE_COMMAND = "DrawLine";
        protected const string DRAW_FREEHAND_COMMAND = "DrawFreehand";
		protected const string DRAW_RECTANGLE_COMMAND = "DrawRectangle";
		protected const string DRAW_FILL_COMMAND = "DrawFill";
        protected const string COPY_WALKABLE_AREA_MASK_COMMAND = "CopyWalkableMaskToRegions";
        protected const string IMPORT_MASK_COMMAND = "ImportAreaMask";
        protected const string EXPORT_MASK_COMMAND = "ExportAreaMask";
        protected const string UNDO_COMMAND = "UndoAreaDraw";
		protected const string GREYED_OUT_MASKS_COMMAND = "GreyOutMasks";

        protected const int TOOLBAR_INDEX_UNDO = 5;
		protected const int TOOLBAR_INDEX_GREY_OUT_MASKS = 8;
        
        private const string MENU_ITEM_COPY_COORDS = "CopyCoordinates";
        private const string ERASER = "Eraser";
        private int _menuClickX, _menuClickY;        

		private readonly Brush[] _brushesForAreas = new Brush[]{Brushes.Black, Brushes.DarkBlue,
			Brushes.DarkGreen, Brushes.DarkCyan, Brushes.DarkRed, Brushes.DarkMagenta, 
			Brushes.Brown, Brushes.Red, Brushes.Red, Brushes.Blue,
			Brushes.LightGreen, Brushes.Cyan, Brushes.Red, Brushes.Pink,
			Brushes.Yellow, Brushes.White};
		private readonly Pen[] _pensForAreas = new Pen[]{Pens.Black, Pens.DarkBlue,
			Pens.DarkGreen, Pens.DarkCyan, Pens.DarkRed, Pens.DarkMagenta, 
			Pens.Brown, Pens.Red, Pens.Red, Pens.Blue,
			Pens.LightGreen, Pens.Cyan, Pens.Red, Pens.Pink,
			Pens.Yellow, Pens.White};

        private GUIController.PropertyObjectChangedHandler _propertyObjectChangedDelegate;
        protected Room _room;
        protected Panel _panel;        
		protected ToolTip _tooltip;
        private int _selectedArea = 1;
		private int _drawingWithArea;
        private bool _mouseDown = false;
        // Mouse hold/release positions in ROOM's coordinates
        private int _mouseDownX, _mouseDownY;
        private int _currentMouseX, _currentMouseY;
        private bool _shouldSetDrawModeOnMouseUp = false;

        private static AreaDrawMode _drawMode = AreaDrawMode.Select;
        private static List<MenuCommand> _toolbarIcons = null;
        private static bool _registeredIcons = false;
        private static Cursor _selectCursor;
		private static bool _greyedOutMasks = true;

        public BaseAreasEditorFilter(Panel displayPanel, Room room)
        {
            if (!_registeredIcons)
            {
                Factory.GUIController.RegisterIcon("SelectAreaIcon", Resources.ResourceManager.GetIcon("findarea.ico"));
                Factory.GUIController.RegisterIcon("DrawLineIcon", Resources.ResourceManager.GetIcon("drawline.ico"));
                Factory.GUIController.RegisterIcon("DrawFreehandIcon", Resources.ResourceManager.GetIcon("drawfree.ico"));
				Factory.GUIController.RegisterIcon("DrawRectIcon", Resources.ResourceManager.GetIcon("drawrect.ico"));
				Factory.GUIController.RegisterIcon("DrawFillIcon", Resources.ResourceManager.GetIcon("drawfill.ico"));
                Factory.GUIController.RegisterIcon("ImportMaskIcon", Resources.ResourceManager.GetIcon("importmask.ico"));
                Factory.GUIController.RegisterImage("ExportMaskIcon", Resources.ResourceManager.GetBitmap("exportmask.png"));
                Factory.GUIController.RegisterIcon("CopyWalkableAreaMaskIcon", Resources.ResourceManager.GetIcon("copymask.ico"));
				Factory.GUIController.RegisterIcon("GreyedOutMasksIcon", Resources.ResourceManager.GetIcon("greymasks.ico"));                
				_selectCursor = Resources.ResourceManager.GetCursor("findarea.cur");
                _registeredIcons = true;
            }

            _tooltip = new ToolTip();
            _tooltip.IsBalloon = true;
            _toolbarIcons = new List<MenuCommand>();
            _toolbarIcons.Add(new MenuCommand(SELECT_AREA_COMMAND, "Select area (Ctrl+C)", "SelectAreaIcon"));
            _toolbarIcons.Add(new MenuCommand(DRAW_LINE_COMMAND, "Line tool (Ctrl+N)", "DrawLineIcon"));
            _toolbarIcons.Add(new MenuCommand(DRAW_FREEHAND_COMMAND, "Freehand tool (Ctrl+D)", "DrawFreehandIcon"));
			_toolbarIcons.Add(new MenuCommand(DRAW_RECTANGLE_COMMAND, "Rectangle tool (Ctrl+E)", "DrawRectIcon"));
			_toolbarIcons.Add(new MenuCommand(DRAW_FILL_COMMAND, "Fill area (Ctrl+F)", "DrawFillIcon"));
            _toolbarIcons.Add(new MenuCommand(UNDO_COMMAND, "Undo (Ctrl+Z)", "UndoIcon"));
			_toolbarIcons.Add(new MenuCommand(IMPORT_MASK_COMMAND, "Import mask from file", "ImportMaskIcon"));
            _toolbarIcons.Add(new MenuCommand(EXPORT_MASK_COMMAND, "Export mask to file", "ExportMaskIcon"));
            _toolbarIcons.Add(new MenuCommand(COPY_WALKABLE_AREA_MASK_COMMAND, "Copy walkable area mask to regions", "CopyWalkableAreaMaskIcon"));
			_toolbarIcons.Add(new MenuCommand(GREYED_OUT_MASKS_COMMAND, "Show non-selected masks greyed out", "GreyedOutMasksIcon"));
			_toolbarIcons[(int)_drawMode].Checked = true;
			_toolbarIcons[TOOLBAR_INDEX_GREY_OUT_MASKS].Checked = _greyedOutMasks;

            _room = room;
            _panel = displayPanel;
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
            UpdateUndoButtonEnabledState();
            RoomItemRefs = new SortedDictionary<string, int>();
            DesignItems = new SortedDictionary<string, DesignTimeProperties>();
            InitGameEntities();
        }

        public abstract string Name { get; }
        public abstract string DisplayName { get; }

        public SortedDictionary<string, DesignTimeProperties> DesignItems { get; private set; }
        /// <summary>
        /// A lookup table for getting game object reference by they key.
        /// the keys are area IDs and values are area indexes.
        /// </summary>
        private SortedDictionary<string, int> RoomItemRefs { get; set; }

        public abstract RoomAreaMaskType MaskToDraw
        {
            get;
        }

        public event EventHandler OnItemsChanged;
        public event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;

        public abstract int ItemCount { get; }

		public int SelectedArea
		{
			get { return _selectedArea; }
            set 
            { 
                _selectedArea = value;
                if (OnSelectedItemChanged != null)
                {
                    OnSelectedItemChanged(this, new SelectedRoomItemEventArgs(GetValidItemName(_selectedArea)));
                }
            }
		}

		public string HelpKeyword
		{
			get { return string.Empty; }
		}

		public bool ShowTransparencySlider
		{
			get { return true; }
		}

        public bool SupportVisibleItems { get { return false; } }
        public bool Modified { get; set; }
        public bool Visible { get; set; }
        public bool Locked { get; set; }

        protected virtual void FilterActivated()
		{
		}

        protected bool IsFilterOn()
        {
            return Factory.GUIController.ActivePane.ToolbarCommands == _toolbarIcons;
        }

        public void Invalidate() { _panel.Invalidate(); }

        public void PaintToHDC(IntPtr hDC, RoomEditorState state)
        {
        }

        /// <summary>
        /// Draw hint overlay.
        /// NOTE: this is NOT drawing on actual mask, which is performed when
        /// user releases mouse button.
        /// </summary>
        public virtual void Paint(Graphics graphics, RoomEditorState state)
        {
            int roomPixel = state.RoomSizeToWindow(1);
            int halfRoomPixel = roomPixel / 2;
            if ((_mouseDown) && (_drawMode == AreaDrawMode.Line))
            {
                int penWidth = (int)(roomPixel * GetHintScaleFactor(state));
                Pen pen = GetPenForArea(_drawingWithArea);
                pen = new Pen(pen.Color, penWidth);
                graphics.DrawLine(pen, state.RoomXToWindow(_mouseDownX) + halfRoomPixel,
                    state.RoomYToWindow(_mouseDownY) + halfRoomPixel,
                    state.RoomXToWindow(_currentMouseX) + halfRoomPixel,
                    state.RoomYToWindow(_currentMouseY) + halfRoomPixel);
                pen.Dispose();
            }
			else if ((_mouseDown) && (_drawMode == AreaDrawMode.Rectangle))
			{
                int x1 = state.RoomXToWindow(_mouseDownX);
                int y1 = state.RoomYToWindow(_mouseDownY);
                int x2 = state.RoomXToWindow(_currentMouseX);
                int y2 = state.RoomYToWindow(_currentMouseY);
                EnsureSmallestNumberIsFirst(ref x1, ref x2);
                EnsureSmallestNumberIsFirst(ref y1, ref y2);
                graphics.FillRectangle(GetBrushForArea(_drawingWithArea),
                    x1, y1, x2 - x1 + roomPixel, y2 - y1 + roomPixel);
			}
		}

		private void EnsureSmallestNumberIsFirst(ref int int1, ref int int2)
		{
			if (int1 > int2)
			{
				int temp = int1;
				int1 = int2;
				int2 = temp;
			}
		}

		private Brush GetBrushForArea(int area)
		{
			if (area < _brushesForAreas.Length)
			{
				return _brushesForAreas[area];
			}
			return Brushes.Red;
		}

		protected Pen GetPenForArea(int area)
		{
			if (area < _pensForAreas.Length)
			{
				return _pensForAreas[area];
			}
			return Pens.Red;
		}

        // Gets the scale factor for drawing auxiliary stuff on screen.
        // This does not have any relation to screen/room coordinate conversion,
        // only lets to have extra size for some hint lines etc, in hi-res games.
        // TODO: choose this factor based on game size vs window size relation
        private float GetHintScaleFactor(RoomEditorState state)
        {
            return _room.MaskResolution;
        }

        public void MouseDownAlways(MouseEventArgs e, RoomEditorState state) 
        {
            if (!IsFilterOn())
            {
                DeselectArea();
                _drawingWithArea = 0;
                _drawMode = AreaDrawMode.Select;
            }
        }

        public virtual bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            if (e.Button == MouseButtons.Middle)
            {
                return false;
            }
            
            int x = state.WindowXToRoom(e.X);
            int y = state.WindowYToRoom(e.Y);

            AreaDrawMode drawMode = IsFilterOn() ? _drawMode : AreaDrawMode.Select;

            if (IsLocked(_selectedArea) && drawMode != AreaDrawMode.Select) return false;

            if (drawMode == AreaDrawMode.Freehand)
            {
                Factory.NativeProxy.CreateUndoBuffer(_room, this.MaskToDraw);
            }

			_drawingWithArea = _selectedArea;

			if (e.Button == MouseButtons.Right)
			{
				_drawingWithArea = 0;
			}

            if ((drawMode == AreaDrawMode.Line) ||
                (drawMode == AreaDrawMode.Freehand) ||
                (drawMode == AreaDrawMode.Rectangle))
            {
                if (_selectedArea == 0)
                {
                    _tooltip.Show("You are currently using the eraser. To draw new areas, change the selection in the list above the Properties Pane.", _panel, e.X, e.Y - 70, 2000);
                }
                _mouseDown = true;
                _mouseDownX = x;
                _mouseDownY = y;
                _room.Modified = true;
            }
            else if (drawMode == AreaDrawMode.Fill)
            {
                Factory.NativeProxy.CreateUndoBuffer(_room, this.MaskToDraw);
                Factory.NativeProxy.DrawFillOntoMask(_room, this.MaskToDraw, x, y, _drawingWithArea);
                _room.Modified = true;
                UpdateUndoButtonEnabledState();
            }
            else if (drawMode == AreaDrawMode.Select)
            {
                int area = Factory.NativeProxy.GetAreaMaskPixel(_room, this.MaskToDraw, x, y);                                
                if (area != 0 && !IsLocked(area))
                {
                    SelectedArea = area;
                    SelectedAreaChanged(_selectedArea);
                }
            }
            return true;
        }

        private bool IsLocked(int area)
        {
            return DesignItems[GetItemID(area)].Locked;
        }

        public virtual bool MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            _mouseDown = false;
            AreaDrawMode drawMode = IsFilterOn() ? _drawMode : AreaDrawMode.Select;

            if (IsLocked(_selectedArea) && drawMode != AreaDrawMode.Select) return false;

            if (_shouldSetDrawModeOnMouseUp)
            {
                _shouldSetDrawModeOnMouseUp = false;
                SetDrawMode();
            }
            if (e.Button == MouseButtons.Middle)
            {
                ShowCoordMenu(e, state);
            }
            else if (drawMode == AreaDrawMode.Line)
            {
                Factory.NativeProxy.CreateUndoBuffer(_room, this.MaskToDraw);
                Factory.NativeProxy.DrawLineOntoMask(_room, this.MaskToDraw, _mouseDownX, _mouseDownY, _currentMouseX, _currentMouseY, _drawingWithArea);
                _panel.Invalidate();
                UpdateUndoButtonEnabledState();
            }
            else if (drawMode == AreaDrawMode.Rectangle)
            {
                Factory.NativeProxy.CreateUndoBuffer(_room, this.MaskToDraw);
                Factory.NativeProxy.DrawFilledRectOntoMask(_room, this.MaskToDraw, _mouseDownX, _mouseDownY, _currentMouseX, _currentMouseY, _drawingWithArea);
                _panel.Invalidate();
                UpdateUndoButtonEnabledState();
            }
            else return false;
            return true;
        }

		public virtual bool DoubleClick(RoomEditorState state)
		{
            return false;
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

            _menuClickX = state.WindowXToRoom(e.X);
            _menuClickY = state.WindowYToRoom(e.Y);

            menu.Show(_panel, e.X, e.Y);
        }

        public virtual bool MouseMove(int x, int y, RoomEditorState state)
        {
            _currentMouseX = state.WindowXToRoom(x);
            _currentMouseY = state.WindowYToRoom(y);

            AreaDrawMode drawMode = IsFilterOn() ? _drawMode : AreaDrawMode.Select;            

            if (_mouseDown)
            {
                if (drawMode == AreaDrawMode.Freehand)
                {
					Factory.NativeProxy.DrawLineOntoMask(_room, this.MaskToDraw, _mouseDownX, _mouseDownY, _currentMouseX, _currentMouseY, _drawingWithArea);
                    _mouseDownX = _currentMouseX;
                    _mouseDownY = _currentMouseY;
                    UpdateUndoButtonEnabledState();
                }

                return true;
            }

            return false;
        }

		public bool KeyPressed(Keys key)
		{
			if ((key == (Keys.Control | Keys.Z)) && (_toolbarIcons[TOOLBAR_INDEX_UNDO].Enabled))
			{
				CommandClick(UNDO_COMMAND);
			}
			else if ((key == (Keys.Control | Keys.N)) && (!_mouseDown))
			{
				CommandClick(DRAW_LINE_COMMAND);
			}
			else if ((key == (Keys.Control | Keys.D)) && (!_mouseDown))
			{
				CommandClick(DRAW_FREEHAND_COMMAND);
			}
			else if ((key == (Keys.Control | Keys.F)) && (!_mouseDown))
			{
				CommandClick(DRAW_FILL_COMMAND);
			}
			else if ((key == (Keys.Control | Keys.E)) && (!_mouseDown))
			{
				CommandClick(DRAW_RECTANGLE_COMMAND);
			}
			else if ((key == (Keys.Control | Keys.C)) && (!_mouseDown))
			{
				CommandClick(SELECT_AREA_COMMAND);
			}
            return false;
		}

        public virtual void CommandClick(string command)
        {
            foreach (MenuCommand menuCommand in _toolbarIcons)
            {
				if (menuCommand.ID != GREYED_OUT_MASKS_COMMAND)
				{
					menuCommand.Checked = false;
				}
            }

            if (command == SELECT_AREA_COMMAND)
            {
                _drawMode = AreaDrawMode.Select;
            }
            else if (command == DRAW_LINE_COMMAND)
            {
                _drawMode = AreaDrawMode.Line;
            }
            else if (command == DRAW_FREEHAND_COMMAND)
            {
                _drawMode = AreaDrawMode.Freehand;
            }
			else if (command == DRAW_RECTANGLE_COMMAND)
			{
				_drawMode = AreaDrawMode.Rectangle;
			}
			else if (command == DRAW_FILL_COMMAND)
			{
				_drawMode = AreaDrawMode.Fill;
			}
			else if (command == UNDO_COMMAND)
			{
				Factory.NativeProxy.RestoreFromUndoBuffer(_room, this.MaskToDraw);
				Factory.NativeProxy.ClearUndoBuffer();
				_room.Modified = true;
				_panel.Invalidate();
				UpdateUndoButtonEnabledState();
			}
			else if (command == IMPORT_MASK_COMMAND)
			{
				string fileName = Factory.GUIController.ShowOpenFileDialog("Select mask to import...", Constants.MASK_IMAGE_FILE_FILTER);
				if (fileName != null)
				{
					ImportMaskFromFile(fileName);
				}
			}
            else if (command == EXPORT_MASK_COMMAND)
            {
                string fileName = Factory.GUIController.ShowSaveFileDialog("Save mask as...", Constants.MASK_IMAGE_FILE_FILTER);
                if (fileName != null)
                {
                    ExportMaskFromFile(fileName);
                }
            }
            else if (command == COPY_WALKABLE_AREA_MASK_COMMAND)
			{
				if (Factory.GUIController.ShowQuestion("This will overwrite your Regions mask with a copy of your Walkable Areas mask. Are you sure you want to do this?") == DialogResult.Yes)
				{
					Factory.NativeProxy.CopyWalkableAreaMaskToRegions(_room);
					_room.Modified = true;
					_panel.Invalidate();
				}
			}
			else if (command == GREYED_OUT_MASKS_COMMAND)
			{
				_greyedOutMasks = !_greyedOutMasks;
				_toolbarIcons[TOOLBAR_INDEX_GREY_OUT_MASKS].Checked = _greyedOutMasks;
				Factory.NativeProxy.GreyOutNonSelectedMasks = _greyedOutMasks;
				_panel.Invalidate();
			}

            _toolbarIcons[(int)_drawMode].Checked = true;
            Factory.ToolBarManager.RefreshCurrentPane();
        }

        private void ImportMaskFromFile(string fileName)
        {
            try
            {
                Bitmap bmp = new Bitmap(fileName);

                if (!(((bmp.Width == _room.Width) && (bmp.Height == _room.Height))))
                {
                    Factory.GUIController.ShowMessage("This file cannot be imported because it is not the same size as the room background." +
                        "\nFile size: " + bmp.Width + " x " + bmp.Height +
                        "\nRoom size: " + _room.Width + " x " + _room.Height, MessageBoxIcon.Warning);
                    bmp.Dispose();
                    return;
                }

                if (bmp.PixelFormat != PixelFormat.Format8bppIndexed)
                {
                    Factory.GUIController.ShowMessage("This is not a valid mask bitmap. Masks must be 256-colour (8-bit) images, using the first colours in the palette to draw the room areas.", MessageBoxIcon.Warning);
                    bmp.Dispose();
                    return;
                }

                Factory.NativeProxy.CreateUndoBuffer(_room, this.MaskToDraw);
                Factory.NativeProxy.ImportAreaMask(_room, this.MaskToDraw, bmp);
                bmp.Dispose();
                _room.Modified = true;
                _panel.Invalidate();
                UpdateUndoButtonEnabledState();
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("There was an error importing the area mask. The error was: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private void ExportMaskFromFile(string fileName)
        {
            try
            {
                Bitmap bmp = Factory.NativeProxy.ExportAreaMask(_room, this.MaskToDraw);
                bmp.Save(fileName, ImageFormat.Bmp);
                bmp.Dispose();
            }
            catch (Exception ex)
            {
                Factory.GUIController.ShowMessage("There was an error exporting the area mask. The error was: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private void UpdateUndoButtonEnabledState()
        {
            bool shouldBeEnabled = Factory.NativeProxy.DoesUndoBufferExist();

            if (shouldBeEnabled != _toolbarIcons[TOOLBAR_INDEX_UNDO].Enabled)
            {
                _toolbarIcons[TOOLBAR_INDEX_UNDO].Enabled = shouldBeEnabled;
                Factory.ToolBarManager.RefreshCurrentPane();
            }
        }

        public void FilterOn()
        {
            SetPropertyGridList();
            Factory.GUIController.ActivePane.ToolbarCommands = _toolbarIcons;
            bool hasSelectedCommand = false;
            foreach (MenuCommand menuCommand in _toolbarIcons)
            {
                if (menuCommand.Checked)
                {
                    hasSelectedCommand = true;
                    break;
                }
            }
            if (!hasSelectedCommand) CommandClick(SELECT_AREA_COMMAND);
            else Factory.ToolBarManager.RefreshCurrentPane();

            if (_selectedArea >= ItemCount)
            {
                DeselectArea();
            }
            SelectedAreaChanged(_selectedArea);
            Factory.GUIController.OnPropertyObjectChanged += _propertyObjectChangedDelegate;
            
			FilterActivated();
            _shouldSetDrawModeOnMouseUp = true;            
        }

        public void FilterOff()
        {            
            if (_tooltip.Active)
            {
                _tooltip.Hide(_panel);
            }

            _mouseDown = false;
            Factory.GUIController.OnPropertyObjectChanged -= _propertyObjectChangedDelegate;
            if (Factory.GUIController.ActivePane != null)
            {
                Factory.GUIController.ActivePane.ToolbarCommands = null;
            }
            Factory.ToolBarManager.RefreshCurrentPane();
        }

        public void Dispose()
        {
            _tooltip.Dispose();
        }

        public string GetItemName(string id)
        {
            int area;
            if (id != null && RoomItemRefs.TryGetValue(id, out area))
                return GetValidItemName(area);
            return null;
        }

        public void SelectItem(string id)
        { 
            int area;
            if (id != null && RoomItemRefs.TryGetValue(id, out area))
            {
                _selectedArea = area;
                SelectedAreaChanged(area);                
                return;  
            }
            Factory.GUIController.SetPropertyGridObject(_room);            
        }

        public virtual Cursor GetCursor(int x, int y, RoomEditorState state)
        {
            if (_drawMode != AreaDrawMode.Select)
            {
                return Cursors.Cross;
            }
            if (state.CurrentCursor == null)
            {
                x = state.WindowXToRoom(x);
                y = state.WindowYToRoom(y);
                int area = Factory.NativeProxy.GetAreaMaskPixel(_room, this.MaskToDraw, x, y);
                if (area != 0 && !IsLocked(area))
                {
                    return _selectCursor;
                }
            }
            return null;
        }

        public bool AllowClicksInterception()
        {
            return _drawMode == AreaDrawMode.Select;
        }

        protected void DeselectArea()
        {
            _selectedArea = 0;
        }

        protected string GetItemID(int id)
        {
            // Use numeric area ID as a "unique identifier", for now
            return id.ToString("D4");
        }

        /// <summary>
        /// Gets a human-readable area name.
        /// </summary>
        /// <param name="id"></param>
        protected abstract string GetItemName(int id);
        
        protected string GetValidItemName(int id)
        {
            return id == 0 ? ERASER : GetItemName(id);
        }

        private void SetDrawMode()
        {
            for (int i = 0; i < _toolbarIcons.Count; i++)
            {
                MenuCommand menuCommand = _toolbarIcons[i];
                if (menuCommand.Checked)
                {
                    _drawMode = (AreaDrawMode)i;
                    return;
                }
            }
        }

        /// <summary>
        /// Initialize dictionary of current item references.
        /// </summary>
        /// <returns></returns>
        protected abstract SortedDictionary<string, int> InitItemRefs();
        protected abstract void SetPropertyGridList();
        protected abstract void SelectedAreaChanged(int areaNumber);
        protected abstract void GUIController_OnPropertyObjectChanged(object newPropertyObject);

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
    }
}
