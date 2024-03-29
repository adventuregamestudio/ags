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
    /// CharactersEditorFilter manages characters in rooms.
    /// Characters themselves are global objects and cannot be created in the
    /// room editor, instead room editor displays those characters that start
    /// in this room, and lets position them, and edit their properties.
    /// </summary>
    public class CharactersEditorFilter : BaseThingEditorFilter<Character>
    {
        private const string MENU_ITEM_COPY_CHAR_COORDS = "CopyCharacterCoordinatesToClipboard";

        private Game _game = null;
        private bool _movingCharacterWithMouse = false;
        private int _menuClickX = 0;
        private int _menuClickY = 0;
        private int _mouseOffsetX, _mouseOffsetY;
        private bool _movingCharacterWithKeyboard = false;
        private int _movingKeysDown = 0;
        private Timer _movingHintTimer = new Timer();

        public Character SelectedCharacter { get { return _selectedObject; } }

        public CharactersEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room, Game game)
            : base(displayPanel, editor, room)
        {
            _game = game;
            // Init a starting list of item references for navigation UI
            // TODO: we have to do it from child class and pass to parent,
            // because child class may require extra data to gather this list, and
            // it's only initialized when child's constructor is called (after base class!).
            // There may be better way, like another method in IRoomEditorFilter which
            // would be called after constructor but before filter is made ready for use.
            InitRoomItemRefs(CollectItemRefs());

            Components.CharactersComponent cmp = ComponentController.Instance.FindComponent<Components.CharactersComponent>();
            if (cmp != null)
            {
                cmp.OnCharacterIDChanged += OnCharacterIDChanged;
                cmp.OnCharacterRoomChanged += OnCharacterRoomChanged;
            }

            _movingHintTimer.Interval = 2000;
            _movingHintTimer.Tick += MovingHintTimer_Tick;
        }

        public override bool MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            if (e.Button == MouseButtons.Middle) return false;

            int xClick = state.WindowXToRoom(e.X);
            int yClick = state.WindowYToRoom(e.Y);
            Character character = GetCharacter(xClick, yClick, state);
            if (character != null) SelectCharacter(character, xClick, yClick, state);
            else _selectedObject = null;

            if (_selectedObject != null)
            {
                Factory.GUIController.SetPropertyGridObject(_selectedObject);
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

        public override bool MouseMove(int x, int y, RoomEditorState state)
        {
            if (!_movingCharacterWithMouse) return false;

            int newX = state.WindowXToRoom(x) - _mouseOffsetX;
            int newY = state.WindowYToRoom(y) - _mouseOffsetY;
            return MoveCharacter(newX, newY);
        }

        private bool MoveCharacter(int newX, int newY)
        {
            if (_selectedObject == null)
            {
                ClearMovingState();
            }
            else
            {
                if ((newX != _selectedObject.StartX) ||
                    (newY != _selectedObject.StartY))
                {
                    _selectedObject.StartX = newX;
                    _selectedObject.StartY = newY;
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
            int tempx = _selectedObject.StartX;
            int tempy = _selectedObject.StartY;
            RoomEditorState.AdjustCoordsToMatchEngine(_room, ref tempx, ref tempy);
            string textToCopy = tempx.ToString() + ", " + tempy.ToString();
            Utilities.CopyTextToClipboard(textToCopy);
        }

        private void ShowCharCoordMenu(MouseEventArgs e, RoomEditorState state)
        {
            if (_selectedObject != null)
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

        public override bool MouseUp(MouseEventArgs e, RoomEditorState state)
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

        public override void PaintToHDC(IntPtr hdc, RoomEditorState state)
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

        public override void Paint(Graphics graphics, RoomEditorState state)
        {
            if (!Enabled || _selectedObject == null)
                return;

            Pen pen = new Pen(Color.Goldenrod);
            pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;
            
            {
                Rectangle rect = GetCharacterRect(_selectedObject, state);
                graphics.DrawRectangle(pen, rect);

                if (_movingCharacterWithMouse || _movingCharacterWithKeyboard)
                {
                    Brush shadeBrush = new SolidBrush(Color.FromArgb(200, Color.Black));
                    System.Drawing.Font font = new System.Drawing.Font("Arial", 10.0f);
                    string toDraw = String.Format("X:{0}, Y:{1}", _selectedObject.StartX, _selectedObject.StartY);

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

        protected override void FilterActivated()
        {
            ClearMovingState();
        }

        protected override void FilterDeactivated()
        {
            ClearMovingState();
        }

        public void UpdateCharactersRoom(Character character, int oldRoom)
        {
            if (character.StartingRoom == _room.Number)
            {
                AddObjectRef(character);
            }
            else if (oldRoom == _room.Number && character.StartingRoom != _room.Number)
            {
                RemoveObjectRef(character);
                if (_selectedObject == character)
                    _selectedObject = null;
            }
        }

        protected override void GUIController_OnPropertyObjectChanged(object newPropertyObject)
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
                _selectedObject = null;
                _panel.Invalidate();
            }
        }

        public override string Name { get { return "Characters"; } }
        public override string DisplayName { get { return "Characters"; } }

        public override event EventHandler OnItemsChanged;
        public override event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;
        public override event EventHandler<RoomFilterContextMenuArgs> OnContextMenu;

		public override string HelpKeyword
		{
			get { return string.Empty; }
		}

        public override bool DoubleClick(RoomEditorState state)
        {
            return false;
        }

        public override void CommandClick(string command)
        {
        }

        public override bool KeyPressed(Keys key)
        {
            if (_selectedObject == null)
                return false;
            if (DesignItems[GetItemID(_selectedObject)].Locked)
                return false;

            switch (key)
            {
                case Keys.Left:
                    _movingKeysDown |= 1; _movingCharacterWithKeyboard = true;
                    return MoveCharacter(_selectedObject.StartX - 1, _selectedObject.StartY);
                case Keys.Right:
                    _movingKeysDown |= 2; _movingCharacterWithKeyboard = true;
                    return MoveCharacter(_selectedObject.StartX + 1, _selectedObject.StartY);
                case Keys.Up:
                    _movingKeysDown |= 4; _movingCharacterWithKeyboard = true;
                    return MoveCharacter(_selectedObject.StartX, _selectedObject.StartY - 1);
                case Keys.Down:
                    _movingKeysDown |= 8; _movingCharacterWithKeyboard = true;
                    return MoveCharacter(_selectedObject.StartX, _selectedObject.StartY + 1);
            }
            return false;
        }

        public override bool KeyReleased(Keys key)
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

        protected override string GetItemID(Character c)
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
        private SortedDictionary<string, Character> CollectItemRefs()
        {
            SortedDictionary<string, Character> items = new SortedDictionary<string, Character>();
            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                if (character.StartingRoom == _room.Number)
                    items.Add(GetItemID(character), character);
            }
            return items;
        }

        /// <summary>
        /// Gets this object's script name.
        /// </summary>
        protected override string GetItemScriptName(Character obj)
        {
            return obj.ScriptName;
        }

        /// <summary>
        /// Forms a PropertyGrid's entry title for this object.
        /// </summary>
        protected override string GetPropertyGridItemTitle(Character obj)
        {
            return obj.PropertyGridTitle;
        }

        public override Cursor GetCursor(int x, int y, RoomEditorState state)
        {
            if (_movingCharacterWithMouse) return Cursors.Hand;
            x = state.WindowXToRoom(x);
            y = state.WindowYToRoom(y);
            if (GetCharacter(x, y, state) != null) return Cursors.Default;
            return null;
        }

        public override bool AllowClicksInterception()
        {
            return true;
        }

        private void SetSelectedCharacter(Character character)
        {
            _selectedObject = character;
            if (OnSelectedItemChanged != null)
            {
                OnSelectedItemChanged(this, new SelectedRoomItemEventArgs(character.PropertyGridTitle));
            }
            ClearMovingState();
        }

        private void OnCharacterIDChanged(object sender, CharacterIDChangedEventArgs e)
        {
            UpdateObjectRef(e.Character, GetItemID(e.OldID));
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
    }
}
