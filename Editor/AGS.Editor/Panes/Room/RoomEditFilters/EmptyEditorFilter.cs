using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public class EmptyEditorFilter : IRoomEditorFilter
    {
		private const string MENU_ITEM_COPY_COORDS = "CopyCoordinatesToClipboard";

		private int _menuClickX, _menuClickY;
		private Panel _panel;
        private Room _room;

		public EmptyEditorFilter(Panel displayPanel, Room room)
        {
			_panel = displayPanel;
            _room = room;
            VisibleItems = new List<string>();
            LockedItems = new List<string>();
        }

        public string DisplayName { get { return "Nothing"; } }

        public bool VisibleByDefault { get { return false; } }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public List<string> VisibleItems { get; private set; }
        public List<string> LockedItems { get; private set; }

        public bool SupportVisibleItems { get { return false; } }

        public event EventHandler OnItemsChanged { add { } remove { } }
        public event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged { add { } remove { } }

        public int ItemCount
        {
            get { return 0; }
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

        public void Invalidate() { _panel.Invalidate(); }

        public void PaintToHDC(IntPtr hDC, RoomEditorState state)
        {
        }

        public void Paint(Graphics graphics, RoomEditorState state)
        {
        }

        public void MouseDownAlways(MouseEventArgs e, RoomEditorState state) { }

        public bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            return false;
        }

        public bool MouseUp(MouseEventArgs e, RoomEditorState state)
        {
			if (e.Button == MouseButtons.Middle)
			{
				ShowCoordMenu(e, state);
                return true;
			}
            return false;
        }

		public bool DoubleClick(RoomEditorState state)
		{
            return false;
		}

		private void CoordMenuEventHandler(object sender, EventArgs e)
		{
			string textToCopy = _menuClickX.ToString() + ", " + _menuClickY.ToString();
            Utilities.CopyTextToClipboard(textToCopy);
		}

		private void ShowCoordMenu(MouseEventArgs e, RoomEditorState state)
		{
			EventHandler onClick = new EventHandler(CoordMenuEventHandler);
			ContextMenuStrip menu = new ContextMenuStrip();
			menu.Items.Add(new ToolStripMenuItem("Copy mouse coordinates to clipboard", null, onClick, MENU_ITEM_COPY_COORDS));

			_menuClickX = state.WindowXToRoom(e.X);
			_menuClickY = state.WindowYToRoom(e.Y);

            if ((Factory.AGSEditor.CurrentGame.Settings.UseLowResCoordinatesInScript) &&
                (_room.Resolution == RoomResolution.HighRes))
            {
                _menuClickX /= 2;
                _menuClickY /= 2;
            }

			menu.Show(_panel, e.X, e.Y);
		}

        public bool MouseMove(int x, int y, RoomEditorState state)
        {
            return false;
        }

        public void FilterOn()
        {
        }

        public void FilterOff()
        {
        }

        public void CommandClick(string command)
        {
        }

        public bool KeyPressed(Keys key)
        {
            return false;
        }

        public void Dispose()
        {
        }

        public List<string> GetItemsNames() 
        { 
            return new List<string>(); 
        }

        public void SelectItem(string name) 
        {
        }

        public Cursor GetCursor(int x, int y, RoomEditorState state)
        {
            return null;
        }

        public bool AllowClicksInterception()
        {
            return true;
        }
    }

}
