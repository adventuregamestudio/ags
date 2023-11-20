using System;
using System.Drawing;
using System.Windows.Forms;
using AGS.Types;
using AgsView = AGS.Types.View;
using System.Web;
using System.Collections.Generic;
using System.Drawing.Imaging;

namespace AGS.Editor
{
    /// <summary>
    /// Comment by Shane Stevens:
    /// CharactersEditorFilter is an entirely new file created for displaying characters in rooms.
    /// Most of the methods are adapted from ObjectsEditorFilter as their functionality is similar.
    /// 
    /// TODO: it must be possible to pick out a base class from ObjectsEditorFilter and
    /// CharactersEditorFilter, there much common functionality here.
    /// </summary>
    public class CharactersEditorFilter : IRoomEditorFilter
    {
        private const string MENU_ITEM_COPY_CHAR_COORDS = "CopyCharacterCoordinatesToClipboard";

        private GUIController.PropertyObjectChangedHandler _propertyObjectChangedDelegate;
        private Game _game = null;
        private Room _room;
        private Panel _panel;
        RoomSettingsEditor _editor;
        private bool _isOn = false;
        private Character _selectedCharacter = null;
        private bool _movingCharacterWithMouse = false;
        private int _menuClickX = 0;
        private int _menuClickY = 0;
        private int _mouseOffsetX, _mouseOffsetY;
        private bool _movingCharacterWithKeyboard = false;
        private int _movingKeysDown = 0;
        private Timer _movingHintTimer = new Timer();

        public Character SelectedCharacter { get { return _selectedCharacter; } }

        public CharactersEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room, Game game)
        {
            _room = room;
            _panel = displayPanel;
            _game = game;
            _editor = editor;
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

            _movingHintTimer.Interval = 2000;
            _movingHintTimer.Tick += MovingHintTimer_Tick;
        }

        public bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            if (e.Button == MouseButtons.Middle) return false;

            int xClick = state.WindowXToRoom(e.X);
            int yClick = state.WindowYToRoom(e.Y);
            Character character = GetCharacter(xClick, yClick, state);
            if (character != null) SelectCharacter(character, xClick, yClick, state);
            else _selectedCharacter = null;

            if (_selectedCharacter != null)
            {
                Factory.GUIController.SetPropertyGridObject(_selectedCharacter);
                return true;
            }
            return false;
        }

        private void SelectCharacter(Character character, int xClick, int yClick, RoomEditorState state)
        {
            SetSelectedCharacter(character);
            if (!DesignItems[GetItemID(character)].Locked)
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
                _movingCharacterWithMouse = true;
            }
        }

        private Character GetCharacter(int x, int y, RoomEditorState state)
        {
            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                DesignTimeProperties p;
                if (!DesignItems.TryGetValue(GetItemID(character), out p))
                    continue; // character is not in the room
                if (!p.Visible) continue;

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
            if (_selectedCharacter == null)
            {
                ClearMovingState();
            }
            else
            {
                if ((newX != _selectedCharacter.StartX) ||
                    (newY != _selectedCharacter.StartY))
                {
                    _selectedCharacter.StartX = newX;
                    _selectedCharacter.StartY = newY;
                    // NOTE: do not mark room as modified, as characters are not part of room data
                }
                _movingHintTimer.Stop();
            }
            return true;
        }

        private void ClearMovingState()
        {
            _movingCharacterWithMouse = false;
            _movingCharacterWithKeyboard = false;
            _movingKeysDown = 0;
            _movingHintTimer.Stop();
        }

        private void MovingHintTimer_Tick(object sender, EventArgs e)
        {
            ClearMovingState();
            Invalidate();
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
                OnContextMenu?.Invoke(this, new RoomFilterContextMenuArgs(menu, e.X, e.Y));

                _menuClickX = state.WindowXToRoom(e.X);
                _menuClickY = state.WindowYToRoom(e.Y);

                menu.Show(_panel, e.X, e.Y);
            }
        }

        public bool MouseUp(MouseEventArgs e, RoomEditorState state)
        {
            _movingCharacterWithMouse = false;
            if (e.Button == MouseButtons.Middle) return false;

            if (e.Button == MouseButtons.Right)
            {
                ShowCharCoordMenu(e, state);
                return true;
            }
            return false;
        }

        public void Invalidate() { _panel.Invalidate(); }

        public void Paint(Graphics graphics, RoomEditorState state)
        {
            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                DesignTimeProperties p;
                if (DesignItems.TryGetValue(GetItemID(character), out p) && p.Visible)
                {
                    DrawCharacter(graphics, character, state);
                }
            }

            if (!Enabled || _selectedCharacter == null)
                return;

            Pen pen = new Pen(Color.Goldenrod);
            pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;
            
            {
                Rectangle rect = GetCharacterRect(_selectedCharacter, state);
                graphics.DrawRectangle(pen, rect);

                if (_movingCharacterWithMouse || _movingCharacterWithKeyboard)
                {
                    Brush shadeBrush = new SolidBrush(Color.FromArgb(200, Color.Black));
                    System.Drawing.Font font = new System.Drawing.Font("Arial", 10.0f);
                    string toDraw = String.Format("X:{0}, Y:{1}", _selectedCharacter.StartX, _selectedCharacter.StartY);

                    var textSize = graphics.MeasureString(toDraw, font);
                    int scaledx = rect.X + (rect.Width / 2) - ((int)textSize.Width / 2);
                    int scaledy = rect.Y - (int)textSize.Height;
                    if (scaledx < 0) scaledx = 0;
                    if (scaledy < 0) scaledy = 0;
                    if (scaledx + textSize.Width >= graphics.VisibleClipBounds.Width)
                        scaledx = (int)(graphics.VisibleClipBounds.Width - textSize.Width);
                    if (scaledy + textSize.Height >= graphics.VisibleClipBounds.Height)
                        scaledy = (int)(graphics.VisibleClipBounds.Height - textSize.Height);

                    graphics.FillRectangle(shadeBrush, scaledx, scaledy, textSize.Width, textSize.Height);
                    graphics.DrawString(toDraw, font, pen.Brush, scaledx, scaledy);
                }
            }
        }

        private void DrawCharacter(Graphics graphics, Character character, RoomEditorState state)
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

                Size spriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(spriteNum);
                spriteSize.Width = state.RoomSizeToWindow(spriteSize.Width);
                spriteSize.Height = state.RoomSizeToWindow(spriteSize.Height);
                int xPos = state.RoomXToWindow(character.StartX) - spriteSize.Width / 2;
                int yPos = state.RoomYToWindow(character.StartY) - spriteSize.Height;

                using (Bitmap sprite = Factory.NativeProxy.GetBitmapForSprite(spriteNum))
                using (Bitmap sprite32bppAlpha = new Bitmap(sprite.Width, sprite.Height, PixelFormat.Format32bppArgb))
                {
                    sprite32bppAlpha.SetRawData(sprite.GetRawData());
                    graphics.DrawImage(sprite32bppAlpha, xPos, yPos, spriteSize.Width, spriteSize.Height);
                }
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
            _isOn = true;
            ClearMovingState();
        }

        public void FilterOff()
        {
            Factory.GUIController.OnPropertyObjectChanged -= _propertyObjectChangedDelegate;
            _isOn = false;
            ClearMovingState();
        }

        public void UpdateCharactersRoom(Character character, int oldRoom)
        {
            if (character.StartingRoom == _room.Number)
            {
                AddCharacterRef(character);
            }
            else if (oldRoom == _room.Number && character.StartingRoom != _room.Number)
            {
                RemoveCharacterRef(character);
                if (_selectedCharacter == character)
                    _selectedCharacter = null;
            }
        }

        private void SetPropertyGridList()
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(_room.PropertyGridTitle, _room);
            foreach (var item in RoomItemRefs)
            {
                list.Add(item.Value.PropertyGridTitle, item.Value);
            }
            Factory.GUIController.SetPropertyGridObjectList(list, _editor.ContentDocument, _room);
        }

        protected void SetPropertyGridObject(object obj)
        {
            Factory.GUIController.SetPropertyGridObject(obj, _editor.ContentDocument);
        }

        private void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is Character)
            {
                Character c = (Character)newPropertyObject;
                if (c.StartingRoom == _room.Number)
                {
                    SetSelectedCharacter(c);
                    _panel.Invalidate();
                }
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
        public bool Enabled { get { return _isOn; } }

        public SortedDictionary<string, DesignTimeProperties> DesignItems { get; private set; }
        /// <summary>
        /// A lookup table for getting game object reference by they key.
        /// </summary>
        private SortedDictionary<string, Character> RoomItemRefs { get; set; }

        public event EventHandler OnItemsChanged;
        public event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;
        public event EventHandler<RoomFilterContextMenuArgs> OnContextMenu;

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
            if (_selectedCharacter == null)
                return false;
            if (DesignItems[GetItemID(_selectedCharacter)].Locked)
                return false;

            switch (key)
            {
                case Keys.Left:
                    _movingKeysDown |= 1; _movingCharacterWithKeyboard = true;
                    return MoveCharacter(_selectedCharacter.StartX - 1, _selectedCharacter.StartY);
                case Keys.Right:
                    _movingKeysDown |= 2; _movingCharacterWithKeyboard = true;
                    return MoveCharacter(_selectedCharacter.StartX + 1, _selectedCharacter.StartY);
                case Keys.Up:
                    _movingKeysDown |= 4; _movingCharacterWithKeyboard = true;
                    return MoveCharacter(_selectedCharacter.StartX, _selectedCharacter.StartY - 1);
                case Keys.Down:
                    _movingKeysDown |= 8; _movingCharacterWithKeyboard = true;
                    return MoveCharacter(_selectedCharacter.StartX, _selectedCharacter.StartY + 1);
            }
            return false;
        }

        public bool KeyReleased(Keys key)
        {
            int moveKeys = _movingKeysDown;
            switch (key)
            {
                case Keys.Left: moveKeys &= ~1; break;
                case Keys.Right: moveKeys &= ~2; break;
                case Keys.Up: moveKeys &= ~4; break;
                case Keys.Down: moveKeys &= ~8; break;
            }
            if (moveKeys != _movingKeysDown)
            {
                _movingKeysDown = moveKeys;
                if (_movingKeysDown == 0)
                {
                    _movingHintTimer.Start();
                    return true;
                }
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
                    SetPropertyGridObject(character);                    
                    return;
                }
            }

            _selectedCharacter = null;
            SetPropertyGridObject(_room);
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
            ClearMovingState();
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
            if (Enabled)
                SetPropertyGridList();
        }

        private void OnCharacterRoomChanged(object sender, CharacterRoomChangedEventArgs e)
        {
            UpdateCharactersRoom(e.Character, e.PreviousRoom);
            OnItemsChanged(this, null);
            Invalidate();
            if (Enabled)
                SetPropertyGridList();
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
            if (newID == oldID)
                return;
            // If the new key is also present that means we are swapping two items
            if (RoomItemRefs.ContainsKey(newID))
            {
                var char2 = RoomItemRefs[newID];
                RoomItemRefs.Remove(newID);
                RoomItemRefs.Remove(oldID);
                RoomItemRefs.Add(newID, c);
                RoomItemRefs.Add(oldID, char2);
                // We must keep DesignTimeProperties!
                var char1Item = DesignItems[oldID];
                var char2Item = DesignItems[newID];
                DesignItems.Remove(newID);
                DesignItems.Remove(oldID);
                DesignItems.Add(newID, char1Item);
                DesignItems.Add(oldID, char2Item);
            }
            else
            {
                RoomItemRefs.Remove(oldID);
                RoomItemRefs.Add(newID, c);
                // We must keep DesignTimeProperties!
                DesignItems.Add(newID, DesignItems[oldID]);
                DesignItems.Remove(oldID);
            }
        }
    }
}
