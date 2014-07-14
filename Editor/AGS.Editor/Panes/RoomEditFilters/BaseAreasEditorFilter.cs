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
        protected const string UNDO_COMMAND = "UndoAreaDraw";
        protected const string GREYED_OUT_MASKS_COMMAND = "GreyOutMasks";

        protected const int TOOLBAR_INDEX_UNDO = 5;
        protected const int TOOLBAR_INDEX_GREY_OUT_MASKS = 8;

        private const string MENU_ITEM_COPY_COORDS = "CopyCoordinates";
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
        protected int _selectedArea = 1;
        protected ToolTip _tooltip;
        private int _drawingWithArea;
        private bool _mouseDown = false;
        private int _mouseDownX, _mouseDownY;
        private int _currentMouseX, _currentMouseY;
        private bool _highResMask = false;

        private static AreaDrawMode _drawMode = AreaDrawMode.Line;
        private static List<MenuCommand> _toolbarIcons = null;
        private static bool _registeredIcons = false;
        private static Cursor _selectCursor;
        private static bool _greyedOutMasks = true;

        public BaseAreasEditorFilter(Panel displayPanel, Room room, bool highResMask)
        {
            if (!_registeredIcons)
            {
                Factory.GUIController.RegisterIcon("SelectAreaIcon", Resources.ResourceManager.GetIcon("findarea.ico"));
                Factory.GUIController.RegisterIcon("DrawLineIcon", Resources.ResourceManager.GetIcon("drawline.ico"));
                Factory.GUIController.RegisterIcon("DrawFreehandIcon", Resources.ResourceManager.GetIcon("drawfree.ico"));
                Factory.GUIController.RegisterIcon("DrawRectIcon", Resources.ResourceManager.GetIcon("drawrect.ico"));
                Factory.GUIController.RegisterIcon("DrawFillIcon", Resources.ResourceManager.GetIcon("drawfill.ico"));
                Factory.GUIController.RegisterIcon("ImportMaskIcon", Resources.ResourceManager.GetIcon("importmask.ico"));
                Factory.GUIController.RegisterIcon("CopyWalkableAreaMaskIcon", Resources.ResourceManager.GetIcon("copymask.ico"));
                Factory.GUIController.RegisterIcon("GreyedOutMasksIcon", Resources.ResourceManager.GetIcon("greymasks.ico"));
                _selectCursor = Resources.ResourceManager.GetCursor("findarea.cur");
                _registeredIcons = true;
            }

            _tooltip = new ToolTip();
            _tooltip.IsBalloon = true;
            _highResMask = highResMask;
            _toolbarIcons = new List<MenuCommand>();
            _toolbarIcons.Add(new MenuCommand(SELECT_AREA_COMMAND, "Select area (Ctrl+C)", "SelectAreaIcon"));
            _toolbarIcons.Add(new MenuCommand(DRAW_LINE_COMMAND, "Line tool (Ctrl+N)", "DrawLineIcon"));
            _toolbarIcons.Add(new MenuCommand(DRAW_FREEHAND_COMMAND, "Freehand tool (Ctrl+D)", "DrawFreehandIcon"));
            _toolbarIcons.Add(new MenuCommand(DRAW_RECTANGLE_COMMAND, "Rectangle tool (Ctrl+E)", "DrawRectIcon"));
            _toolbarIcons.Add(new MenuCommand(DRAW_FILL_COMMAND, "Fill area (Ctrl+F)", "DrawFillIcon"));
            _toolbarIcons.Add(new MenuCommand(UNDO_COMMAND, "Undo (Ctrl+Z)", "UndoIcon"));
            _toolbarIcons.Add(new MenuCommand(IMPORT_MASK_COMMAND, "Import mask from file", "ImportMaskIcon"));
            _toolbarIcons.Add(new MenuCommand(COPY_WALKABLE_AREA_MASK_COMMAND, "Copy walkable area mask to regions", "CopyWalkableAreaMaskIcon"));
            _toolbarIcons.Add(new MenuCommand(GREYED_OUT_MASKS_COMMAND, "Show non-selected masks greyed out", "GreyedOutMasksIcon"));
            _toolbarIcons[(int)_drawMode].Checked = true;
            _toolbarIcons[TOOLBAR_INDEX_GREY_OUT_MASKS].Checked = _greyedOutMasks;

            _room = room;
            _panel = displayPanel;
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
            UpdateUndoButtonEnabledState();
        }

        public abstract RoomAreaMaskType MaskToDraw
        {
            get;
        }

        public int SelectedArea
        {
            get { return _selectedArea; }
        }

        public string HelpKeyword
        {
            get { return string.Empty; }
        }

        public bool ShowTransparencySlider
        {
            get { return true; }
        }

        protected virtual void FilterActivated()
        {
        }

        public void PaintToHDC(IntPtr hDC, RoomEditorState state)
        {
        }

        public virtual void Paint(Graphics graphics, RoomEditorState state)
        {
            if ((_mouseDown) && (_drawMode == AreaDrawMode.Line))
            {
                int penWidth = GetScaleFactor(state);
                int extraOffset = penWidth / 2;
                Pen pen = GetPenForArea(_drawingWithArea);
                pen = new Pen(pen.Color, penWidth);
                graphics.DrawLine(pen, GameToScreenX(_mouseDownX, state) + extraOffset,
                    GameToScreenY(_mouseDownY, state) + extraOffset,
                    GameToScreenX(_currentMouseX, state) + extraOffset,
                    GameToScreenY(_currentMouseY, state) + extraOffset);
                pen.Dispose();
            }
            else if ((_mouseDown) && (_drawMode == AreaDrawMode.Rectangle))
            {
                int mousePressedAtX = GameToScreenX(_mouseDownX, state);
                int mousePressedAtY = GameToScreenY(_mouseDownY, state);
                int mouseNowAtX = GameToScreenX(_currentMouseX, state);
                int mouseNowAtY = GameToScreenY(_currentMouseY, state);
                EnsureSmallestNumberIsFirst(ref mousePressedAtX, ref mouseNowAtX);
                EnsureSmallestNumberIsFirst(ref mousePressedAtY, ref mouseNowAtY);
                mouseNowAtX += GetScaleFactor(state) - 1;
                mouseNowAtY += GetScaleFactor(state) - 1;

                graphics.FillRectangle(GetBrushForArea(_drawingWithArea), mousePressedAtX, mousePressedAtY, mouseNowAtX - mousePressedAtX + 1, mouseNowAtY - mousePressedAtY + 1);
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

        private int GetScaleFactor(RoomEditorState state)
        {
            if (_room.Resolution == RoomResolution.HighRes)
            {
                if (_highResMask)
                {
                    return state.ScaleFactor;
                }
                return state.ScaleFactor * 2;
            }
            else if (Factory.AGSEditor.CurrentGame.IsHighResolution)
            {
                // Low-res room in hi-res game
                return state.ScaleFactor * 2;
            }
            else
            {
                return state.ScaleFactor;
            }
        }

        protected int GameToScreenX(int gameX, RoomEditorState state)
        {
            return (gameX * GetScaleFactor(state)) - state.ScrollOffsetX;
        }

        protected int GameToScreenY(int gameY, RoomEditorState state)
        {
            return (gameY * GetScaleFactor(state)) - state.ScrollOffsetY;
        }

        protected int ScreenToGameX(int screenX, RoomEditorState state)
        {
            return (screenX + state.ScrollOffsetX) / GetScaleFactor(state);
        }

        protected int ScreenToGameY(int screenY, RoomEditorState state)
        {
            return (screenY + state.ScrollOffsetY) / GetScaleFactor(state);
        }

        public virtual void MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            if (e.Button == MouseButtons.Middle)
            {
                return;
            }

            int x = ScreenToGameX(e.X, state);
            int y = ScreenToGameY(e.Y, state);

            if (_drawMode == AreaDrawMode.Freehand)
            {
                Factory.NativeProxy.CreateUndoBuffer(_room, this.MaskToDraw);
            }

            _drawingWithArea = _selectedArea;

            if (e.Button == MouseButtons.Right)
            {
                _drawingWithArea = 0;
            }

            if ((_drawMode == AreaDrawMode.Line) ||
                (_drawMode == AreaDrawMode.Freehand) ||
                (_drawMode == AreaDrawMode.Rectangle))
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
            else if (_drawMode == AreaDrawMode.Fill)
            {
                Factory.NativeProxy.CreateUndoBuffer(_room, this.MaskToDraw);
                Factory.NativeProxy.DrawFillOntoMask(_room, this.MaskToDraw, x, y, _drawingWithArea);
                _room.Modified = true;
                UpdateUndoButtonEnabledState();
            }
            else if (_drawMode == AreaDrawMode.Select)
            {
                _selectedArea = Factory.NativeProxy.GetAreaMaskPixel(_room, this.MaskToDraw, x, y);
                SelectedAreaChanged(_selectedArea);
            }
        }

        public virtual void MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            _mouseDown = false;
            if (e.Button == MouseButtons.Middle)
            {
                ShowCoordMenu(e, state);
            }
            else if (_drawMode == AreaDrawMode.Line)
            {
                Factory.NativeProxy.CreateUndoBuffer(_room, this.MaskToDraw);
                Factory.NativeProxy.DrawLineOntoMask(_room, this.MaskToDraw, _mouseDownX, _mouseDownY, _currentMouseX, _currentMouseY, _drawingWithArea);
                _panel.Invalidate();
                UpdateUndoButtonEnabledState();
            }
            else if (_drawMode == AreaDrawMode.Rectangle)
            {
                Factory.NativeProxy.CreateUndoBuffer(_room, this.MaskToDraw);
                Factory.NativeProxy.DrawFilledRectOntoMask(_room, this.MaskToDraw, _mouseDownX, _mouseDownY, _currentMouseX, _currentMouseY, _drawingWithArea);
                _panel.Invalidate();
                UpdateUndoButtonEnabledState();
            }
        }

        public virtual void DoubleClick(RoomEditorState state)
        {
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

            _menuClickX = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
            _menuClickY = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;

            menu.Show(_panel, e.X, e.Y);
        }

        public virtual bool MouseMove(int x, int y, RoomEditorState state)
        {
            _currentMouseX = ScreenToGameX(x, state);
            _currentMouseY = ScreenToGameY(y, state);

            if (_drawMode == AreaDrawMode.Select)
            {
                state.CurrentCursor = _selectCursor;
            }
            else
            {
                state.CurrentCursor = Cursors.Cross;
            }

            if (_mouseDown)
            {
                if (_drawMode == AreaDrawMode.Freehand)
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
                string fileName = Factory.GUIController.ShowOpenFileDialog("Select mask to import...", GUIController.IMAGE_FILE_FILTER);
                if (fileName != null)
                {
                    ImportMaskFromFile(fileName);
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

                if (!(((bmp.Width == _room.Width) && (bmp.Height == _room.Height)) ||
                    ((bmp.Width == _room.Width / 2) && (bmp.Height == _room.Height / 2))))
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
            CommandClick(SELECT_AREA_COMMAND);
            SelectedAreaChanged(_selectedArea);
            Factory.GUIController.OnPropertyObjectChanged += _propertyObjectChangedDelegate;

            FilterActivated();
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

        protected abstract void SetPropertyGridList();
        protected abstract void SelectedAreaChanged(int areaNumber);
        protected abstract void GUIController_OnPropertyObjectChanged(object newPropertyObject);
    }

}
