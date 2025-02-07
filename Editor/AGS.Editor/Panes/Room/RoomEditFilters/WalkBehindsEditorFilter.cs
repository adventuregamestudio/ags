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

        public WalkBehindsEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room)
            : base(displayPanel, editor, room)
        {
        }

        public override string Name { get { return "WalkBehinds"; } }
        public override string DisplayName { get { return "Walk-behinds"; } }

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
            if (!Enabled)
                return;

            int lineYPos = GetCurrentAreaBaselineScreenY(state);
			Pen pen = (Pen)GetPenForArea(SelectedArea).Clone();
			pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dash;

			for (int i = 0; i < (int)state.Scale; i++)
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
				int newBaseline = state.WindowYToRoom(y);
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
            else if (!Enabled) return base.MouseUp(e, state);
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
			return state.RoomYToWindow(_room.WalkBehinds[SelectedArea].Baseline);
		}

		private bool IsCursorOnHorizontalEdge(int cursorY, int edgeY, RoomEditorState state)
		{
			return ((cursorY >= edgeY - 1) && (cursorY <= edgeY + (int)state.Scale));
		}

        protected override void SelectedAreaChanged(int areaNumber)
        {
            SetPropertyGridObject(_room.WalkBehinds[areaNumber]);
        }

        protected override string GetItemName(int id)
        {
            return MakeLayerItemName("Walk-behind area", null, null, id);
        }

        protected override SortedDictionary<string, int> InitItemRefs()
        {
            SortedDictionary<string, int> items = new SortedDictionary<string, int>();
            foreach (RoomWalkBehind area in _room.WalkBehinds)
            {
                items.Add(GetItemID(area.ID), area.ID);
            }
            return items;
        }

        protected override Dictionary<string, object> GetPropertyGridList()
        {
            var list = new Dictionary<string, object>();
            list.Add(_room.PropertyGridTitle, _room);
            foreach (RoomWalkBehind area in _room.WalkBehinds)
            {
                list.Add(area.PropertyGridTitle, area);
            }
            return list;
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
