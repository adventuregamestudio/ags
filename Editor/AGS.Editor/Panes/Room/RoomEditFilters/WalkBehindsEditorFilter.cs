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
            : base(displayPanel, room)
        {
        }

        public override string DisplayName { get { return "Walk-behinds"; } }

        public override bool VisibleByDefault { get { return false; } }

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
			Pen pen = (Pen)GetPenForArea(SelectedArea).Clone();
			pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dash;

			for (int i = 0; i < state.ScaleFactor; i++)
			{
				graphics.DrawLine(pen, 0, lineYPos + i, graphics.VisibleClipBounds.Right, lineYPos + i);
			}

			base.Paint(graphics, state);
		}

        public override Cursor GetCursor(int x, int y, RoomEditorState state)
        {
            if (_draggingBaseline) return Cursors.HSplit;
            if (IsCursorOnHorizontalEdge(y, GetCurrentAreaBaselineScreenY(state), state))
            {
                return Cursors.HSplit;
            }			
            return base.GetCursor(x, y, state);
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
				_room.WalkBehinds[SelectedArea].Baseline = newBaseline;
                _room.Modified = true;				
				return true;
			}

			if (base.MouseMove(x, y, state))
			{
				return true;
			}

			return false;
		}

		public override bool MouseDown(MouseEventArgs e, RoomEditorState state)
		{            
			if (IsCursorOnHorizontalEdge(e.Y, GetCurrentAreaBaselineScreenY(state), state))
			{
				_draggingBaseline = true;
                return true;
			}
			else
			{
				return base.MouseDown(e, state);
			}
		}

		public override bool MouseUp(MouseEventArgs e, RoomEditorState state)
		{
            if (_draggingBaseline)
            {
                _draggingBaseline = false;
                return true;
            }
            else if (!IsFilterOn()) return base.MouseUp(e, state);
            else
            {
                bool handledMouseUp = base.MouseUp(e, state);

                if ((_room.WalkBehinds[SelectedArea].Baseline < 1) &&
                    (!_shownTooltip))
                {
                    _tooltip.Show("After painting the area, remember to set its baseline by dragging the line down from the top of the background.", _panel, e.X, e.Y - 70, 5000);
                    _shownTooltip = true;
                    return true;
                }

                return handledMouseUp;
            }
		}

		private int GetCurrentAreaBaselineScreenY(RoomEditorState state)
		{
			return (_room.WalkBehinds[SelectedArea].Baseline * state.ScaleFactor) - state.ScrollOffsetY;
		}

		private bool IsCursorOnHorizontalEdge(int cursorY, int edgeY, RoomEditorState state)
		{
			return ((cursorY >= edgeY - 1) && (cursorY <= edgeY + state.ScaleFactor));
		}

        protected override void SelectedAreaChanged(int areaNumber)
        {
            Factory.GUIController.SetPropertyGridObject(_room.WalkBehinds[areaNumber]);
        }

        protected override Dictionary<string, int> GetItems()
        {
            Dictionary<string, int> items = new Dictionary<string, int>(_room.WalkBehinds.Count);
            foreach (RoomWalkBehind area in _room.WalkBehinds)
            {
                items.Add(GetItemName(area.ID, area.PropertyGridTitle), area.ID);
            }
            return items;
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
                SelectedArea = ((RoomWalkBehind)newPropertyObject).ID;
                _panel.Invalidate();
            }
            else if (newPropertyObject is Room)
            {
                DeselectArea();
                _panel.Invalidate();
            }
        }
    }

}
