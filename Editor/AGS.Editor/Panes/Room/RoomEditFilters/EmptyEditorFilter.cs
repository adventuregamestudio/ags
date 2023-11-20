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
		private Panel _panel;
        private Room _room;

        public EmptyEditorFilter(Panel displayPanel, Room room)
        {
			_panel = displayPanel;
            _room = room;
            DesignItems = new SortedDictionary<string, DesignTimeProperties>();
        }

        public string Name { get { return "Nothing"; } }
        public string DisplayName { get { return "Nothing"; } }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public SortedDictionary<string, DesignTimeProperties> DesignItems { get; private set; }

        public bool SupportVisibleItems { get { return false; } }
        public bool Modified { get; set; }
        public bool Visible { get; set; }
        public bool Locked { get; set; }
        public bool Enabled { get { return false; } }

        public event EventHandler OnItemsChanged { add { } remove { } }
        public event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged { add { } remove { } }
        public event EventHandler<RoomFilterContextMenuArgs> OnContextMenu { add { } remove { } }

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

        public void Paint(Graphics graphics, RoomEditorState state)
        {
        }

        public bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            return false;
        }

        public bool MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            return false;
        }

		public bool DoubleClick(RoomEditorState state)
		{
            return false;
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

        public bool KeyReleased(Keys key)
        {
            return false;
        }

        public void Dispose()
        {
        }

        public string GetItemName(string id)
        {
            return null;
        }

        public void SelectItem(string id) 
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
