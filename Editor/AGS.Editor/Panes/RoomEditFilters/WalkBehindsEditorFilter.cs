using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class WalkBehindsEditorFilter : BaseAreasEditorFilter
    {
		private bool _draggingBaseline = false;
		private bool _shownTooltip = false;

        public WalkBehindsEditorFilter(Panel displayPanel, Room room)
            : base(displayPanel, room, true)
        {
        }

        public override RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.WalkBehinds; }
        }

        public override int ItemCount
        {
            get { return _room.WalkBehindCount; }
        }

		public override void Paint(Graphics graphics, RoomEditorState state)
		{
			int lineYPos = GetCurrentAreaBaselineScreenY(state);
			Pen pen = (Pen)GetPenForArea(_selectedArea).Clone();
			pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dash;

			for (int i = 0; i < state.ScaleFactor; i++)
			{
				graphics.DrawLine(pen, 0, lineYPos + i, graphics.VisibleClipBounds.Right, lineYPos + i);
			}

			base.Paint(graphics, state);
		}

		public override bool MouseMove(int x, int y, RoomEditorState state)
		{
			if (_draggingBaseline)
			{
				int newBaseline = (y + state.ScrollOffsetY) / state.ScaleFactor;
				if (newBaseline < 0)
				{
					newBaseline = 0;
				}
				if (newBaseline >= _room.Height)
				{
					newBaseline = _room.Height - 1;
				}
				_room.WalkBehinds[_selectedArea].Baseline = newBaseline;
                _room.Modified = true;
				state.CurrentCursor = Cursors.HSplit;
				return true;
			}

			if (base.MouseMove(x, y, state))
			{
				return true;
			}

			if (IsCursorOnHorizontalEdge(y, GetCurrentAreaBaselineScreenY(state), state))
			{
				state.CurrentCursor = Cursors.HSplit;
			}

			return false;
		}

		public override void MouseDown(MouseEventArgs e, RoomEditorState state)
		{
			if (IsCursorOnHorizontalEdge(e.Y, GetCurrentAreaBaselineScreenY(state), state))
			{
				_draggingBaseline = true;
			}
			else
			{
				base.MouseDown(e, state);
			}
		}

		public override void MouseUp(MouseEventArgs e, RoomEditorState state)
		{
			if (_draggingBaseline)
			{
				_draggingBaseline = false;
			}
			else
			{
				base.MouseUp(e, state);

				if ((_room.WalkBehinds[_selectedArea].Baseline < 1) &&
					(!_shownTooltip))
				{
					_tooltip.Show("After painting the area, remember to set its baseline by dragging the line down from the top of the background.", _panel, e.X, e.Y - 70, 5000);
					_shownTooltip = true;
				}
			}
		}

		private int GetCurrentAreaBaselineScreenY(RoomEditorState state)
		{
			return (_room.WalkBehinds[_selectedArea].Baseline * state.ScaleFactor) - state.ScrollOffsetY;
		}

		private bool IsCursorOnHorizontalEdge(int cursorY, int edgeY, RoomEditorState state)
		{
			return ((cursorY >= edgeY - 1) && (cursorY <= edgeY + state.ScaleFactor));
		}

        protected override void SelectedAreaChanged(int areaNumber)
        {
            Factory.GUIController.SetPropertyGridObject(_room.WalkBehinds[areaNumber]);
        }

		protected override void FilterActivated()
		{
			Factory.GUIController.ShowCuppit("Walk-behinds allow you to give the illusion of 3D by making parts of the background image be drawn in front of the characters. Each area has a baseline, which defines how high up the screen the character needs to be in order to be drawn behind the area.", "Walk-behinds introduction");
		}

        protected override void SetPropertyGridList()
        {
            Dictionary<string, object> defaultPropertyObjectList = new Dictionary<string, object>();
            defaultPropertyObjectList.Add(_room.PropertyGridTitle, _room);
            foreach (RoomWalkBehind area in _room.WalkBehinds)
            {
                defaultPropertyObjectList.Add(area.PropertyGridTitle, area);
            }

            Factory.GUIController.SetPropertyGridObjectList(defaultPropertyObjectList);
        }

        protected override void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is RoomWalkBehind)
            {
                _selectedArea = ((RoomWalkBehind)newPropertyObject).ID;
                _panel.Invalidate();
            }
            else if (newPropertyObject is Room)
            {
                _selectedArea = 0;
                _panel.Invalidate();
            }
        }

        protected override void Room_OnRegionCountChanged(RoomAreaMaskType maskType)
        {
            if (maskType == RoomAreaMaskType.WalkBehinds)
            {
                SetPropertyGridList();
            }
        }
    }

}
