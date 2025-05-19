using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// ObjectsEditorFilter manages RoomObjects.
    /// </summary>
    public class ObjectsEditorFilter : BaseThingEditorFilter<RoomObject>
    {
        private const string MENU_ITEM_DELETE = "DeleteObject";
        private const string MENU_ITEM_NEW = "NewObject";
        private const string MENU_ITEM_OBJECT_COORDS = "ObjectCoordinates";
        private List<RoomObject> _objectBaselines = new List<RoomObject>();

        public ObjectsEditorFilter(Panel displayPanel, RoomSettingsEditor editor, Room room)
            : base(displayPanel, editor, room)
        {
            // Init a starting list of item references for navigation UI
            InitRoomItemRefs(CollectItemRefs());

        }

        public override string Name { get { return "Objects"; } }
        public override string DisplayName { get { return "Objects"; } }

        public override event EventHandler OnItemsChanged;
        public override event EventHandler<SelectedRoomItemEventArgs> OnSelectedItemChanged;
        public override event EventHandler<RoomFilterContextMenuArgs> OnContextMenu;

		public override string HelpKeyword
		{
			get { return string.Empty; }
		}

        public override void Paint(Graphics graphics, RoomEditorState state)
        {
            _objectBaselines.Clear();
            _objectBaselines.AddRange(_room.Objects.Select(o =>
            {
                o.EffectiveBaseline = o.Baseline <= 0 ? o.StartY : o.Baseline;
                return o;
            }));
            _objectBaselines.Sort();

            foreach (RoomObject obj in _objectBaselines.Where(o => DesignItems[GetItemID(o)].Visible))
            {
                Size spriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(obj.Image);
                spriteSize.Width = state.RoomSizeToWindow(spriteSize.Width);
                spriteSize.Height = state.RoomSizeToWindow(spriteSize.Height);
                int xpos = state.RoomXToWindow(obj.StartX);
                int ypos = state.RoomYToWindow(obj.StartY + 1) - spriteSize.Height;

                Utilities.DrawSpriteOnGraphics(graphics, obj.Image, xpos, ypos, spriteSize.Width, spriteSize.Height);
            }

            if (!Enabled || _selectedObject == null)
                return;

            DesignTimeProperties design = DesignItems[GetItemID(_selectedObject)];
            if (!design.Visible)
                return;

            int width, height;
            Utilities.GetSizeSpriteWillBeRenderedInGame(_selectedObject.Image, out width, out height);
            width = state.RoomSizeToWindow(width);
            height = state.RoomSizeToWindow(height);
            int xPos = state.RoomXToWindow(_selectedObject.StartX);
            int yPos = state.RoomYToWindow(_selectedObject.StartY + 1) - height;
            Pen pen = new Pen(Color.Goldenrod);
            pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;
            graphics.DrawRectangle(pen, xPos, yPos, width, height);

            if (IsMovingObject)
            {
                Brush shadeBrush = new SolidBrush(Color.FromArgb(200, Color.Black));
                System.Drawing.Font font = new System.Drawing.Font("Arial", 10.0f);
                string toDraw = String.Format("X:{0}, Y:{1}", _selectedObject.StartX, _selectedObject.StartY);

                var textSize = graphics.MeasureString(toDraw, font);
                int scaledx = xPos + (width / 2) - ((int)textSize.Width / 2);
                int scaledy = yPos - (int)textSize.Height;
                if (scaledx < 0) scaledx = 0;
                if (scaledy < 0) scaledy = 0;
                if (scaledx + textSize.Width >= graphics.VisibleClipBounds.Width)
                    scaledx = (int)(graphics.VisibleClipBounds.Width - textSize.Width);
                if (scaledy + textSize.Height >= graphics.VisibleClipBounds.Height)
                    scaledy = (int)(graphics.VisibleClipBounds.Height - textSize.Height);

                graphics.FillRectangle(shadeBrush, scaledx, scaledy, textSize.Width, textSize.Height);
                graphics.DrawString(toDraw, font, pen.Brush, (float)scaledx, (float)scaledy);
            }
            else if (design.Locked)
            {
                pen = new Pen(Color.Goldenrod, 2);
                xPos = state.RoomXToWindow(_selectedObject.StartX) + (width / 2);
                yPos = state.RoomYToWindow(_selectedObject.StartY) - (height / 2);
                Point center = new Point(xPos, yPos);

                graphics.DrawLine(pen, center.X - 3, center.Y - 3, center.X + 3, center.Y + 3);
                graphics.DrawLine(pen, center.X - 3, center.Y + 3, center.X + 3, center.Y - 3);
            }
        }

        private RoomObject GetObject(int x, int y)
        {
            for (int i = _objectBaselines.Count - 1; i >= 0; i--)
            {
                RoomObject obj = _objectBaselines[i];
                DesignTimeProperties p = DesignItems[GetItemID(obj)];
                if (!p.Visible) continue;
                if (HitTest(obj, x, y)) return obj;
            }
            return null;
        }

        private bool HitTest(RoomObject obj, int x, int y)
        {
            int width, height;
            Utilities.GetSizeSpriteWillBeRenderedInGame(obj.Image, out width, out height);
            return ((x >= obj.StartX) && (x < obj.StartX + width) &&
                (y >= obj.StartY - height) && (y < obj.StartY));
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            ToolStripMenuItem item = (ToolStripMenuItem)sender;
            if (item.Name == MENU_ITEM_DELETE)
            {
                if (Factory.GUIController.ShowQuestion("Are you sure you want to delete this object?") == DialogResult.Yes)
                {
                    _room.Objects.Remove(_selectedObject);
                    _objectBaselines.Remove(_selectedObject);
                    RemoveObjectRef(_selectedObject);
                    foreach (RoomObject obj in _room.Objects)
                    {
                        if (obj.ID >= _selectedObject.ID)
                        {
                            string oldID = GetItemID(obj);
                            obj.ID--;
                            UpdateObjectRef(obj, oldID);
                        }
                    }
                    OnItemsChanged(this, null);
                    _selectedObject = null;
                    Factory.GUIController.SetPropertyGridObject(_room);
                    SetPropertyGridList();
                    _room.Modified = true;
                    _panel.Invalidate();
                }
            }
            else if (item.Name == MENU_ITEM_NEW)
            {
                if (_room.Objects.Count >= Room.MAX_OBJECTS)
                {
                    Factory.GUIController.ShowMessage("This room already has the maximum " + Room.MAX_OBJECTS + " objects.", MessageBoxIcon.Information);
                    return;
                }
                RoomObject newObj = new RoomObject(_room);
                newObj.ID = _room.Objects.Count;
                newObj.Name = Factory.AGSEditor.GetFirstAvailableScriptName("oObject", 0, _room);
                newObj.StartX = MenuClickPos.X;
                newObj.StartY = MenuClickPos.Y;
                newObj.Interactions.ScriptModule = _room.Interactions.ScriptModule;
                _room.Objects.Add(newObj);
                AddObjectRef(newObj);
                OnItemsChanged(this, null);
                SetSelectedObject(newObj);
                SetPropertyGridList();
                Factory.GUIController.SetPropertyGridObject(newObj);
                _room.Modified = true;
                _panel.Invalidate();
            }
            else if (item.Name == MENU_ITEM_OBJECT_COORDS)
            {
                int tempx = _selectedObject.StartX;
                int tempy = _selectedObject.StartY;
                string textToCopy = tempx.ToString() + ", " + tempy.ToString();
                Utilities.CopyTextToClipboard(textToCopy);
            }
        }

        protected override void ShowContextMenu(MouseEventArgs e, RoomEditorState state)
        {
            EventHandler onClick = new EventHandler(ContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            if (_selectedObject != null)
            {
                menu.Items.Add(new ToolStripMenuItem("Delete", null, onClick, MENU_ITEM_DELETE));
                menu.Items.Add(new ToolStripSeparator());
            }
            menu.Items.Add(new ToolStripMenuItem("Place New Object Here", null, onClick, MENU_ITEM_NEW));
            if (_selectedObject != null)
            {
                menu.Items.Add(new ToolStripMenuItem("Copy Object Coordinates to Clipboard", null, onClick, MENU_ITEM_OBJECT_COORDS));
            }
            OnContextMenu?.Invoke(this, new RoomFilterContextMenuArgs(menu, e.X, e.Y));
            menu.Show(_panel, e.X, e.Y);
        }

		public override bool DoubleClick(RoomEditorState state)
		{
			if (_lastSelectedObject != null)
			{
				Sprite chosenSprite = SpriteChooser.ShowSpriteChooser(_lastSelectedObject.Image);
				if (chosenSprite != null && chosenSprite.Number != _lastSelectedObject.Image)
				{
					_lastSelectedObject.Image = chosenSprite.Number;
					_room.Modified = true;
				}
                return true;
			}
            return false;
		}

        protected override void FilterActivated()
        {
        }

        protected override void FilterDeactivated()
        {
        }

        public override void CommandClick(string command)
        {
        }

        /// <summary>
        /// Initialize dictionary of current item references.
        /// </summary>
        protected SortedDictionary<string, RoomObject> CollectItemRefs()
        {
            SortedDictionary<string, RoomObject> items = new SortedDictionary<string, RoomObject>();
            foreach (RoomObject obj in _room.Objects)
            {
                items.Add(GetItemID(obj), obj);
            }
            return items;
        }

        /// <summary>
        /// Gets this object's script name.
        /// </summary>
        protected override string GetItemScriptName(RoomObject obj)
        {
            return obj.Name;
        }

        /// <summary>
        /// Forms a PropertyGrid's entry title for this object.
        /// </summary>
        protected override string GetPropertyGridItemTitle(RoomObject obj)
        {
            return obj.PropertyGridTitle;
        }

        /// <summary>
        /// Forms a object's label for the Navigation bar.
        /// </summary>
        protected override string GetNavbarItemTitle(RoomObject obj)
        {
            return MakeLayerItemName("Object", obj.Name, obj.Description, obj.ID);
        }

        public override bool AllowClicksInterception()
        {
            return true;
        }

        protected override void GUIController_OnPropertyObjectChanged(object newPropertyObject)
        {
            if (newPropertyObject is RoomObject)
            {
                SetSelectedObject((RoomObject)newPropertyObject);                
                _panel.Invalidate();
            }
            else if (newPropertyObject is Room)
            {
                _selectedObject = null;
                _panel.Invalidate();
            }
        }

        protected override string GetItemID(RoomObject obj)
        {
            // Use numeric object's ID as a "unique identifier", for now (script name is optional!)
            return obj.ID.ToString("D4");
        }

        /// <summary>
        /// Tries to get an object under given coordinates.
        /// Returns null if no object was found.
        /// </summary>
        protected override RoomObject GetObjectAtCoords(int x, int y, RoomEditorState state)
        {
            return GetObject(x, y);
        }

        /// <summary>
        /// Gets current object's position.
        /// </summary>
        protected override void GetObjectPosition(RoomObject obj, out int curX, out int curY)
        {
            curX = obj.StartX;
            curY = obj.StartY;
        }

        /// <summary>
        /// Tries to assign new position in room for the given object.
        /// Returns if anything has changed as a result.
        /// </summary>
        protected override bool SetObjectPosition(RoomObject obj, int newX, int newY)
        {
            if ((newX != _selectedObject.StartX) ||
                (newY != _selectedObject.StartY))
            {
                _selectedObject.StartX = newX;
                _selectedObject.StartY = newY;
                return true;
            }
            return false;
        }

        /// <summary>
        /// Change object current selection.
        /// </summary>
        protected override void SetSelectedObject(RoomObject obj)
        {
            _selectedObject = obj;
            if (_selectedObject != null &&
                OnSelectedItemChanged != null)
            {
                OnSelectedItemChanged(this, new SelectedRoomItemEventArgs(GetNavbarItemTitle(_selectedObject)));
            }
            ClearMovingState();
        }
    }
}
