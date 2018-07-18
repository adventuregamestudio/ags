using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
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
            VisibleItems = new List<string>();
            LockedItems = new List<string>();
        }

        public string DisplayName { get { return "Edges"; } }

        public bool VisibleByDefault { get { return false; } }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public List<string> VisibleItems { get; private set; }
        public List<string> LockedItems { get; private set; }

        public bool SupportVisibleItems { get { return true; } }

        public event EventHandler OnItemsChanged { add { } remove { } }
        public event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;

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

        private void DrawDoubleWidthVerticalLine(Graphics graphics, int x, float scale)
        {
            graphics.DrawLine(Pens.Yellow, x, 0, x, _room.Height * scale);
            graphics.DrawLine(Pens.Yellow, x + 1, 0, x + 1, _room.Height * scale);
        }

        private void DrawDoubleHeightHorizontalLine(Graphics graphics, int y, float scale)
        {
            graphics.DrawLine(Pens.Yellow, 0, y, _room.Width * scale, y);
            graphics.DrawLine(Pens.Yellow, 0, y + 1, _room.Width * scale, y + 1);
        }

        public void Invalidate() { _panel.Invalidate(); }

        public void PaintToHDC(IntPtr hDC, RoomEditorState state)
        {
        }

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

        public void Paint(Graphics graphics, RoomEditorState state)
        {
            float scale = state.Scale;

            if (VisibleItems.Contains(SelectedEdge.Left.ToString())) 
                DrawDoubleWidthVerticalLine(graphics, state.RoomXToWindow(_room.LeftEdgeX), scale);
            if (VisibleItems.Contains(SelectedEdge.Right.ToString()))
                DrawDoubleWidthVerticalLine(graphics, state.RoomXToWindow(_room.RightEdgeX), scale);
            if (VisibleItems.Contains(SelectedEdge.Top.ToString()))
                DrawDoubleHeightHorizontalLine(graphics, state.RoomYToWindow(_room.TopEdgeY), scale);
            if (VisibleItems.Contains(SelectedEdge.Bottom.ToString()))
                DrawDoubleHeightHorizontalLine(graphics, state.RoomYToWindow(_room.BottomEdgeY), scale);
        }

        public void MouseDownAlways(MouseEventArgs e, RoomEditorState state)
        {
            _selectedEdge = SelectedEdge.None;
        }

        public bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            _mouseDown = true;
            int x = state.WindowXToRoom(e.X);
            int y = state.WindowYToRoom(e.Y);

            if (IsCursorOnVerticalEdge(x, _room.LeftEdgeX, SelectedEdge.Left) && SetSelectedEdge(SelectedEdge.Left)) {}            
            else if (IsCursorOnVerticalEdge(x, _room.RightEdgeX, SelectedEdge.Right) && SetSelectedEdge(SelectedEdge.Right)) {}            
            else if (IsCursorOnHorizontalEdge(y, _room.TopEdgeY, SelectedEdge.Top) && SetSelectedEdge(SelectedEdge.Top)) {}
            else if (IsCursorOnHorizontalEdge(y, _room.BottomEdgeY, SelectedEdge.Bottom) && SetSelectedEdge(SelectedEdge.Bottom)) {}                        

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

        }

        public void FilterOff()
        {
            if (_tooltip.Active)
            {
                _tooltip.Hide(_panel);
            }
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
            if (!IsMoveable(edge)) return false;
            return ((cursorX >= edgeX - 1) && (cursorX <= edgeX + 1));
        }

        private bool IsCursorOnHorizontalEdge(int cursorY, int edgeY, SelectedEdge edge)
        {
            if (!IsMoveable(edge)) return false;
            return ((cursorY >= edgeY - 1) && (cursorY <= edgeY + 1));
        }

        private bool IsMoveable(SelectedEdge edge)
        {
            if (!VisibleItems.Contains(edge.ToString()) || LockedItems.Contains(edge.ToString())) return false;
            return true;
        }

        public void CommandClick(string command)
        {
        }

        public void Dispose()
        {
            _tooltip.Dispose();
        }

        public List<string> GetItemsNames()
        {
            return new List<string> { SelectedEdge.Left.ToString(), SelectedEdge.Right.ToString(),
                SelectedEdge.Top.ToString(), SelectedEdge.Bottom.ToString()};
        }

        public void SelectItem(string name)
        {
            if (name == SelectedEdge.Bottom.ToString()) _selectedEdge = SelectedEdge.Bottom;
            else if (name == SelectedEdge.Top.ToString()) _selectedEdge = SelectedEdge.Top;
            else if (name == SelectedEdge.Right.ToString()) _selectedEdge = SelectedEdge.Right;
            else if (name == SelectedEdge.Left.ToString()) _selectedEdge = SelectedEdge.Left;
            else
            {
                _selectedEdge = SelectedEdge.None;                
                return;
            }
            _lastSelectedEdge = _selectedEdge;            
        }

        public Cursor GetCursor(int x, int y, RoomEditorState state)
        {
            int roomX = state.WindowXToRoom(x);
            int roomY = state.WindowYToRoom(y);
            string toolTipText = null;
            Cursor cursor = null;
            if (IsCursorOnVerticalEdge(roomX, _room.LeftEdgeX, SelectedEdge.Left))
            {
                cursor = Cursors.VSplit;
                toolTipText = "Left edge";
            }
            else if (IsCursorOnVerticalEdge(roomX, _room.RightEdgeX, SelectedEdge.Right))
            {
                cursor = Cursors.VSplit;
                toolTipText = "Right edge";
            }
            else if (IsCursorOnHorizontalEdge(roomY, _room.TopEdgeY, SelectedEdge.Top))
            {
                cursor = Cursors.HSplit;
                toolTipText = "Top edge";
            }
            else if (IsCursorOnHorizontalEdge(roomY, _room.BottomEdgeY, SelectedEdge.Bottom))
            {
                cursor = Cursors.HSplit;
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
    }

}
