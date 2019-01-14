using System;
using System.Drawing;
using System.Windows.Forms;
using AGS.Types;
using AgsView = AGS.Types.View;
using System.Web;
using System.Collections.Generic;

namespace AGS.Editor
{
    /// <summary>
    /// Comment by Shane Stevens:
    /// CharactersEditorFilter is an entirely new file created for displaying characters in rooms.
    /// Most of the methods are adapted from ObjectsEditorFilter as their functionality is similar.
    /// </summary>
    public class CharactersEditorFilter : IRoomEditorFilter
    {
        private const string MENU_ITEM_COPY_COORDS = "CopyCoordinatesToClipboard";
        private const string MENU_ITEM_COPY_CHAR_COORDS = "CopyCharacterCoordinatesToClipboard";

        private GUIController.PropertyObjectChangedHandler _propertyObjectChangedDelegate;
        private Game _game = null;
        private Room _room;
        private Panel _panel;
        private Character _selectedCharacter = null;
        private bool _movingCharacterWithMouse = false;
        private int _menuClickX = 0;
        private int _menuClickY = 0;
        private int _mouseOffsetX, _mouseOffsetY;

        public CharactersEditorFilter(Panel displayPanel, Room room, Game game)
        {
            _room = room;
            _panel = displayPanel;
            _game = game;
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
            VisibleItems = new List<string>();
            LockedItems = new List<string>();
        }

        public void MouseDownAlways(MouseEventArgs e, RoomEditorState state) 
        {
            _selectedCharacter = null;
        }

        public bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            int xClick = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
            int yClick = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;
            Character character = GetCharacter(xClick, yClick, state);
            if (character != null) SelectCharacter(character, xClick, yClick, state);
            
            if (_selectedCharacter != null)
            {
                Factory.GUIController.SetPropertyGridObject(_selectedCharacter);
                return true;
            }
            return false;
        }

        private void SelectCharacter(Character character, int xClick, int yClick, RoomEditorState state)
        {            
            if (!state.DragFromCenter) 
            {
                _mouseOffsetX = xClick - character.StartX;
                _mouseOffsetY = yClick - character.StartY;
            }
            else
            {
                _mouseOffsetX = 0;
                _mouseOffsetY = 0;
            }
            SetSelectedCharacter(character);
            _movingCharacterWithMouse = true;
        }

        private Character GetCharacter(int x, int y, RoomEditorState state)
        {
            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                if (_room.Number != character.StartingRoom) continue;

                if (!VisibleItems.Contains(character.ScriptName) || LockedItems.Contains(character.ScriptName)) continue;

                AgsView view = _game.FindViewByID(character.NormalView);

                if (view != null && view.Loops.Count > 0)
                {
                    if (HitTest(x, y, character, view)) return character;
                }
            }
            return null;
        }

        private bool HitTest(int x, int y, Character character, AgsView view)
        { 
            int spriteNum = 0;

            if (view.Loops[0].Frames.Count > 0)
            {
                ViewFrame thisFrame = view.Loops[0].Frames[0];
                spriteNum = thisFrame.Image;
            }

            int width = GetSpriteWidthForGameResolution(spriteNum);
            int height = GetSpriteHeightForGameResolution(spriteNum);

            return ((x >= character.StartX - (width / 2)) && (x < character.StartX + (width / 2)) &&
                (y >= character.StartY - height) && (y < character.StartY));          
        }

        public bool MouseMove(int x, int y, RoomEditorState state)
        {
            if (!_movingCharacterWithMouse) return false;
            
            int newX = (x + state.ScrollOffsetX) / state.ScaleFactor - _mouseOffsetX;
            int newY = (y + state.ScrollOffsetY) / state.ScaleFactor - _mouseOffsetY;
            return MoveCharacter(newX, newY);                     
        }

        private bool MoveCharacter(int newX, int newY)
        {
            if (_selectedCharacter.StartX == newX &&
                _selectedCharacter.StartY == newY)
            {
                return false;
            }
            _selectedCharacter.StartX = newX;
            _selectedCharacter.StartY = newY;
            return true;
        }

        private void CoordMenuEventHandler(object sender, EventArgs e)
        {
            int tempx = _menuClickX;
            int tempy = _menuClickY;

            string textToCopy = tempx.ToString() + ", " + tempy.ToString();
            Utilities.CopyTextToClipboard(textToCopy);
        }

        private void ShowCoordMenu(MouseEventArgs e, RoomEditorState state)
        {
            EventHandler onClick = new EventHandler(CoordMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            menu.Items.Add(new ToolStripMenuItem("Copy mouse coordinates to clipboard", null, onClick, MENU_ITEM_COPY_COORDS));

            _menuClickX = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
            _menuClickY = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;

            menu.Show(_panel, e.X, e.Y);
        }

        private void CharCoordMenuEventHandler(object sender, EventArgs e)
        {
            int tempx = _selectedCharacter.StartX;
            int tempy = _selectedCharacter.StartY;

            string textToCopy = tempx.ToString() + ", " + tempy.ToString();
            Utilities.CopyTextToClipboard(textToCopy);
        }

        private void ShowCharCoordMenu(MouseEventArgs e, RoomEditorState state)
        {
            if (_selectedCharacter != null)
            {
                EventHandler onClick = new EventHandler(CharCoordMenuEventHandler);
                ContextMenuStrip menu = new ContextMenuStrip();
                menu.Items.Add(new ToolStripMenuItem("Copy Character coordinates to clipboard", null, onClick, MENU_ITEM_COPY_CHAR_COORDS));

                _menuClickX = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
                _menuClickY = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;

                menu.Show(_panel, e.X, e.Y);
            }
        }

        public bool MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            _movingCharacterWithMouse = false;

            if (e.Button == MouseButtons.Middle)
            {
                ShowCoordMenu(e, state);
            }
            else if (e.Button == MouseButtons.Right)
            {
                ShowCharCoordMenu(e, state);
            }
            else return false;
            return true;
        }

        public void Invalidate() { _panel.Invalidate(); }

        public void PaintToHDC(IntPtr hdc, RoomEditorState state)
        {
            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                if (_room.Number == character.StartingRoom && VisibleItems.Contains(character.ScriptName))
                {
                    DrawCharacter(character, state);
                }
            }
        }

        // CLNUP need to check
        private int GetSpriteHeightForGameResolution(int spriteSlot)
        {
            return Factory.NativeProxy.GetSpriteHeight(spriteSlot);
        }

        // CLNUP need to check
        private int GetSpriteWidthForGameResolution(int spriteSlot)
        {
            return Factory.NativeProxy.GetSpriteWidth(spriteSlot);
        }

        public void Paint(Graphics graphics, RoomEditorState state)
        {
            Pen pen = new Pen(Color.Goldenrod);
            pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;

            if (_selectedCharacter != null)
            {
                int scale = state.ScaleFactor;
                Rectangle rect = GetCharacterRect(_selectedCharacter, scale, state);
                graphics.DrawRectangle(pen, rect);

                if (_movingCharacterWithMouse)
                {
                    System.Drawing.Font font = new System.Drawing.Font("Arial", 10.0f);
                    string toDraw = String.Format("X:{0}, Y:{1}", _selectedCharacter.StartX, _selectedCharacter.StartY);

                    int scaledx = rect.X + (rect.Width / 2) - ((int)graphics.MeasureString(toDraw, font).Width / 2);
                    int scaledy = rect.Y - (int)graphics.MeasureString(toDraw, font).Height;
                    if (scaledx < 0) scaledx = 0;
                    if (scaledy < 0) scaledy = 0;

                    graphics.DrawString(toDraw, font, pen.Brush, (float)scaledx, (float)scaledy);
                }
            }
        }

        private int AdjustXCoordinateForWindowScroll(int x, RoomEditorState state)
        {
            return (x - (state.ScrollOffsetX / state.ScaleFactor)) * state.ScaleFactor;
        }

        private int AdjustYCoordinateForWindowScroll(int y, RoomEditorState state)
        {
            return (y - (state.ScrollOffsetY / state.ScaleFactor)) * state.ScaleFactor;
        }

        private void DrawCharacter(Character character, RoomEditorState state)
        {
            AgsView view = _game.FindViewByID(character.NormalView);

            if (view != null && view.Loops.Count > 0)
            {
                int scale = state.ScaleFactor; // ScaleFactor is from the slider in the room tab

                int spriteNum = 0;

                //this is a check to make certain that loop 0 frame 0 of the character normalview has an image;
                //if not, it defaults to using spriteNum 0
                if (view.Loops[0].Frames.Count > 0)
                {
                    ViewFrame thisFrame = view.Loops[0].Frames[0];
                    spriteNum = thisFrame.Image;
                }
                int xPos = AdjustXCoordinateForWindowScroll(character.StartX, state);// character.StartX* scale;
                int yPos = AdjustYCoordinateForWindowScroll(character.StartY, state);// character.StartY* scale;
                int spriteWidth = GetSpriteWidthForGameResolution(spriteNum) * scale;// Factory.NativeProxy.GetRelativeSpriteWidth(spriteNum) * scale;
                int spriteHeight = GetSpriteHeightForGameResolution(spriteNum) * scale; // Factory.NativeProxy.GetRelativeSpriteHeight(spriteNum) * scale;

                Factory.NativeProxy.DrawSpriteToBuffer(spriteNum, xPos - spriteWidth / 2, yPos - spriteHeight, scale);
            }
        }

        private Rectangle GetCharacterRect(Character character, int scale, RoomEditorState state)
        {
            AgsView view = _game.FindViewByID(character.NormalView);
            int xPos = AdjustXCoordinateForWindowScroll(character.StartX, state);// character.StartX* scale;
            int yPos = AdjustYCoordinateForWindowScroll(character.StartY, state);// character.StartY* scale;

            if (view == null || view.Loops.Count == 0)
            {
                return new Rectangle(xPos - 5, yPos - 5, 10, 10);
            }

            int spriteNum = 0;
            if (view.Loops[0].Frames.Count > 0)
                spriteNum = _game.FindViewByID(character.NormalView).Loops[0].Frames[0].Image;
            int spriteWidth = GetSpriteWidthForGameResolution(spriteNum) * scale;// Factory.NativeProxy.GetRelativeSpriteWidth(spriteNum) * scale;
            int spriteHeight = GetSpriteHeightForGameResolution(spriteNum) * scale; // Factory.NativeProxy.GetRelativeSpriteHeight(spriteNum) * scale;
            return new Rectangle(xPos - spriteWidth / 2, yPos - spriteHeight, spriteWidth, spriteHeight);
        }

        public void FilterOn()
        {
            SetPropertyGridList();
            Factory.GUIController.OnPropertyObjectChanged += _propertyObjectChangedDelegate;
        }

        public void FilterOff()
        {
            Factory.GUIController.OnPropertyObjectChanged -= _propertyObjectChangedDelegate;
        }

        private void SetPropertyGridList()
        {
            Dictionary<string, object> defaultPropertyObjectList = new Dictionary<string, object>();
            defaultPropertyObjectList.Add(_room.PropertyGridTitle, _room);
            foreach (Character character in GetItems())
            {
                defaultPropertyObjectList.Add(character.PropertyGridTitle, character);                
            }

            Factory.GUIController.SetPropertyGridObjectList(defaultPropertyObjectList);
        }

        private void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is Character)
            {
                SetSelectedCharacter((Character)newPropertyObject);                 
                _panel.Invalidate();
            }
            else if (newPropertyObject is Room)
            {
                _selectedCharacter = null;
                _panel.Invalidate();
            }
        }

        public string DisplayName { get { return "Characters"; } }

        public bool VisibleByDefault { get { return true; } }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public bool SupportVisibleItems { get { return true; } }

        public List<string> VisibleItems { get; private set; }
        public List<string> LockedItems { get; private set; }
        
        public event EventHandler OnItemsChanged { add { } remove { } }
        public event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;

	public int ItemCount
	{
            get 
            {
                int count = 0;
                foreach (Character character in GetItems()) count++;
                return count;
            }
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

        public bool DoubleClick(RoomEditorState state)
        {
            return false;
        }

        public void CommandClick(string command)
        {
        }

        public bool KeyPressed(Keys key)
        {
            if (_selectedCharacter == null) return false;
            switch (key)
            {
                case Keys.Right:
                    return MoveCharacter(_selectedCharacter.StartX + 1, _selectedCharacter.StartY);
                case Keys.Left:
                    return MoveCharacter(_selectedCharacter.StartX - 1, _selectedCharacter.StartY);
                case Keys.Down:
                    return MoveCharacter(_selectedCharacter.StartX, _selectedCharacter.StartY + 1);
                case Keys.Up:
                    return MoveCharacter(_selectedCharacter.StartX, _selectedCharacter.StartY - 1);
            }
            return false;
        }

        public void Dispose()
        {
        }

        public List<string> GetItemsNames()
        {
            List<string> names = new List<string>();
            foreach (Character character in GetItems())
            {
                names.Add(character.ScriptName);
            }
            return names;
        }

        private IEnumerable<Character> GetItems()
        {
            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                if (character.StartingRoom == _room.Number)
                {
                    yield return character;
                }
            }
        }

        public void SelectItem(string name)
        {
            if (name != null)
            {
                foreach (Character character in GetItems())
                {
                    if (character.ScriptName != name) continue;
                    _selectedCharacter = character;
                    Factory.GUIController.SetPropertyGridObject(character);                    
                    return;
                }
            }

            _selectedCharacter = null;
            Factory.GUIController.SetPropertyGridObject(_room);            
        }

        public Cursor GetCursor(int x, int y, RoomEditorState state)
        {
            if (_movingCharacterWithMouse) return Cursors.Hand;
            x = (x + state.ScrollOffsetX) / state.ScaleFactor;
            y = (y + state.ScrollOffsetY) / state.ScaleFactor;
            if (GetCharacter(x, y, state) != null) return Cursors.Default;
            return null;
        }

        public bool AllowClicksInterception()
        {
            return true;
        }

        private void SetSelectedCharacter(Character character)
        {
            _selectedCharacter = character;
            if (OnSelectedItemChanged != null)
            {
                OnSelectedItemChanged(this, new SelectedRoomItemEventArgs(character.ScriptName));
            }
        }

    }
}
