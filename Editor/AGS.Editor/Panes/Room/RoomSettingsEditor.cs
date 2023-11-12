using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Imaging;
using System.Data;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using AGS.Editor.Panes.Room;
using AddressBarExt;
using AddressBarExt.Controls;
using System.Linq;
using System.Drawing.Drawing2D;

namespace AGS.Editor
{
    public partial class RoomSettingsEditor : EditorContentPanel
    {
        private const int SCROLLBAR_WIDTH_BUFFER = 40;
        private const string MENU_ITEM_COPY_COORDS = "CopyCoordinatesToClipboard";

        // NOTE: the reason we need to pass editor reference to the SaveRoom hander is that
        // currently design-time properties of room items are stored inside editor filter classes.
        public delegate bool SaveRoomHandler(Room room, RoomSettingsEditor editor);
        public event SaveRoomHandler SaveRoom;
        public delegate void AbandonChangesHandler(Room room);
        public event AbandonChangesHandler AbandonChanges;

        private readonly Room _room;
        private readonly IRoomController _roomController;
        private IRoomEditorFilter _layer;
        private RoomEditNode _layersRoot;
        private IRoomEditorFilter _emptyLayer;
        private List<IRoomEditorFilter> _layers = new List<IRoomEditorFilter>();
        private CharactersEditorFilter _characterLayer; // need to store the reference for special processing
        private bool _editorConstructed = false;
        private bool _mouseIsDown = false;
        private int _menuClickX;
        private int _menuClickY;
        private object _startNode; // track breadcrumbs path so that it can be compared when saving

        private const int ZOOM_STEP_VALUE = 25;
        private const int ZOOM_MAX_VALUE = 600;
        private bool _isChangingZoom = false;
        private RoomEditorState _state = new RoomEditorState();

        /// <summary>
        /// Room editor item layers.
        /// </summary>
        public IEnumerable<IRoomEditorFilter> Layers { get { return _layers; } }

        /// <summary>
        /// Selected navbar's node.
        /// </summary>
        public IAddressNode CurrentNode { get { return _editAddressBar.CurrentNode; } }

        internal static Cursor LockedCursor { get; private set; }

        /// <summary>
        /// Tells if the design-time properties of the room were modified since last save.
        /// </summary>
        public bool DesignModified
        {
            get
            {
                foreach (IRoomEditorFilter layer in _layers)
                    if (layer.Modified) return true;

                if (_startNode != _editAddressBar.CurrentNode.UniqueID)
                    return true;

                return false;
            }
            set
            { // Kind of ugly, I know...
                foreach (IRoomEditorFilter layer in _layers)
                    layer.Modified = value;

                _startNode = _editAddressBar.CurrentNode.UniqueID;
            }
        }


        public RoomSettingsEditor(Room room, IRoomController roomController)
        {
            if (LockedCursor == null)
            {
                LockedCursor = Resources.ResourceManager.GetCursor("lock_cur.cur");
            }

            this.SetStyle(ControlStyles.Selectable, true);

            InitializeComponent();
            _room = room;
            _roomController = roomController;
            sldZoomLevel.Maximum = ZOOM_MAX_VALUE / ZOOM_STEP_VALUE;
            sldZoomLevel.Value = 100 / ZOOM_STEP_VALUE;
            // TODO: choose default zoom based on the room size vs window size?
            SetZoomSliderToMultiplier(_room.Width <= 320 ? 2 : 1);

            _emptyLayer = new EmptyEditorFilter(bufferedPanel1, _room);
            _layers.Add(new EdgesEditorFilter(bufferedPanel1, _room));
            _characterLayer = new CharactersEditorFilter(bufferedPanel1, this, _room, Factory.AGSEditor.CurrentGame);
            _layers.Add(_characterLayer);
            _layers.Add(new ObjectsEditorFilter(bufferedPanel1, this, _room));
            _layers.Add(new HotspotsEditorFilter(bufferedPanel1, this, _room, _roomController));
            _layers.Add(new WalkableAreasEditorFilter(bufferedPanel1, this, _room, _roomController));
            _layers.Add(new WalkBehindsEditorFilter(bufferedPanel1, this, _room, _roomController));
            _layers.Add(new RegionsEditorFilter(bufferedPanel1, this, _room, _roomController));

            foreach (IRoomEditorFilter layer in _layers)
            {
                layer.OnContextMenu += layer_OnContextMenu;
                layer.OnItemsChanged += layer_OnItemsChanged;
                layer.OnSelectedItemChanged += layer_OnSelectedItemChanged;
            }
            
            RefreshLayersTree();
            _editAddressBar.SelectionChange += editAddressBar_SelectionChange;
            _startNode = _editAddressBar.CurrentNode.UniqueID;

            RepopulateBackgroundList(0);
            UpdateScrollableWindowSize();
            sldZoomLevel.Maximum = ZOOM_MAX_VALUE / ZOOM_STEP_VALUE;
            sldZoomLevel.Value = 100 / ZOOM_STEP_VALUE;
            // TODO: choose default zoom based on the room size vs window size?
            SetZoomSliderToMultiplier(_room.Width <= 320 ? 2 : 1);

            MouseWheel += new MouseEventHandler(RoomSettingsEditor_MouseWheel);
            bufferedPanel1.MouseWheel += new MouseEventHandler(RoomSettingsEditor_MouseWheel);
            sldZoomLevel.MouseWheel += new MouseEventHandler(RoomSettingsEditor_MouseWheel);
            cmbBackgrounds.MouseWheel += new MouseEventHandler(RoomSettingsEditor_MouseWheel);
            bufferedPanel1.PanButtons = MouseButtons.Middle;

            _editorConstructed = true;
        }

        /// <summary>
        /// Update the breadcrumb navigation bar, make all nodes correspond to the design-time state
        /// of the room layers and items.
        /// </summary>
        /// TODO: currently this is the only way to sync navbar with the design-time properties.
        /// find a better solution, perhaps tie each DesignTimeProperties object to a bar node.
        public void RefreshLayersTree()
        {
            IAddressNode currentNode = _editAddressBar.CurrentNode;
            IAddressNode[] layers = new IAddressNode[_layers.Count];
            VisibleLayerRadioGroup visibleLayerRadioGroup = new VisibleLayerRadioGroup();
            for (int layerIndex = 0; layerIndex < layers.Length; layerIndex++)
            {
                IRoomEditorFilter layer = _layers[layerIndex];                
                IAddressNode[] children = new IAddressNode[layer.DesignItems.Count];
                int index = 0;
                foreach (var item in layer.DesignItems)
                {
                    string id = item.Key;
                    string name = layer.GetItemName(id);
                    children[index++] = new RoomEditNode(GetLayerItemUniqueID(layer, name), name, id,
                        new IAddressNode[0], item.Value.Visible, item.Value.Locked, false);
                }
                RoomEditNode node = new RoomEditNode(layer.DisplayName, children, layer.Visible, layer.Locked);
                node.Layer = layer;
                if (layer is BaseAreasEditorFilter)
                {
                    node.VisibleGroup = visibleLayerRadioGroup;
                }
                foreach (IAddressNode child in children)
                {
                    child.Parent = node;
                }
                layers[layerIndex] = node;
            }
            _layersRoot = new RoomEditNode("Room", layers, true, false);
            foreach (IAddressNode layer in layers)
            {
                layer.Parent = _layersRoot;
            }
            _editAddressBar.InitializeRoot(_layersRoot);

            SelectOldNode(currentNode);
        }

        /// <summary>
        /// Attempts to select room node following the given path item by item.
        /// Path elements are compared with RoomItemID or UniqueID if former is not set.
        /// </summary>
        public bool TrySelectNodeUsingDesignIDPath(string[] path)
        {
            if (path.Length < 1) return false;
            if (path[0].CompareTo(_layersRoot.UniqueID) != 0) return false;
            IAddressNode node = _layersRoot;
            for (int i = 1; i < path.Length; ++i)
            {
                foreach (IAddressNode child in node.Children)
                {
                    var roomNode = child as RoomEditNode;
                    if (roomNode != null && !string.IsNullOrEmpty(roomNode.RoomItemID))
                    {
                        if (roomNode.RoomItemID.CompareTo(path[i]) == 0)
                        {
                            node = roomNode;
                            continue;
                        }
                    }
                    else
                    {
                        if (child.UniqueID.ToString().CompareTo(path[i]) == 0)
                        {
                            node = child;
                            continue;
                        }
                    }
                }
            }
            _editAddressBar.CurrentNode = node;
            return true;
        }

        /// <summary>
        /// Invalidate the surface of the drawing control and causes the control to be redrawn
        /// </summary>
        public void InvalidateDrawingBuffer() => bufferedPanel1.Invalidate();
        
        private void SelectOldNode(IAddressNode currentNode)
        {
            if (currentNode == null) return;

            IAddressNode selectedNode = _layersRoot.GetChild(currentNode.UniqueID, true);
            if (selectedNode != null)
            {
                _editAddressBar.CurrentNode = selectedNode;
                return;
            }
            if (currentNode.Parent == null)
            {
                _editAddressBar.CurrentNode = _layersRoot;
                return;
            }
            selectedNode = _layersRoot.GetChild(currentNode.Parent.UniqueID, true);
            if (selectedNode == null)
            {
                _editAddressBar.CurrentNode = _layersRoot;
                return;
            }
            //We found the old node's parent, but not the node itself, so it was probably renamed.
            //To find the renamed item, we need to find the item that doesn't exist in the old tree.
            foreach (IAddressNode child in selectedNode.Children)
            {
                bool foundChild = false;
                foreach (IAddressNode oldChild in currentNode.Parent.Children)
                {
                    if (oldChild.UniqueID.Equals(child.UniqueID))
                    {
                        foundChild = true;
                        break;
                    }
                }
                if (!foundChild)
                {
                    _editAddressBar.CurrentNode = child;
                    return;
                }
            }
            //We didn't find any missing node, lets at least select the parent
            _editAddressBar.CurrentNode = selectedNode;            
        }

        private void layer_OnItemsChanged(object sender, EventArgs e)
        {
            RefreshLayersTree();            
        }

        private void layer_OnSelectedItemChanged(object sender, SelectedRoomItemEventArgs e)
        {
            IRoomEditorFilter layer = sender as IRoomEditorFilter;
            if (layer == null) { SelectLayer(null); return; }
            IAddressNode node = _editAddressBar.RootNode.GetChild(GetLayerItemUniqueID(layer, e.Item), true);
            if (node == null) { SelectLayer(null); return; }
            _editAddressBar.CurrentNode = node;
            SelectLayer(layer);
            //selecting hotspot from designer Panel, then cant Draw more hotspots...
        }

        /// <summary>
        /// Room layer wants to display context menu, we shall add our items too.
        /// </summary>
        private void layer_OnContextMenu(object sender, RoomFilterContextMenuArgs e)
        {
            e.Menu.Items.Add(new ToolStripSeparator());
            PrepareContextMenu(e.Menu, e.X, e.Y);
        }

        private string GetLayerItemUniqueID(IRoomEditorFilter layer, string itemName)
        {
            return layer.DisplayName + itemName;
        }
                
        private void RoomSettingsEditor_MouseWheel(object sender, MouseEventArgs e)
		{
            // Ctrl + Wheel = zoom
            if (ModifierKeys == Keys.Control)
            {
                // For zooming with mouse wheel we use current mouse position as anchor
                // (ofcourse we need to translate it to the panel position)
                Point anchor = bufferedPanel1.PointToClient(Cursor.Position);
                if (e.Delta > 0)
                {
                    if (sldZoomLevel.Value < sldZoomLevel.Maximum)
                    {
                        ChangeZoom(sldZoomLevel.Value + 1, anchor);
                    }
                }
                else
                {
                    if (sldZoomLevel.Value > sldZoomLevel.Minimum)
                    {
                        ChangeZoom(sldZoomLevel.Value - 1, anchor);
                    }
                }
            }
            // Shift + Wheel = scroll horizontal
            else if (ModifierKeys == Keys.Shift)
            {
                bufferedPanel1.AutoScrollPosition = new Point(Math.Abs(bufferedPanel1.AutoScrollPosition.X) - e.Delta, Math.Abs(bufferedPanel1.AutoScrollPosition.Y));
            }
            // Wheel without modifiers = scroll vertical
            else if(ModifierKeys == Keys.None)
            {
                bufferedPanel1.AutoScrollPosition = new Point(Math.Abs(bufferedPanel1.AutoScrollPosition.X), Math.Abs(bufferedPanel1.AutoScrollPosition.Y) - e.Delta);
            }
            // Ridiculous solution, found on stackoverflow.com
            // TODO: check again later, how reliable this is?!
            HandledMouseEventArgs ee = (HandledMouseEventArgs)e;
            ee.Handled = true;
		}

        private void SetZoomSliderToMultiplier(int factor)
        {
            sldZoomLevel.Value = (100 * factor) / ZOOM_STEP_VALUE;
            ChangeZoom(sldZoomLevel.Value, Point.Empty);
        }

        private void UpdateScrollableWindowSize()
        {
            bufferedPanel1.AutoScrollMinSize = new Size(_state.RoomSizeToWindow(_room.Width), _state.RoomSizeToWindow(_room.Height));
        }

        private void RepopulateBackgroundList(int selectIndex) 
        {
            cmbBackgrounds.Items.Clear();
            for (int i = 0; i < _room.BackgroundCount; i++)
            {
                if (i == 0)
                {
                    cmbBackgrounds.Items.Add("Main background");
                }
                else
                {
                    cmbBackgrounds.Items.Add("Background " + i);
                }
            }
            if (_room.BackgroundCount < Room.MAX_BACKGROUNDS)
            {
                cmbBackgrounds.Items.Add("<Import new background...>");
            }
            cmbBackgrounds.SelectedIndex = selectIndex;
        }

        private void bufferedPanel1_Paint(object sender, PaintEventArgs e)
        {
            _state.Offset = new Point(-bufferedPanel1.AutoScrollPosition.X, -bufferedPanel1.AutoScrollPosition.Y);

            int backgroundNumber = cmbBackgrounds.SelectedIndex;
            if (backgroundNumber < _room.BackgroundCount)
            {
                e.Graphics.SetClip(new Rectangle(0, 0, bufferedPanel1.ClientSize.Width + SystemInformation.VerticalScrollBarWidth, bufferedPanel1.ClientSize.Height + SystemInformation.HorizontalScrollBarHeight));
                e.Graphics.Clear(Color.LightGray);
                e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
                e.Graphics.SmoothingMode = SmoothingMode.None;
                e.Graphics.PixelOffsetMode = PixelOffsetMode.Half;
                e.Graphics.SetClip(new Rectangle(0, 0, _state.RoomSizeToWindow(_room.Width), _state.RoomSizeToWindow(_room.Height)));

                // Adjust co-ordinates using original scale factor so that it lines
                // up with objects, etc
                Point drawOffs = new Point(_state.RoomXToWindow(0), _state.RoomYToWindow(0));
                IRoomEditorFilter maskFilter = GetCurrentMaskFilter();
                lock (_room)
                {
                    _roomController.DrawRoomBackground(
                        e.Graphics,
                        drawOffs,
                        backgroundNumber,
                        _state.Scale,
                        maskFilter?.MaskToDraw ?? RoomAreaMaskType.None,
                        sldTransparency.Value,
                        maskFilter == null || !maskFilter.Enabled ? 0 : maskFilter.SelectedArea);
                }

                foreach (IRoomEditorFilter layer in _layers.Where(l => IsVisible(l)))
                {
                    layer.Paint(e.Graphics, _state);
                }
            }
            base.OnPaint(e);
        }

        private IRoomEditorFilter GetCurrentMaskFilter()
        {
            foreach (IRoomEditorFilter filter in _layers)
            {
                if (!(filter is BaseAreasEditorFilter)) continue;
                if (IsVisible(filter)) return filter;
            }
            return null;
        }

        private bool IsVisible(IRoomEditorFilter layer)
        {
            return layer.Visible;
        }

        private bool IsLocked(IRoomEditorFilter layer)
        {
            if (!layer.Visible) return true;
            if (_layer != null && !_layer.AllowClicksInterception() && _layer != layer) return true;
            return layer.Locked;
        }

        private void cmbBackgrounds_SelectedIndexChanged(object sender, EventArgs e)
        {
            if ((cmbBackgrounds.SelectedIndex == cmbBackgrounds.Items.Count - 1) &&
                (_room.BackgroundCount < Room.MAX_BACKGROUNDS))
            {
                if ((_room.BackgroundCount > 1) ||
                    (Factory.GUIController.ShowQuestion("You are about to import an extra background into this room; this is used for switching backgrounds in game, or creating full-size background animations. Are you sure this is what you want to do?\n\nIf you just want to change the background image, use the 'Change' button instead.", MessageBoxIcon.Question) == DialogResult.Yes))
                {
                    ImportBackground(cmbBackgrounds.SelectedIndex);
                    RepopulateBackgroundList(_room.BackgroundCount - 1);
                }
                else
                {
                    cmbBackgrounds.SelectedIndex = 0;
                }
            }
            else
            {
                bufferedPanel1.Invalidate();
            }

            btnDelete.Enabled = (cmbBackgrounds.SelectedIndex > 0);
        }

        private Bitmap ExtendBitmapIfSmallerThanScreen(Bitmap source)
        {
            if ((source.Width < Factory.AGSEditor.CurrentGame.MinRoomWidth) ||
                (source.Height < Factory.AGSEditor.CurrentGame.MinRoomHeight))
            {
                int newWidth = Math.Max(source.Width, Factory.AGSEditor.CurrentGame.MinRoomWidth);
                int newHeight = Math.Max(source.Height, Factory.AGSEditor.CurrentGame.MinRoomHeight);
                if (source.PixelFormat != System.Drawing.Imaging.PixelFormat.Format8bppIndexed)
                {
                    Bitmap paddedBmp = new Bitmap(newWidth, newHeight, source.PixelFormat);
                    Graphics g = Graphics.FromImage(paddedBmp);
                    g.Clear(Color.Black);
					// Have to specify Width and Height to stop it using
					// the DPI of the image and drawing at different size
					g.DrawImage(source, 0, 0, source.Width, source.Height);
                    g.Dispose();
                    source.Dispose();
                    return paddedBmp;
                }
                else
                {
					return ExtendCanvasSizeOf8BitBitmap(source, newWidth, newHeight);
                }
            }
            return source;
        }

		/// <summary>
		/// The built-in .NET drawing routines don't work with 8-bit images, so
		/// we have to do this manually. How rubbish. The source image is
		/// destroyed and a new one returned to replace it.
		/// </summary>
		private Bitmap ExtendCanvasSizeOf8BitBitmap(Bitmap source, int newWidth, int newHeight)
		{
			if (source.PixelFormat != PixelFormat.Format8bppIndexed)
			{
				throw new AGSEditorException("This function only works with 8-bit images; use built-in .net routines for higher colour depths");
			}
			int bytesPerPixel = 1;
			Bitmap paddedBmp = new Bitmap(newWidth, newHeight, source.PixelFormat);
			Rectangle sourceRect = new Rectangle(0, 0, source.Width, source.Height);
			BitmapData sourceData = source.LockBits(sourceRect, ImageLockMode.ReadOnly, source.PixelFormat);
			Rectangle destRect = new Rectangle(0, 0, paddedBmp.Width, paddedBmp.Height);
			BitmapData bmpData = paddedBmp.LockBits(destRect, ImageLockMode.WriteOnly, paddedBmp.PixelFormat);
			int srcMemoryAddress = sourceData.Scan0.ToInt32();
			int memoryAddress = bmpData.Scan0.ToInt32();
			for (int y = 0; y < newHeight; y++)
			{
				byte[] line = new byte[bmpData.Stride];
				Array.Clear(line, 0, line.Length);
				if (y < source.Height)
				{
					Marshal.Copy(new IntPtr(srcMemoryAddress), line, 0, source.Width * bytesPerPixel);
				}
				Marshal.Copy(line, 0, new IntPtr(memoryAddress), line.Length);

				srcMemoryAddress += sourceData.Stride;
				memoryAddress += bmpData.Stride;
			}
			paddedBmp.UnlockBits(bmpData);
			source.UnlockBits(sourceData);

			ColorPalette sourcePalette = source.Palette;
			ColorPalette destPalette = paddedBmp.Palette;
			for (int i = 0; i < sourcePalette.Entries.Length; i++)
			{
				destPalette.Entries[i] = sourcePalette.Entries[i];
			}
			// The palette needs to be re-set onto the bitmap to force it
			// to update its internal storage of the colours
			paddedBmp.Palette = destPalette;

			source.Dispose();
			return paddedBmp;
		}

        private void ImportBackground(int bgIndex)
        {
            string selectedFile = Factory.GUIController.ShowOpenFileDialog("Select background to import...", Constants.IMAGE_FILE_FILTER);
            if (selectedFile != null)
            {
				Bitmap bmp = null;
                try
                {
                    bmp = new Bitmap(selectedFile);
                    bmp = ExtendBitmapIfSmallerThanScreen(bmp);
                    bool doImport = true;
                    bool deleteExtraFrames = false;

					if ((bmp.Width != _room.Width) || (bmp.Height != _room.Height))
                    {
                        if (bgIndex > 0)
                        {
                            Factory.GUIController.ShowMessage("This image is a different size to the main background image. All backgrounds for the room must be of the same size." + Environment.NewLine + Environment.NewLine + "Main background: " + _room.Width + " x " + _room.Height + "; this image: " + bmp.Width + " x " + bmp.Height, MessageBoxIcon.Warning);
                            doImport = false;
                        }
                        else if (_room.BackgroundCount > 1)
                        {
                            if (Factory.GUIController.ShowQuestion("Changing the size of the main background will mean that all extra backgrounds are deleted. Are you sure you want to do this?") != DialogResult.Yes)
                            {
                                doImport = false;
                            }
                            else
                            {
                                deleteExtraFrames = true;
                            }
                        }
                        if (doImport)
                        {
                            if (Factory.GUIController.ShowQuestion("The new background is a different size to the old one. If you import it, all your regions, hotspots and walkable areas will be cleared. Do you want to proceed?") != DialogResult.Yes)
                            {
                                doImport = false;
                            }
                        }
                    }
                    if (doImport)
                    {
                        _roomController.SetBackground(bgIndex, bmp);

                        if (deleteExtraFrames)
                        {
                            while (_room.BackgroundCount > 1)
                            {
                                _roomController.DeleteBackground(1);
                            }
                            RepopulateBackgroundList(0);
                        }

                        // TODO: choose default zoom based on the room size vs window size?
                        SetZoomSliderToMultiplier(_room.Width <= 320 ? 2 : 1);
                    }
                }
                catch (Exception ex)
                {
                    Factory.GUIController.ShowMessage("The background could not be imported. The error was:" + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
                }
				if (bmp != null)
				{
					bmp.Dispose();
				}
            }
        }

        private void btnChangeImage_Click(object sender, EventArgs e)
        {
            ImportBackground(cmbBackgrounds.SelectedIndex);
            bufferedPanel1.Invalidate();
            Factory.GUIController.RefreshPropertyGrid();
        }

        private void btnDelete_Click(object sender, EventArgs e)
        {
            if (Factory.GUIController.ShowQuestion("Are you sure you want to delete this background?") == DialogResult.Yes)
            {
                _roomController.DeleteBackground(cmbBackgrounds.SelectedIndex);
                RepopulateBackgroundList(0);
            }
        }

        private void btnExport_Click(object sender, EventArgs e)
        {
            string fileName = Factory.GUIController.ShowSaveFileDialog("Export background as...", Constants.IMAGE_FILE_FILTER);
            if (fileName != null)
            {
                using (Bitmap bmp = _roomController.GetBackground(cmbBackgrounds.SelectedIndex))
                {
                    ImportExport.ExportBitmapToFile(fileName, bmp);
                }
            }
        }

        private void bufferedPanel1_MouseDown(object sender, MouseEventArgs e)
        {
            if (bufferedPanel1.IsPanning) return;
            bool handled = false;
            if (_layer != null && !IsLocked(_layer))
            {
                handled = _layer.MouseDown(e, _state);
            }
            if (!handled)
            {
                if (e.Button == MouseButtons.Right &&
                    (ModifierKeys == Keys.None || ModifierKeys == Keys.Shift))
                {
                    ShowContextMenu(e.X, e.Y);
                }
            }
            _mouseIsDown = true;
		}

        private void bufferedPanel1_MouseUp(object sender, MouseEventArgs e)
        {
            if (_mouseIsDown)
            {
                bool handled = false;
                if (_layer != null && !IsLocked(_layer))
                {
                    handled = _layer.MouseUp(e, _state);
                }
                Factory.GUIController.RefreshPropertyGrid();
                bufferedPanel1.Invalidate();
                _mouseIsDown = false;
            }
			SetFocusToAllowArrowKeysToWork();
		}

		private void SetFocusToAllowArrowKeysToWork()
		{
			cmbBackgrounds.Focus();
		}

		private void bufferedPanel1_DoubleClick(object sender, EventArgs e)
		{
            if (_layer != null && !IsLocked(_layer))
                _layer.DoubleClick(_state);
			Factory.GUIController.RefreshPropertyGrid();
			bufferedPanel1.Invalidate();
		}

		private void bufferedPanel1_MouseMove(object sender, MouseEventArgs e)
        {
            if (bufferedPanel1.IsPanning) return;
            int mouseXPos = _state.WindowXToRoom(e.X);
            int mouseYPos = _state.WindowYToRoom(e.Y);
            string xPosText = mouseXPos.ToString();
            string yPosText = mouseYPos.ToString();
            if ((mouseXPos < 0) || (mouseXPos >= _room.Width))
            {
                xPosText = "?";
            }
            if ((mouseYPos < 0) || (mouseYPos >= _room.Height))
            {
                yPosText = "?";
            }
            lblMousePos.Text = $"{xPosText}, {yPosText}";

            SelectCursor(e.X, e.Y, _state);
            if (_layer != null && !IsLocked(_layer))
            {
                if (_layer.MouseMove(e.X, e.Y, _state))
                {
                    bufferedPanel1.Invalidate();
                }
            }
        }

        private void SelectCursor(int x, int y, RoomEditorState state)
        {
            if (_layer != null)
            {
                state.CurrentCursor = _layer.Locked ? LockedCursor : _layer.GetCursor(x, y, state);
            }
            else
            {
                state.CurrentCursor = Cursors.Default;
            }
            bufferedPanel1.Cursor = state.CurrentCursor ?? Cursors.Default;
        }

        private void editAddressBar_SelectionChange(object sender, NodeChangedArgs e)
        {
            if (_layersRoot.UniqueID.Equals(e.OUniqueID))
            {
                SetPropertyGridObject(_room);
                SelectLayer(null);
                return;
            }

            RoomEditNode node = _layersRoot.GetChild(e.OUniqueID, true) as RoomEditNode;
            if (node == null) { SelectLayer(null); return; }
            RoomEditNode layerNode = node;
            while (layerNode != null && layerNode.Layer == null)
            {
                layerNode = layerNode.Parent as RoomEditNode;
            }
            if (layerNode == null) { SelectLayer(null); return; }

            layerNode.IsVisible = true;
            SelectLayer(layerNode.Layer);
            layerNode.Layer.SelectItem(node == layerNode ? null : node.RoomItemID);
        }

        private void SelectLayer(IRoomEditorFilter layer)
        {
            if (!_editorConstructed) return;
            if (layer == _layer) return;

            if (_layer != null) _layer.FilterOff();
            _layer = layer ?? _emptyLayer;

            SetDefaultPropertyGridList();
            SetPropertyGridObject(_room);
            lblTransparency.Visible = _layer.ShowTransparencySlider;
            sldTransparency.Visible = _layer.ShowTransparencySlider;
            chkCharacterOffset.Visible = _layer is CharactersEditorFilter;
            
            _layer.FilterOn();

            bufferedPanel1.Invalidate();
            // ensure that shortcut keys do not move the combobox
            bufferedPanel1.Focus();            
        }
		
        private void SetDefaultPropertyGridList()
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(_room.PropertyGridTitle, _room);
            if (Factory.GUIController.ActivePane == this.ContentDocument)
            {
                Factory.GUIController.SetPropertyGridObjectList(list);
            }
            else
            {
                this.ContentDocument.PropertyGridObjectList = list;
                this.ContentDocument.SelectedPropertyGridObject = _room;
            }
        }

        private void SetPropertyGridObject(object obj)
        {
            if (Factory.GUIController.ActivePane == this.ContentDocument)
                Factory.GUIController.SetPropertyGridObject(obj);
            else
                this.ContentDocument.SelectedPropertyGridObject = obj;
        }

		protected override string OnGetHelpKeyword()
		{
            // CHECKME later: not implemented in the manual
            //if (_layer != null) 
			//    return _layer.HelpKeyword;
            return "Room Editor";
        }

		private bool DoesThisPanelHaveFocus()
		{
            return this.ActiveControl != null && this.ActiveControl.Focused;
            /*if (Utilities.IsMonoRunning())
            {
                return this.ActiveControl.Focused;
            }
			Control focused = Utilities.GetControlThatHasFocus();
			return (focused == this.ActiveControl);*/
		}

		private bool ProcessPanKeyPress(Keys keyData)
        {
            switch (keyData)
            {
            case Keys.Down:
	            bufferedPanel1.AutoScrollPosition = new Point(Math.Abs(bufferedPanel1.AutoScrollPosition.X), Math.Abs(bufferedPanel1.AutoScrollPosition.Y) + 50);
                return true;
            case Keys.Up:
	            bufferedPanel1.AutoScrollPosition = new Point(Math.Abs(bufferedPanel1.AutoScrollPosition.X), Math.Abs(bufferedPanel1.AutoScrollPosition.Y) - 50);
                return true;
            case Keys.Right:
	            bufferedPanel1.AutoScrollPosition = new Point(Math.Abs(bufferedPanel1.AutoScrollPosition.X) + 50, Math.Abs(bufferedPanel1.AutoScrollPosition.Y));
                return true;
            case Keys.Left:
	            bufferedPanel1.AutoScrollPosition = new Point(Math.Abs(bufferedPanel1.AutoScrollPosition.X) - 50, Math.Abs(bufferedPanel1.AutoScrollPosition.Y));
                return true;
            case Keys.Space:
                bufferedPanel1.PanButtons |= MouseButtons.Left;
                return true;
            }
			return false;
		}

        private bool ProcessPanKeyRelease(Keys keyData)
        {
            switch (keyData)
            {
                case Keys.Space:
                    bufferedPanel1.PanButtons &= ~MouseButtons.Left;
                    return true;
            }
            return false;
        }

		protected override bool HandleKeyPress(Keys keyData)
		{
            if (!DoesThisPanelHaveFocus())
            {
                return false;
            }

            if (_layer != null && !IsLocked(_layer) && _layer.KeyPressed(keyData))
            {
                bufferedPanel1.Invalidate();
                Factory.GUIController.RefreshPropertyGrid();
                return true;
            }
            return ProcessPanKeyPress(keyData);
		}

        protected override bool HandleKeyRelease(Keys keyData)
        {
            if (!DoesThisPanelHaveFocus())
            {
                return false;
            }

            return ProcessPanKeyRelease(keyData);
        }

        private void ShowContextMenu(int x, int y)
        {
            ContextMenuStrip menu = new ContextMenuStrip();
            PrepareContextMenu(menu, x, y);
            menu.Show(bufferedPanel1, x, y);
        }

        private void PrepareContextMenu(ContextMenuStrip menu, int x, int y)
        {
            EventHandler onClick = new EventHandler(CoordMenuEventHandler);
            menu.Items.Add(new ToolStripMenuItem("Copy mouse coordinates to clipboard", null, onClick, MENU_ITEM_COPY_COORDS));
            _menuClickX = _state.WindowXToRoom(x);
            _menuClickY = _state.WindowYToRoom(y);
        }

        private void CoordMenuEventHandler(object sender, EventArgs e)
        {
            int tempx = _menuClickX;
            int tempy = _menuClickY;
            string textToCopy = tempx.ToString() + ", " + tempy.ToString();
            Utilities.CopyTextToClipboard(textToCopy);
        }

        protected override void OnPanelClosing(bool canCancel, ref bool cancelClose)
        {
            if ((canCancel) && (_room.Modified || DesignModified))
            {
                DialogResult answer = MessageBox.Show("Do you want to save your changes to this room before closing it?", "Save changes?", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
                if (answer == DialogResult.Cancel)
                {
                    cancelClose = true;
                }
                else if (answer == DialogResult.Yes)
                {
                    if (SaveRoom != null)
                    {
                        cancelClose = !SaveRoom(_room.Modified ? _room : null, DesignModified ? this : null);
                    }
                }
                else if (AbandonChanges != null)
                {
                    AbandonChanges(_room);
                }
            }
        }

		protected override void OnPropertyChanged(string propertyName, object oldValue)
		{
			_room.Modified = true;
            bufferedPanel1.Invalidate();

            bool needRefresh = false;
            // TODO: unfortunately had to duplicate handling of property change here;
            // cannot forward event to the CharacterComponent.OnPropertyChanged,
            // because its implementation relies on it being active Pane!
            if (propertyName == Character.PROPERTY_NAME_STARTINGROOM)
            {
                if (_characterLayer != null)
                {
                    int oldRoom = (int)oldValue;
                    _characterLayer.UpdateCharactersRoom(_characterLayer.SelectedCharacter, oldRoom);
                    needRefresh = true;
                }
            }

            if (propertyName == RoomHotspot.PROPERTY_NAME_SCRIPT_NAME ||
				propertyName == RoomObject.PROPERTY_NAME_SCRIPT_NAME ||
                propertyName == Character.PROPERTY_NAME_SCRIPTNAME ||
                needRefresh)
			{
                if (_layer != null)
                {
                    // Force the layer to refresh its property list with the new name   
                    // TODO: find out if this hack can be avoided             
                    _layer.FilterOff();
                    _layer.FilterOn();
                }
                RefreshLayersTree();
			}
        }

		protected override void OnWindowActivated()
		{
			SetFocusToAllowArrowKeysToWork();
		}

        protected override void OnCommandClick(string command)
        {
            if (_layer == null) return;
            _layer.CommandClick(command);
        }

        protected override void OnDispose()
        {
            if (_layer != null) _layer.FilterOff();
            foreach (IRoomEditorFilter layer in _layers)
            {
                layer.Dispose();
            }
        }

        /// <summary>
        /// Set new zoom level and updates controls accordingly.
        /// </summary>
        /// <param name="zoom">Zoom level in ZOOM_STEP_VALUE percents.</param>
        /// <param name="anchor">The room point of reference which must retain its position after zoom.
        /// This point should be given in the ** window ** coordinates though (best to think as a point
        /// under mouse cursor, or center of the panel).
        /// </param>
        private void ChangeZoom(int zoom, Point anchor)
        {
            _isChangingZoom = true; // set the flag to prevent controls reupdating themselves infinitely

            float oldScale = _state.Scale;
            Point oldOffset = _state.Offset;

            // Update scale
            float newScale = zoom * ZOOM_STEP_VALUE * 0.01f;
            _state.Scale = newScale;
            UpdateScrollableWindowSize();

            // Update offset
            Point newOffset = RoomEditorState.RecalcOffset(oldOffset, oldScale, newScale, anchor);
            newOffset.X = Math.Min(bufferedPanel1.AutoScrollMinSize.Width - 1, newOffset.X);
            newOffset.Y = Math.Min(bufferedPanel1.AutoScrollMinSize.Height - 1, newOffset.Y);
            newOffset.X = Math.Max(0, newOffset.X);
            newOffset.Y = Math.Max(0, newOffset.Y);
            _state.Offset = newOffset;

            // Update scroll positions and redraw
            bufferedPanel1.HorizontalScroll.Value = MathExtra.Clamp(_state.Offset.X, bufferedPanel1.HorizontalScroll.Minimum, bufferedPanel1.HorizontalScroll.Maximum);
            bufferedPanel1.VerticalScroll.Value = MathExtra.Clamp(_state.Offset.Y, bufferedPanel1.VerticalScroll.Minimum, bufferedPanel1.VerticalScroll.Maximum);
            bufferedPanel1.Invalidate();
            // Update slider position (in case zoom was set by other controls)
            sldZoomLevel.Value = zoom;
            // Update label text
            lblZoomInfo.Text = String.Format("{0}%", zoom * ZOOM_STEP_VALUE);
            _isChangingZoom = false;
        }

        private void sldZoomLevel_Scroll(object sender, EventArgs e)
		{
            if (_isChangingZoom)
                return;
            // for the zoom slider we use center of panel as anchor
            ChangeZoom(sldZoomLevel.Value, new Point(bufferedPanel1.ClientSize.Width / 2, bufferedPanel1.ClientSize.Height / 2));
        }

		private void sldTransparency_Scroll(object sender, EventArgs e)
		{
			bufferedPanel1.Invalidate();
		}

		private void chkCharacterOffset_CheckedChanged(object sender, EventArgs e)
        {
            _state.DragFromCenter = chkCharacterOffset.Checked;
        }

        private void RoomSettingsEditor_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "room-editor");

            t.GroupBoxHelper(mainFrame, "room-editor/box");
            bufferedPanel1.BackColor = mainFrame.BackColor;
            bufferedPanel1.ForeColor = mainFrame.ForeColor;

            t.ButtonHelper(btnChangeImage, "room-editor/btn-change-image");
            t.ButtonHelper(btnDelete, "room-editor/btn-delete");
            t.ButtonHelper(btnExport, "room-editor/btn-export");

            t.SetColor("room-editor/combo-view-type/background", c => _editAddressBar.BackColor = c);
            t.SetColor("room-editor/combo-view-type/foreground", c => _editAddressBar.ForeColor = c);
            t.SetColor("room-editor/combo-view-type/drop-down/background", c => _editAddressBar.DropDownBackColor = c);
            t.SetColor("room-editor/combo-view-type/drop-down/foreground", c => _editAddressBar.DropDownForeColor = c);

            if (t.ComboBoxHelper(mainFrame.Controls, ref cmbBackgrounds, "room-editor/combo-backgrounds"))
            {
                cmbBackgrounds.SelectedIndexChanged += cmbBackgrounds_SelectedIndexChanged;
            }
        }
    }

    // TODO: refactor this to make code shared with the GUI Editor
    public class RoomEditorState
    {
        // Multiplier, defining convertion between room and editor coords.
        private float _scale;
        // Offsets, in window coordinates.
        private Point _scrollOffset;

        internal Cursor CurrentCursor;
        internal bool DragFromCenter;

        internal RoomEditorState()
        {
            Scale = 1f;
        }

        internal int WindowXToRoom(int x)
        {
            return (int)((x + _scrollOffset.X) / _scale);
        }

        internal int WindowYToRoom(int y)
        {
            return (int)((y + _scrollOffset.Y) / _scale);
        }

        internal int RoomXToWindow(int x)
        {
            return (int)(x * _scale) - _scrollOffset.X;
        }

        internal int RoomYToWindow(int y)
        {
            return (int)(y * _scale) - _scrollOffset.Y;
        }

        internal int RoomSizeToWindow(int sz)
        {
            return (int)(sz * _scale);
        }

        internal int WindowSizeToRoom(int sz)
        {
            return (int)(sz / _scale);
        }

        /// <summary>
        /// Scale of the Room image on screen.
        /// </summary>
        internal float Scale
        {
            get { return _scale; }
            set { _scale = value; }
        }

        /// <summary>
        /// Offset of the Room image on screen.
        /// </summary>
        internal Point Offset
        {
            get { return _scrollOffset; }
            set { _scrollOffset = value; }
        }

        /// <summary>
        /// Recalculates resulting image offsets, depending on the changing zoom and anchor point.
        /// </summary>
        // TODO: move this elsewhere, this kind of logic should perhaps take place in a dedicated control.
        // FreePanControl?
        /// <param name="curOffset"></param>
        /// <param name="oldScale"></param>
        /// <param name="newScale"></param>
        /// <param name="anchor">Anchor point in * window * coordinates.</param>
        /// <returns>Image offset in * window * coordinates</returns>
        internal static Point RecalcOffset(Point curOffset, float oldScale, float newScale, Point anchor)
        {
            // The idea here is that the anchor point defines the room location that must remain
            // * under same window location * after the rescaling.
            // In other words, the distance from window sides to this point must be same with old and new zoom.
            // Since the anchor is already given to us in window coordinates, its meaning is equal to distance from window's left-top;
            // this distance will remain the same, what will change is the distance in * room coordinates * (because room scale changes).
            
            Point roomAnchor = new Point((int)((anchor.X + curOffset.X) / oldScale), (int)((anchor.Y + curOffset.Y) / oldScale));
            Point newRoomDist = new Point((int)(anchor.X / newScale), (int)(anchor.Y / newScale));
            Point newRoomLT = new Point(roomAnchor.X - newRoomDist.X, roomAnchor.Y - newRoomDist.Y);
            Point newWindowLT = new Point((int)(newRoomLT.X * newScale), (int)(newRoomLT.Y * newScale));
            return newWindowLT;
        }
    }
}
