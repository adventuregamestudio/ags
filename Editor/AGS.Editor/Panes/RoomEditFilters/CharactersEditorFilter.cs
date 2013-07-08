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
        }

        public void MouseDown(MouseEventArgs e, RoomEditorState state)
        {
            int xClick = (e.X + state.ScrollOffsetX) / state.ScaleFactor;
            int yClick = (e.Y + state.ScrollOffsetY) / state.ScaleFactor;

            _selectedCharacter = null;

            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                if (_room.Number == character.StartingRoom)
                {
                    AgsView view = _game.FindViewByID(character.NormalView);

                    if (view != null && view.Loops.Count > 0)
                    {
                        SelectCharacter(view, character, xClick, yClick, state);
                    }
                }
            }

            if (_selectedCharacter != null)
            {
                Factory.GUIController.SetPropertyGridObject(_selectedCharacter);
            }
            else
            {
                Factory.GUIController.SetPropertyGridObject(_room);
            }
        }

        private void SelectCharacter(AgsView view, Character character, int xClick, int yClick, RoomEditorState state)
        {
            int spriteNum = 0;

            if (view.Loops[0].Frames.Count > 0)
            {
                ViewFrame thisFrame = view.Loops[0].Frames[0];
                spriteNum = thisFrame.Image;
            }

            int width = GetSpriteWidthForGameResolution(spriteNum);
            int height = GetSpriteHeightForGameResolution(spriteNum);

            if ((xClick >= character.StartX - (width / 2)) && (xClick < character.StartX + (width / 2)) &&
                (yClick >= character.StartY - height) && (yClick < character.StartY))
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
                _selectedCharacter = character;
                _movingCharacterWithMouse = true;
            }
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

            if ((Factory.AGSEditor.CurrentGame.Settings.UseLowResCoordinatesInScript) &&
                (_room.Resolution == RoomResolution.HighRes))
            {
                tempx /= 2;
                tempy /= 2;
            }

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

            //this halves the coordinates of the x and y values 
            //if you have low res coordinates set in the properties and are in high res

            if ((Factory.AGSEditor.CurrentGame.Settings.UseLowResCoordinatesInScript) &&
                (_room.Resolution == RoomResolution.HighRes))
            {
                tempx /= 2;
                tempy /= 2;
            }

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

        public void MouseUp(MouseEventArgs e, RoomEditorState state)
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
        }

        public void PaintToHDC(IntPtr hdc, RoomEditorState state)
        {
            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                if (_room.Number == character.StartingRoom)
                {
                    DrawCharacter(character, state);
                }
            }
        }

        private int GetSpriteHeightForGameResolution(int spriteSlot)
        {
            int height;
            if (Factory.AGSEditor.CurrentGame.IsHighResolution)
            {
                height = Factory.NativeProxy.GetSpriteResolutionMultiplier(spriteSlot) *
                         Factory.NativeProxy.GetActualSpriteHeight(spriteSlot);
            }
            else
            {
                height = Factory.NativeProxy.GetRelativeSpriteHeight(spriteSlot);
            }
            return height;
        }

        private int GetSpriteWidthForGameResolution(int spriteSlot)
        {
            int width;
            if (Factory.AGSEditor.CurrentGame.IsHighResolution)
            {
                width = Factory.NativeProxy.GetSpriteResolutionMultiplier(spriteSlot) *
                        Factory.NativeProxy.GetActualSpriteWidth(spriteSlot);
            }
            else
            {
                width = Factory.NativeProxy.GetRelativeSpriteWidth(spriteSlot);
            }
            return width;
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
                int scale = state.ScaleFactor;

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
            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                if (character.StartingRoom == _room.Number)
                {
                    defaultPropertyObjectList.Add(character.PropertyGridTitle, character);
                }
            }

            Factory.GUIController.SetPropertyGridObjectList(defaultPropertyObjectList);
        }

        private void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is Character)
            {
                _selectedCharacter = (Character)newPropertyObject;
                _panel.Invalidate();
            }
            else if (newPropertyObject is Room)
            {
                _selectedCharacter = null;
                _panel.Invalidate();
            }
        }

        public RoomAreaMaskType MaskToDraw
        {
            get { return RoomAreaMaskType.None; }
        }

        public int ItemCount
        {
            get { return 0; } // TODO ?
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

        public void DoubleClick(RoomEditorState state)
        {
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
    }
}
