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

        public Character SelectedCharacter { get { return _selectedCharacter; } }

        public CharactersEditorFilter(Panel displayPanel, Room room, Game game)
        {
            _room = room;
            _panel = displayPanel;
            _game = game;
            _propertyObjectChangedDelegate = new GUIController.PropertyObjectChangedHandler(GUIController_OnPropertyObjectChanged);
            RoomItemRefs = new SortedDictionary<string, Character>();
            DesignItems = new SortedDictionary<string, DesignTimeProperties>();
            InitGameEntities();

            Components.CharactersComponent cmp = ComponentController.Instance.FindComponent<Components.CharactersComponent>();
            if (cmp != null)
            {
                cmp.OnCharacterIDChanged += OnCharacterIDChanged;
                cmp.OnCharacterRoomChanged += OnCharacterRoomChanged;
            }
        }

        public void MouseDownAlways(MouseEventArgs e, RoomEditorState state) 
        {
            _selectedCharacter = null;
        }

        public bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            int xClick = state.WindowXToRoom(e.X);
            int yClick = state.WindowYToRoom(e.Y);
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
                DesignTimeProperties p;
                if (!DesignItems.TryGetValue(GetItemID(character), out p))
                    continue; // character is not in the room
                if (!p.Visible || p.Locked) continue;

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
            
            int width, height;
            Utilities.GetSizeSpriteWillBeRenderedInGame(spriteNum, out width, out height);

            return ((x >= character.StartX - (width / 2)) && (x < character.StartX + (width / 2)) &&
                (y >= character.StartY - height) && (y < character.StartY));          
        }

        public bool MouseMove(int x, int y, RoomEditorState state)
        {
            if (!_movingCharacterWithMouse) return false;
            
            int newX = state.WindowXToRoom(x) - _mouseOffsetX;
            int newY = state.WindowYToRoom(y) - _mouseOffsetY;
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
            RoomEditorState.AdjustCoordsToMatchEngine(_room, ref tempx, ref tempy);
            string textToCopy = tempx.ToString() + ", " + tempy.ToString();
            Utilities.CopyTextToClipboard(textToCopy);
        }

        private void ShowCoordMenu(MouseEventArgs e, RoomEditorState state)
        {
            EventHandler onClick = new EventHandler(CoordMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            menu.Items.Add(new ToolStripMenuItem("Copy mouse coordinates to clipboard", null, onClick, MENU_ITEM_COPY_COORDS));

            _menuClickX = state.WindowXToRoom(e.X);
            _menuClickY = state.WindowYToRoom(e.Y);

            menu.Show(_panel, e.X, e.Y);
        }

        private void CharCoordMenuEventHandler(object sender, EventArgs e)
        {
            int tempx = _selectedCharacter.StartX;
            int tempy = _selectedCharacter.StartY;
            RoomEditorState.AdjustCoordsToMatchEngine(_room, ref tempx, ref tempy);
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

                _menuClickX = state.WindowXToRoom(e.X);
                _menuClickY = state.WindowYToRoom(e.Y);

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
                DesignTimeProperties p;
                if (DesignItems.TryGetValue(GetItemID(character), out p) && p.Visible)
                {
                    DrawCharacter(character, state);
                }
            }
        }

        public void Paint(Graphics graphics, RoomEditorState state)
        {
            Pen pen = new Pen(Color.Goldenrod);
            pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;

            if (_selectedCharacter != null)
            {
                Rectangle rect = GetCharacterRect(_selectedCharacter, state);
                graphics.DrawRectangle(pen, rect);

                if (_movingCharacterWithMouse)
                {
                    System.Drawing.Font font = new System.Drawing.Font("Arial", 10.0f);
                    string toDraw = String.Format("X:{0}, Y:{1}", _selectedCharacter.StartX, _selectedCharacter.StartY);

                    int scaledx = rect.X + (rect.Width / 2) - ((int)graphics.MeasureString(toDraw, font).Width / 2);
                    int scaledy = rect.Y - (int)graphics.MeasureString(toDraw, font).Height;
                    if (scaledx < 0) scaledx = 0;
                    if (scaledy < 0) scaledy = 0;

                    graphics.DrawString(toDraw, font, pen.Brush, scaledx, scaledy);
                }
            }
        }

        private void DrawCharacter(Character character, RoomEditorState state)
        {
            AgsView view = _game.FindViewByID(character.NormalView);

            if (view != null && view.Loops.Count > 0)
            {
                int spriteNum = 0;

                //this is a check to make certain that loop 0 frame 0 of the character normalview has an image;
                //if not, it defaults to using spriteNum 0
                if (view.Loops[0].Frames.Count > 0)
                {
                    ViewFrame thisFrame = view.Loops[0].Frames[0];
                    spriteNum = thisFrame.Image;
                }
                int xPos = state.RoomXToWindow(character.StartX);
                int yPos = state.RoomYToWindow(character.StartY);
                int spriteWidth, spriteHeight;
                Utilities.GetSizeSpriteWillBeRenderedInGame(spriteNum, out spriteWidth, out spriteHeight);
                spriteWidth = state.RoomSizeToWindow(spriteWidth);
                spriteHeight = state.RoomSizeToWindow(spriteHeight);

                Factory.NativeProxy.DrawSpriteToBuffer(spriteNum, xPos - spriteWidth / 2, yPos - spriteHeight, state.Scale);
            }
        }

        private Rectangle GetCharacterRect(Character character, RoomEditorState state)
        {
            AgsView view = _game.FindViewByID(character.NormalView);
            int xPos = state.RoomXToWindow(character.StartX);
            int yPos = state.RoomYToWindow(character.StartY);

            if (view == null || view.Loops.Count == 0)
            {
                return new Rectangle(xPos - 5, yPos - 5, 10, 10);
            }

            int spriteNum = 0;
            if (view.Loops[0].Frames.Count > 0)
                spriteNum = _game.FindViewByID(character.NormalView).Loops[0].Frames[0].Image;
            int spriteWidth, spriteHeight;
            Utilities.GetSizeSpriteWillBeRenderedInGame(spriteNum, out spriteWidth, out spriteHeight);
            spriteWidth = state.RoomSizeToWindow(spriteWidth);
            spriteHeight = state.RoomSizeToWindow(spriteHeight);
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

        public void UpdateCharactersRoom(Character character, int oldRoom)
        {
            if (character.StartingRoom == _room.Number)
                AddCharacterRef(character);
            else if (oldRoom == _room.Number && character.StartingRoom != _room.Number)
                RemoveCharacterRef(character);
        }

        private void SetPropertyGridList()
        {
            Dictionary<string, object> defaultPropertyObjectList = new Dictionary<string, object>();
            defaultPropertyObjectList.Add(_room.PropertyGridTitle, _room);
            foreach (var item in RoomItemRefs)
            {
                defaultPropertyObjectList.Add(item.Value.PropertyGridTitle, item.Value);
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

        public string Name { get { return "Characters"; } }
        public string DisplayName { get { return "Characters"; } }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public bool SupportVisibleItems { get { return true; } }
        public bool Modified { get; set; }
        public bool Visible { get; set; }
        public bool Locked { get; set; }

        public SortedDictionary<string, DesignTimeProperties> DesignItems { get; private set; }
        /// <summary>
        /// A lookup table for getting game object reference by they key.
        /// </summary>
        private SortedDictionary<string, Character> RoomItemRefs { get; set; }

        public event EventHandler OnItemsChanged;
        public event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;

        public int ItemCount
	    {
            get 
            {
                return RoomItemRefs.Count;
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

        private string GetItemID(Character c)
        {
            // Use numeric character's ID as a "unique identifier", for now (script name is optional!)
            return GetItemID(c.ID);
        }

        private string GetItemID(int numID)
        {
            return numID.ToString("D4");
        }

        /// <summary>
        /// Initialize dictionary of current item references.
        /// </summary>
        /// <returns></returns>
        private SortedDictionary<string, Character> InitItemRefs()
        {
            SortedDictionary<string, Character> items = new SortedDictionary<string, Character>();
            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                if (character.StartingRoom == _room.Number)
                    items.Add(GetItemID(character), character);
            }
            return items;
        }

        public string GetItemName(string id)
        {
            Character character;
            if (id != null && RoomItemRefs.TryGetValue(id, out character))
                return character.ScriptName;
            return null;
        }

        public void SelectItem(string id)
        {
            if (id != null)
            {
                Character character;
                if (RoomItemRefs.TryGetValue(id, out character))
                {
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
            x = state.WindowXToRoom(x);
            y = state.WindowYToRoom(y);
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

        private void OnCharacterIDChanged(object sender, CharacterIDChangedEventArgs e)
        {
            UpdateCharacterRef(e.Character, GetItemID(e.OldID));
            OnItemsChanged(this, null);
        }

        private void OnCharacterRoomChanged(object sender, CharacterRoomChangedEventArgs e)
        {
            UpdateCharactersRoom(e.Character, e.PreviousRoom);
            OnItemsChanged(this, null);
            Invalidate();
        }

        private void AddCharacterRef(Character c)
        {
            string id = GetItemID(c);
            if (RoomItemRefs.ContainsKey(id))
                return;
            RoomItemRefs.Add(id, c);
            DesignItems.Add(id, new DesignTimeProperties());
        }

        private void RemoveCharacterRef(Character c)
        {
            string id = GetItemID(c);
            RoomItemRefs.Remove(id);
            DesignItems.Remove(id);
        }

        private void UpdateCharacterRef(Character c, string oldID)
        {
            if (!RoomItemRefs.ContainsKey(oldID))
                return;
            string newID = GetItemID(c);
            RoomItemRefs.Remove(oldID);
            RoomItemRefs.Add(newID, c);
            // We must keep DesignTimeProperties!
            DesignItems.Add(newID, DesignItems[oldID]);
            DesignItems.Remove(oldID);
        }
    }
}
