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

        public string HelpKeyword
        {
            get { return string.Empty; }
        }

        public void PaintToHDC(IntPtr hDC, RoomEditorState state)
        {
        }

        public void Paint(Graphics graphics, RoomEditorState state)
        {
        }

        public void MouseDown(MouseEventArgs e, RoomEditorState state)
        {
        }

        public void MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            if (e.Button == MouseButtons.Middle)
            {
                ShowCoordMenu(e, state);
            }
        }

        public void DoubleClick(RoomEditorState state)
        {
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

            _menuClickX = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
            _menuClickY = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;

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
    }

}
