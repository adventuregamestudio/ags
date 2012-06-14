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

        public EdgesEditorFilter(Panel displayPanel, Room room)
        {
            _room = room;
            _panel = displayPanel;
            _tooltip = new ToolTip();
        }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

		public int SelectedArea
		{
			get { return 0; }
		}

		public bool ShowTransparencySlider
		{
			get { return false; }
		}

        private void DrawDoubleWidthVerticalLine(Graphics graphics, int x, int scaleFactor)
        {
            graphics.DrawLine(Pens.Yellow, x, 0, x, _room.Height * scaleFactor);
            graphics.DrawLine(Pens.Yellow, x + 1, 0, x + 1, _room.Height * scaleFactor);
        }

        private void DrawDoubleHeightHorizontalLine(Graphics graphics, int y, int scaleFactor)
        {
            graphics.DrawLine(Pens.Yellow, 0, y, _room.Width * scaleFactor, y);
            graphics.DrawLine(Pens.Yellow, 0, y + 1, _room.Width * scaleFactor, y + 1);
        }

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
            int scaleFactor = state.ScaleFactor;
            DrawDoubleWidthVerticalLine(graphics, _room.LeftEdgeX * scaleFactor - state.ScrollOffsetX, scaleFactor);
            DrawDoubleWidthVerticalLine(graphics, _room.RightEdgeX * scaleFactor - state.ScrollOffsetX, scaleFactor);
            DrawDoubleHeightHorizontalLine(graphics, _room.TopEdgeY * scaleFactor - state.ScrollOffsetY, scaleFactor);
            DrawDoubleHeightHorizontalLine(graphics, _room.BottomEdgeY * scaleFactor - state.ScrollOffsetY, scaleFactor);
        }

        public void MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            int x = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
            int y = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;
            if (IsCursorOnVerticalEdge(x, _room.LeftEdgeX))
            {
                _selectedEdge = SelectedEdge.Left;
            }
            else if (IsCursorOnVerticalEdge(x, _room.RightEdgeX))
            {
                _selectedEdge = SelectedEdge.Right;
            }
            else if (IsCursorOnHorizontalEdge(y, _room.TopEdgeY))
            {
                _selectedEdge = SelectedEdge.Top;
            }
            else if (IsCursorOnHorizontalEdge(y, _room.BottomEdgeY))
            {
                _selectedEdge = SelectedEdge.Bottom;
            }
            else
            {
                _selectedEdge = SelectedEdge.None;
            }
            _lastSelectedEdge = _selectedEdge;
        }

        public void MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            _selectedEdge = SelectedEdge.None;
        }

		public void DoubleClick(RoomEditorState state)
		{
		}

        public bool MouseMove(int x, int y, RoomEditorState state)
        {
            x += state.ScrollOffsetX;
            y += state.ScrollOffsetY;
            int scaleFactor = state.ScaleFactor;
            bool refresh = false;
            if (_selectedEdge != SelectedEdge.None)
            {
                MoveEdgeWithMouse(x / scaleFactor, y / scaleFactor);
                _room.Modified = true;
                refresh = true;
            }

            string toolTipText = null;
            if (IsCursorOnVerticalEdge(x / scaleFactor, _room.LeftEdgeX))
            {
                state.CurrentCursor = Cursors.VSplit;
                toolTipText = "Left edge";
            }
            else if (IsCursorOnVerticalEdge(x / scaleFactor, _room.RightEdgeX))
            {
                state.CurrentCursor = Cursors.VSplit;
                toolTipText = "Right edge";
            }
            else if (IsCursorOnHorizontalEdge(y / scaleFactor, _room.TopEdgeY))
            {
                state.CurrentCursor = Cursors.HSplit;
                toolTipText = "Top edge";
            }
            else if (IsCursorOnHorizontalEdge(y / scaleFactor, _room.BottomEdgeY))
            {
                state.CurrentCursor = Cursors.HSplit;
                toolTipText = "Bottom edge";
            }
            else
            {
                state.CurrentCursor = Cursors.Default;
            }

            if (toolTipText != null)
            {
                // Tooltip.Show is quite a slow function, so make sure it's not
                // called too often
                if ((Math.Abs(x - _tooltipX) > 5) || (Math.Abs(y - _tooltipY) > 5) ||
                    (_tooltipText != toolTipText) || (!_tooltip.Active))
                {
                    _tooltip.Show(toolTipText, _panel, (x - state.ScrollOffsetX) - 10, (y - state.ScrollOffsetY) + 5);
                    _tooltipX = x;
                    _tooltipY = y;
                    _tooltipText = toolTipText;
                }
            }
            else if (_tooltip.Active)
            {
                _tooltip.Hide(_panel);
            }
            return refresh;
        }

        public void FilterOn()
        {
			Factory.GUIController.ShowCuppit("The room edges set the point at which the Player Walks Off Edge events will be triggered, and are the normal way to go from one room to another. They also act as limits to where AGS will automatically move the character.", "Edges introduction");
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

        private bool IsCursorOnVerticalEdge(int cursorX, int edgeX)
        {
            return ((cursorX >= edgeX - 1) && (cursorX <= edgeX + 1));
        }

        private bool IsCursorOnHorizontalEdge(int cursorY, int edgeY)
        {
            return ((cursorY >= edgeY - 1) && (cursorY <= edgeY + 1));
        }

        public void CommandClick(string command)
        {
        }

        public void Dispose()
        {
            _tooltip.Dispose();
        }

    }

}
