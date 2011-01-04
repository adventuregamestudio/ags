using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class HotspotsEditorFilter : IRoomEditorFilter
    {
        private const string DRAW_LINE_COMMAND = "DrawLine";
        private const string DRAW_FREEHAND_COMMAND = "DrawFreehand";
        private const string DRAW_FILL_COMMAND = "DrawFill";

        private GUIController.PropertyObjectChangedHandler _propertyObjectChangedDelegate;
        private Room _room;
        private Panel _panel;
        private ImageAttributes _alphaTransparentAttr;
        private bool _mouseDown = false;
        private int _mouseDownX, _mouseDownY;
        private int _currentMouseX, _currentMouseY;
        private int _selectedArea = 0;

        private static AreaDrawMode _drawMode = AreaDrawMode.Line;
        private static List<MenuCommand> _toolbarIcons = null;
        private static bool _registeredIcons = false;

        public HotspotsEditorFilter(Panel displayPanel, Room room)
        {
            if (!_registeredIcons)
            {
                Factory.GUIController.RegisterIcon("DrawLineIcon", Resources.ResourceManager.GetIcon("drawline.ico"));
                Factory.GUIController.RegisterIcon("DrawFreehandIcon", Resources.ResourceManager.GetIcon("drawfree.ico"));
                Factory.GUIController.RegisterIcon("DrawFillIcon", Resources.ResourceManager.GetIcon("drawfill.ico"));
                _registeredIcons = true;
                _toolbarIcons = new List<MenuCommand>();
                _toolbarIcons.Add(new MenuCommand(DRAW_LINE_COMMAND, "Line tool", "DrawLineIcon"));
                _toolbarIcons.Add(new MenuCommand(DRAW_FREEHAND_COMMAND, "Freehand tool", "DrawFreehandIcon"));
                _toolbarIcons.Add(new MenuCommand(DRAW_FILL_COMMAND, "Fill area", "DrawFillIcon"));
                _toolbarIcons[0].Checked = true;
            }

            _room = room;
            _panel = displayPanel;
            CreateAlphaTranslucentImageMatrix();
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
        }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.Hotspots; }
        }

        private void CreateAlphaTranslucentImageMatrix()
        {
            // Initialize the color matrix.
            // Note the alpha value in row 4, column 4.
            float[][] matrixItems ={
                new float[] {1, 0, 0, 0, 0}, // R
                new float[] {0, 1, 0, 0, 0}, // G
                new float[] {0, 0, 1, 0, 0}, // B
                new float[] {0, 0, 0, 0.75f, 0}, // A
                new float[] {0, 0, 0, 0, 1}}; // W (?) 

            ColorMatrix colorMatrix = new ColorMatrix(matrixItems);

            // Create an ImageAttributes object and set its color matrix.
            _alphaTransparentAttr = new ImageAttributes();
            _alphaTransparentAttr.SetColorMatrix(colorMatrix, ColorMatrixFlag.Default, ColorAdjustType.Bitmap);
        }

        public void PaintToHDC(IntPtr hDC, RoomEditorState state)
        {
        }

        public void Paint(Graphics graphics, RoomEditorState state)
        {
            if ((_mouseDown) && (_drawMode == AreaDrawMode.Line))
            {
                graphics.DrawLine(Pens.Blue, GameToScreenX(_mouseDownX, state), GameToScreenY(_mouseDownY, state), GameToScreenX(_currentMouseX, state), GameToScreenY(_currentMouseY, state));
            }

            foreach (RoomHotspot hotspot in _room.Hotspots)
            {
                if ((hotspot.WalkToPoint.X > 0) && (hotspot.WalkToPoint.Y > 0))
                {
                    int x = GameToScreenX(hotspot.WalkToPoint.X, state);
                    int y = GameToScreenY(hotspot.WalkToPoint.Y, state);
                    graphics.DrawLine(Pens.Red, x - 4, y - 4, x + 4, y + 4);
                    graphics.DrawLine(Pens.RosyBrown, x - 4, y + 4, x + 4, y - 4);
                    graphics.DrawString(hotspot.ID.ToString(), new System.Drawing.Font(FontFamily.GenericSansSerif, 10, FontStyle.Bold), Brushes.Gold, x + 4, y - 7);
                }
            }
        }

        private int GameToScreenX(int gameX, RoomEditorState state)
        {
            return (gameX * state.ScaleFactor) - state.ScrollOffsetX;
        }

        private int GameToScreenY(int gameY, RoomEditorState state)
        {
            return (gameY * state.ScaleFactor) - state.ScrollOffsetY;
        }

        public void MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            int x = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
            int y = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;

            if ((_drawMode == AreaDrawMode.Line) || (_drawMode == AreaDrawMode.Freehand))
            {
                _mouseDown = true;
                _mouseDownX = x;
                _mouseDownY = y;
            }
            else if (_drawMode == AreaDrawMode.Fill)
            {
                NativeProxy.Instance.DrawFillOntoMask(_room, this.MaskToDraw, x, y, _selectedArea);
            }
        }

        public void MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            _mouseDown = false;
            if (_drawMode == AreaDrawMode.Line)
            {
/*                Graphics g = Graphics.FromImage(_room.HotspotMask);
                g.DrawLine(new Pen(_room.HotspotMask.Palette.Entries[_selectedArea]), _mouseDownX, _mouseDownY, _currentMouseX, _currentMouseY);
                g.Dispose();*/
                NativeProxy.Instance.DrawLineOntoMask(_room, this.MaskToDraw, _mouseDownX, _mouseDownY, _currentMouseX, _currentMouseY, _selectedArea);
                _panel.Invalidate();
            }
            
        }

        public bool MouseMove(int x, int y, RoomEditorState state)
        {
            _currentMouseX = (x + state.ScrollOffsetX) / state.ScaleFactor;
            _currentMouseY = (y + state.ScrollOffsetY) / state.ScaleFactor;

            state.CurrentCursor = Cursors.Cross;

            if (_mouseDown)
            {
                if (_drawMode == AreaDrawMode.Freehand)
                {
                    NativeProxy.Instance.DrawLineOntoMask(_room, this.MaskToDraw, _mouseDownX, _mouseDownY, _currentMouseX, _currentMouseY, _selectedArea);
                    _mouseDownX = _currentMouseX;
                    _mouseDownY = _currentMouseY;
                }

                return true;
            }

            return false;
        }

        public void CommandClick(string command)
        {
            foreach (MenuCommand menuCommand in _toolbarIcons)
            {
                menuCommand.Checked = false;
            }

            if (command == DRAW_LINE_COMMAND)
            {
                _drawMode = AreaDrawMode.Line;
            }
            else if (command == DRAW_FREEHAND_COMMAND)
            {
                _drawMode = AreaDrawMode.Freehand;
            }
            else if (command == DRAW_FILL_COMMAND)
            {
                _drawMode = AreaDrawMode.Fill;
            }

            _toolbarIcons[(int)_drawMode].Checked = true;
            Factory.ToolBarManager.RefreshCurrentPane();
        }

        public void FilterOn()
        {
            SetPropertyGridList();
            Factory.GUIController.ActivePane.ToolbarCommands = _toolbarIcons;
            Factory.ToolBarManager.RefreshCurrentPane();
            GUIController.Instance.OnPropertyObjectChanged += _propertyObjectChangedDelegate;
        }

        public void FilterOff()
        {
            _mouseDown = false;
            GUIController.Instance.OnPropertyObjectChanged -= _propertyObjectChangedDelegate;
            if (Factory.GUIController.ActivePane != null)
            {
                Factory.GUIController.ActivePane.ToolbarCommands = null;
            }
            Factory.ToolBarManager.RefreshCurrentPane();
        }

        public void Dispose()
        {
        }

        private void SetPropertyGridList()
        {
            Dictionary<string, object> defaultPropertyObjectList = new Dictionary<string, object>();
            defaultPropertyObjectList.Add(_room.PropertyGridTitle, _room);
            foreach (RoomHotspot hotspot in _room.Hotspots)
            {
                defaultPropertyObjectList.Add(hotspot.PropertyGridTitle, hotspot);
            }

            GUIController.Instance.SetPropertyGridObjectList(defaultPropertyObjectList);
        }

        private void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is RoomHotspot)
            {
                _selectedArea = ((RoomHotspot)newPropertyObject).ID;
                _panel.Invalidate();
            }
            else if (newPropertyObject is Room)
            {
                _selectedArea = 0;
                _panel.Invalidate();
            }
        }
    }

}
