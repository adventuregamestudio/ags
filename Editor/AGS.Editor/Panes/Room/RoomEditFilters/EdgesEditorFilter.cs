using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class EdgesEditorFilter : IRoomEditorFilter
    {
        private enum SelectedEdge
        {
            None,
            Left,
            Right,
            Top,
            Bottom
        }

        private Room _room;
        private bool _isOn = false;
        private SelectedEdge _selectedEdge = SelectedEdge.None;
        private SelectedEdge _lastSelectedEdge = SelectedEdge.None;
        private Panel _panel;
        private ToolTip _tooltip;
        private int _tooltipX = -100, _tooltipY = -100;
        private string _tooltipText = null;
        private bool _mouseDown;

        public EdgesEditorFilter(Panel displayPanel, Room room)
        {
            _room = room;
            _panel = displayPanel;
            _tooltip = new ToolTip();
            RoomItemRefs = new SortedDictionary<string, SelectedEdge>();
            DesignItems = new SortedDictionary<string, DesignTimeProperties>();
            InitGameEntities();
        }

        public string Name { get { return "Edges"; } }
        public string DisplayName { get { return "Edges"; } }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public SortedDictionary<string, DesignTimeProperties> DesignItems { get; private set; }
        /// <summary>
        /// A lookup table for getting game object reference by they key.
        /// </summary>
        private SortedDictionary<string, SelectedEdge> RoomItemRefs { get; set; }

        public bool SupportVisibleItems { get { return true; } }
        public bool Modified { get; set; }
        public bool Visible { get; set; }
        public bool Locked { get; set; }
        public bool Enabled { get { return _isOn; } }

        public event EventHandler OnItemsChanged { add { } remove { } }
        public event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;
        public event EventHandler<RoomFilterContextMenuArgs> OnContextMenu { add { } remove { } }

        public int ItemCount
        {
            get { return 4; }
        }

        public int SelectedArea
		{
			get { return 0; }
		}

		public bool ShowTransparencySlider
		{
			get { return false; }
		}

        private void DrawEdge(Graphics graphics, int position, float scale, SelectedEdge edge)
        {
            PointF location = new PointF();
            SizeF size = new SizeF();

            switch(edge)
            {
                case SelectedEdge.Left:
                case SelectedEdge.Right:
                    location.X = position;
                    location.Y = 0;
                    size.Width = scale;
                    size.Height = _room.Height * scale;
                    break;
                case SelectedEdge.Top:
                case SelectedEdge.Bottom:
                    location.X = 0;
                    location.Y = position;
                    size.Width = _room.Width * scale;
                    size.Height = scale;
                    break;
            }

            RectangleF rect = new RectangleF(location, size);
            Brush brush = new HatchBrush(HatchStyle.Percent50, Color.Yellow, Color.Transparent);
            graphics.FillRectangle(brush, rect);
        }

        public void Invalidate() { _panel.Invalidate(); }

		public bool KeyPressed(Keys key)
		{
            switch (key)
            {
                case Keys.Right:
                    return MoveEdgeWithKeyboard(1, 0);
                case Keys.Left:
                    return MoveEdgeWithKeyboard(-1, 0);
                case Keys.Down:
                    return MoveEdgeWithKeyboard(0, 1);
                case Keys.Up:
                    return MoveEdgeWithKeyboard(0, -1);
            }
            return false;
		}

        public bool KeyReleased(Keys key)
        {
            return false;
        }

        public void Paint(Graphics graphics, RoomEditorState state)
        {
            if (DesignItems[GetItemID(SelectedEdge.Left)].Visible)
                DrawEdge(graphics, state.RoomXToWindow(_room.LeftEdgeX), state.Scale, SelectedEdge.Left);
            if (DesignItems[GetItemID(SelectedEdge.Right)].Visible)
                DrawEdge(graphics, state.RoomXToWindow(_room.RightEdgeX), state.Scale, SelectedEdge.Right);
            if (DesignItems[GetItemID(SelectedEdge.Top)].Visible)
                DrawEdge(graphics, state.RoomYToWindow(_room.TopEdgeY), state.Scale, SelectedEdge.Top);
            if (DesignItems[GetItemID(SelectedEdge.Bottom)].Visible)
                DrawEdge(graphics, state.RoomYToWindow(_room.BottomEdgeY), state.Scale, SelectedEdge.Bottom);
        }

        public bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            if (e.Button == MouseButtons.Middle) return false;

            _mouseDown = true;
            int x = state.WindowXToRoom(e.X);
            int y = state.WindowYToRoom(e.Y);

            if (IsCursorOnVerticalEdge(x, _room.LeftEdgeX, SelectedEdge.Left) && SetSelectedEdge(SelectedEdge.Left)) {}            
            else if (IsCursorOnVerticalEdge(x, _room.RightEdgeX, SelectedEdge.Right) && SetSelectedEdge(SelectedEdge.Right)) {}            
            else if (IsCursorOnHorizontalEdge(y, _room.TopEdgeY, SelectedEdge.Top) && SetSelectedEdge(SelectedEdge.Top)) {}
            else if (IsCursorOnHorizontalEdge(y, _room.BottomEdgeY, SelectedEdge.Bottom) && SetSelectedEdge(SelectedEdge.Bottom)) {}                        
            else _selectedEdge = SelectedEdge.None;

            _lastSelectedEdge = _selectedEdge;
            return _selectedEdge != SelectedEdge.None;
        }

        public bool MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            _mouseDown = false;
            _selectedEdge = SelectedEdge.None;
            return false;
        }

		public bool DoubleClick(RoomEditorState state)
		{
            return false;
		}

        public bool MouseMove(int x, int y, RoomEditorState state)
        {
            if (_selectedEdge != SelectedEdge.None && _mouseDown)
            {
                x = state.WindowXToRoom(x);
                y = state.WindowYToRoom(y);
                MoveEdgeWithMouse(x, y);
                _room.Modified = true;
                return true;
            }

            return false;
        }

        public void FilterOn()
        {
            _isOn = true;
        }

        public void FilterOff()
        {
            if (_tooltip.Active)
            {
                _tooltip.Hide(_panel);
            }
            _isOn = false;
        }

		public string HelpKeyword
		{
			get { return string.Empty; }
		}

        private bool MoveEdgeWithKeyboard(int offsetX, int offsetY)
        {
            switch (_lastSelectedEdge)
            {
                case SelectedEdge.Left:
                    if (offsetX == 0) return false;
                    MoveLeftEdge(_room.LeftEdgeX + offsetX);
                    break;
                case SelectedEdge.Right:
                    if (offsetX == 0) return false;
                    MoveRightEdge(_room.RightEdgeX + offsetX);
                    break;
                case SelectedEdge.Top:
                    if (offsetY == 0) return false;
                    MoveTopEdge(_room.TopEdgeY + offsetY);
                    break;
                case SelectedEdge.Bottom:
                    if (offsetY == 0) return false;
                    MoveBottomEdge(_room.BottomEdgeY + offsetY);
                    break;
                default:
                    return false;
            }
            return true;
        }

        private void MoveEdgeWithMouse(int x, int y)
        {
            switch (_selectedEdge)
            {
                case SelectedEdge.Left:
                    MoveLeftEdge(x);
                    break;
                case SelectedEdge.Right:
                    MoveRightEdge(x);
                    break;
                case SelectedEdge.Top:
                    MoveTopEdge(y);
                    break;
                case SelectedEdge.Bottom:
                    MoveBottomEdge(y);
                    break;
            }
        }

        private void MoveLeftEdge(int x)
        {
            x = Math.Min(x, _room.RightEdgeX - 1);
            x = Math.Max(x, 0);
            _room.LeftEdgeX = x; 
        }

        private void MoveRightEdge(int x)
        {
            x = Math.Max(x, _room.LeftEdgeX + 1);
            x = Math.Min(x, _room.Width - 1);
            _room.RightEdgeX = x; 
        }

        private void MoveTopEdge(int y)
        {
            y = Math.Min(y, _room.BottomEdgeY - 1);
            y = Math.Max(y, 0);
            _room.TopEdgeY = y;
        }

        private void MoveBottomEdge(int y)
        {
            y = Math.Max(y, _room.TopEdgeY + 1);
            y = Math.Min(y, _room.Height - 1);
            _room.BottomEdgeY = y;
        }

        private bool IsCursorOnVerticalEdge(int cursorX, int edgeX, SelectedEdge edge)
        {
            return ((cursorX >= edgeX - 1) && (cursorX <= edgeX + 1));
        }

        private bool IsCursorOnHorizontalEdge(int cursorY, int edgeY, SelectedEdge edge)
        {
            return ((cursorY >= edgeY - 1) && (cursorY <= edgeY + 1));
        }

        private bool IsMoveable(SelectedEdge edge)
        {
            if (Locked) return false;
            DesignTimeProperties p = DesignItems[GetItemID(edge)];
            if (!p.Visible || p.Locked) return false;
            return true;
        }

        public void CommandClick(string command)
        {
        }

        public void Dispose()
        {
            _tooltip.Dispose();
        }

        private string GetItemID(SelectedEdge e)
        {
            // Use edge's name as a "unique identifier", for now
            return e.ToString();
        }

        /// <summary>
        /// Initialize dictionary of current item references.
        /// </summary>
        /// <returns></returns>
        private SortedDictionary<string, SelectedEdge> InitItemRefs()
        {
            return new SortedDictionary<string, SelectedEdge> {
                { GetItemID(SelectedEdge.Left), SelectedEdge.Left },
                { GetItemID(SelectedEdge.Right), SelectedEdge.Right },
                { GetItemID(SelectedEdge.Top), SelectedEdge.Top },
                { GetItemID(SelectedEdge.Bottom), SelectedEdge.Bottom }
            };
        }

        public string GetItemName(string id)
        {
            SelectedEdge edge;
            if (id != null && RoomItemRefs.TryGetValue(id, out edge))
                return edge.ToString();
            return null;
        }

        public void SelectItem(string id)
        {
            SelectedEdge edge;
            if (id == null || !RoomItemRefs.TryGetValue(id, out edge))
            {
                _selectedEdge = SelectedEdge.None;                
                return;
            }
            _selectedEdge = edge;
            _lastSelectedEdge = _selectedEdge;
        }

        public Cursor GetCursor(int x, int y, RoomEditorState state)
        {
            int roomX = state.WindowXToRoom(x);
            int roomY = state.WindowYToRoom(y);
            string toolTipText = null;
            Cursor cursor = null;
            SelectedEdge draggingEdge = _mouseDown ? _selectedEdge : SelectedEdge.None;
            if (draggingEdge == SelectedEdge.Left ||
                IsCursorOnVerticalEdge(roomX, _room.LeftEdgeX, SelectedEdge.Left))
            {
                cursor = IsMoveable(SelectedEdge.Left) ? Cursors.VSplit : RoomSettingsEditor.LockedCursor;
                toolTipText = "Left edge";
            }
            else if (draggingEdge == SelectedEdge.Right || 
                IsCursorOnVerticalEdge(roomX, _room.RightEdgeX, SelectedEdge.Right))
            {
                cursor = IsMoveable(SelectedEdge.Right) ? Cursors.VSplit : RoomSettingsEditor.LockedCursor;
                toolTipText = "Right edge";
            }
            else if (draggingEdge == SelectedEdge.Top || 
                IsCursorOnHorizontalEdge(roomY, _room.TopEdgeY, SelectedEdge.Top))
            {
                cursor = IsMoveable(SelectedEdge.Top) ? Cursors.HSplit : RoomSettingsEditor.LockedCursor;
                toolTipText = "Top edge";
            }
            else if (draggingEdge == SelectedEdge.Bottom || 
                IsCursorOnHorizontalEdge(roomY, _room.BottomEdgeY, SelectedEdge.Bottom))
            {
                cursor = IsMoveable(SelectedEdge.Bottom) ? Cursors.HSplit : RoomSettingsEditor.LockedCursor;
                toolTipText = "Bottom edge";
            }
            else
            {
                cursor = null;
            }

            if (toolTipText != null)
            {
                // Tooltip.Show is quite a slow function, so make sure it's not
                // called too often
                if ((Math.Abs(x - _tooltipX) > 5) || (Math.Abs(y - _tooltipY) > 5) ||
                    (_tooltipText != toolTipText) || (!_tooltip.Active))
                {
                    _tooltip.Show(toolTipText, _panel, x - 10, y + 5);
                    _tooltipX = x;
                    _tooltipY = y;
                    _tooltipText = toolTipText;
                }
            }
            else if (_tooltip.Active)
            {
                _tooltip.Hide(_panel);
            }
            return cursor;
        }

        public bool AllowClicksInterception()
        {
            return true;
        }

        private bool SetSelectedEdge(SelectedEdge edge)
        {
            if (!IsMoveable(edge)) return false;
            _selectedEdge = edge;
            if (OnSelectedItemChanged != null)
            {
                OnSelectedItemChanged(this, new SelectedRoomItemEventArgs(edge.ToString()));
            }
            return true;
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
    }
}
