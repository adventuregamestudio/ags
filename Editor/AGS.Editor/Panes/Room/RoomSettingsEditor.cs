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
        private RoomLayerTree _layerTree;
        private IRoomEditorFilter _emptyLayer;
        private List<IRoomEditorFilter> _layers = new List<IRoomEditorFilter>();
        private CharactersEditorFilter _characterLayer; // need to store the reference for special processing
        private bool _editorConstructed = false;
        private bool _mouseIsDown = false;
        private int _menuClickX;
        private int _menuClickY;
        private string _startNodePath; // track the selection so that it can be compared when saving

        private const int ZOOM_STEP_VALUE = 25;
        private const int ZOOM_MAX_VALUE = 600;
        private bool _isChangingZoom = false;
        private RoomEditorState _state = new RoomEditorState();

        /// <summary>
        /// Room editor item layers.
        /// </summary>
        public IEnumerable<IRoomEditorFilter> Layers { get { return _layers; } }

        /// <summary>
        /// The selected node's path, as saved in the room's user file.
        /// </summary>
        public string SelectedNodePath
        {
            get { return _layerTree.GetPath(_layerTreeView.SelectedLayerNode ?? _layerTree.Root); }
        }

        public bool IsLayerVisibleByUser(IRoomEditorFilter layer)
        {
            return _layerTree.IsVisibleByUser(layer);
        }

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

                if (SelectedNodePath != _startNodePath)
                    return true;

                return false;
            }
            set
            { // Kind of ugly, I know...
                foreach (IRoomEditorFilter layer in _layers)
                    layer.Modified = value;

                _startNodePath = SelectedNodePath;
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

            _layerTree = new RoomLayerTree(_layers);
            _layerTreeView.Tree = _layerTree;
            _layerTreeView.NodeSelected += layerTreeView_NodeSelected;
            _layerTreeView.NodeToggled += layerTreeView_NodeToggled;
            _layerTreeView.LeaveRequested += layerTreeView_LeaveRequested;
            _startNodePath = SelectedNodePath;

            RepopulateBackgroundList(0);
            UpdateScrollableWindowSize();
            sldZoomLevel.Maximum = ZOOM_MAX_VALUE / ZOOM_STEP_VALUE;
            sldZoomLevel.Value = 100 / ZOOM_STEP_VALUE;
            // TODO: choose default zoom based on the room size vs window size?
            SetZoomSliderToMultiplier(_room.Width <= 320 ? 2 : 1);

            MouseWheel += RoomSettingsEditor_MouseWheel;
            bufferedPanel1.MouseWheel += RoomSettingsEditor_MouseWheel;
            sldZoomLevel.MouseWheel += RoomSettingsEditor_MouseWheel;
            cmbBackgrounds.MouseWheel += RoomSettingsEditor_MouseWheel;
            bufferedPanel1.PanButtons = MouseButtons.Middle;

            _editorConstructed = true;
        }

        /// <summary>
        /// Rebuilds the layer tree, keeping the selection; if its item is gone, its layer is selected.
        /// </summary>
        public void RefreshLayersTree()
        {
            string oldKey = _layerTreeView.SelectedLayerNode?.Key;
            _layerTree.Rebuild();
            RoomLayerNode selected = _layerTreeView.Reload();
            if (selected != null && selected.Key != oldKey)
                ApplyNodeSelection(selected, false);
        }

        /// <summary>
        /// Recounts the drawn areas after the masks were replaced outside the area layers.
        /// </summary>
        public void MasksChanged()
        {
            foreach (BaseAreasEditorFilter layer in _layers.OfType<BaseAreasEditorFilter>())
                layer.ForgetDrawnAreas();
            RefreshLayersTree();
        }

        public bool TrySelectNodeByPath(string path)
        {
            RoomLayerNode node = _layerTree.FindByPath(path);
            if (node == null)
                return false;
            SelectNode(node, false);
            return true;
        }

        /// <summary>
        /// Invalidate the surface of the drawing control and causes the control to be redrawn
        /// </summary>
        public void InvalidateDrawingBuffer() => bufferedPanel1.Invalidate();
        
        private void SelectNode(RoomLayerNode node, bool focusCanvas)
        {
            if (node.Key == _layerTreeView.SelectedLayerNode?.Key)
                return;
            _layerTreeView.SelectKey(node.Key);
            ApplyNodeSelection(node, focusCanvas);
        }

        private void ApplyNodeSelection(RoomLayerNode node, bool focusCanvas)
        {
            // A hidden layer is shown while selected, so that it can be edited
            if (_layerTree.ShowForSelection(node))
            {
                _layerTreeView.InvalidateToggles();
                bufferedPanel1.Invalidate();
            }
            if (node.Layer == null)
            {
                SetPropertyGridObject(_room);
                SelectLayer(null, focusCanvas);
                return;
            }
            IRoomEditorFilter layer = (IRoomEditorFilter)node.Layer;
            SelectLayer(layer, focusCanvas);
            layer.SelectItem(node.Kind == RoomLayerNodeKind.Item ? node.ItemID : null);
        }

        private void layerTreeView_NodeSelected(object sender, RoomLayerNodeEventArgs e)
        {
            ApplyNodeSelection(e.Node, false);
        }

        private void layerTreeView_NodeToggled(object sender, RoomLayerNodeEventArgs e)
        {
            bufferedPanel1.Invalidate(); // visibility changed, or the lock cursor
        }

        private void layerTreeView_LeaveRequested(object sender, EventArgs e)
        {
            SetFocusToAllowArrowKeysToWork();
        }

        private void layer_OnItemsChanged(object sender, EventArgs e)
        {
            RefreshLayersTree();
        }

        private void layer_OnSelectedItemChanged(object sender, SelectedRoomItemEventArgs e)
        {
            IRoomEditorFilter layer = sender as IRoomEditorFilter;
            if (layer == null) { SelectLayer(null, true); return; }
            RoomLayerNode node = _layerTree.FindItem(layer, e.ItemID);
            if (node == null) { SelectLayer(null, true); return; }
            SelectNode(node, true);
            SelectLayer(layer, true);
            //selecting hotspot from designer Panel, then cant Draw more hotspots...
        }

        /// <summary>
        /// Room layer wants to display context menu, we shall add our items too.
        /// </summary>
        private void layer_OnContextMenu(object sender, RoomFilterContextMenuArgs e)
        {
            if (e.Menu.Items.Count > 0)
                e.Menu.Items.Add(new ToolStripSeparator());
            PrepareContextMenu(e.Menu, e.X, e.Y);
        }

        public bool TrySelectObjectByName(string name)
        {
            if (string.IsNullOrEmpty(name) || name == "Room")
            {
                SetPropertyGridObject(_room);
                return true;
            }

            foreach (var layer in _layers)
            {
                if (layer.TrySelectItemByName(name))
                    return true;
            }
            return false;
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
                e.Graphics.Clear(bufferedPanel1.BackColor);
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
                // TODO: adjust ExtendCanvasSizeOf8BitBitmap to support 1-bit and 4-bit bitmaps
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
        /// TODO: adjust this to support 1-bit, 4-bit formats too?
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
                    bmp = BitmapExtensions.LoadNonLockedBitmap(selectedFile);
                    bmp = ExtendBitmapIfSmallerThanScreen(bmp);
                    bool doImport = true;
                    bool deleteExtraFrames = false;
                    bool roomSizeChanged = false;
                    AdjustMaskOptions maskOption = AdjustMaskOptions.ResetMask;
                    int maskXOffset = 0;
                    int maskYOffset = 0;

                    if ((Factory.AGSEditor.CurrentGame.Settings.ColorDepth == GameColorDepth.Palette) && (bmp.GetColorDepth() > 8))
                    {
                        Factory.GUIController.ShowMessage("You cannot import a hi-colour or true-colour image into a 256-colour game.", MessageBoxIcon.Warning);
                        doImport = false;
                    }
                    else if ((bmp.Width != _room.Width) || (bmp.Height != _room.Height))
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
                            using (AdjustMasksDialog dialog = new AdjustMasksDialog())
                            {
                                if (dialog.ShowDialog(this) == DialogResult.OK)
                                {
                                    maskOption = dialog.MaskOption;
                                    maskXOffset = dialog.XOffset;
                                    maskYOffset = dialog.YOffset;
                                    roomSizeChanged = true;
                                }
                                else
                                {
                                    doImport = false;
                                }
                            }
                        }
                    }

                    if (doImport)
                    {
                        Size oldRoomSize = new Size(_room.Width, _room.Height);
                        _roomController.SetBackground(bgIndex, bmp);

                        if (deleteExtraFrames)
                        {
                            while (_room.BackgroundCount > 1)
                            {
                                _roomController.DeleteBackground(1);
                            }
                            RepopulateBackgroundList(0);
                        }

                        // If size or resolution has changed, reset or resize the masks and call OnRoomSizeChanged
                        if (roomSizeChanged)
                        {
                            if (maskOption == AdjustMaskOptions.ResetMask)
                            {
                                _roomController.ResetMasks();
                            }
                            else
                            {
                                bool doScale = (maskOption == AdjustMaskOptions.ScaleMaskImage);
                                _roomController.ResizeMasks(doScale, bmp.Width, bmp.Height, maskXOffset, maskYOffset);
                            }

                            OnRoomSizeChanged(oldRoomSize, maskOption);
                            MasksChanged();
                        }

                        // TODO: choose default zoom based on the room size vs window size?
                        SetZoomSliderToMultiplier(_room.Width <= 320 ? 2 : 1);
                    }
                }
                catch (Exception ex)
                {
                    Factory.GUIController.ShowError("The background could not be imported.", ex, MessageBoxIcon.Warning);
                }
                finally
                {
                    if (bmp != null)
                        bmp.Dispose();
                }
            }
        }

        private void OnRoomSizeChanged(Size oldRoomSize, AdjustMaskOptions maskOption)
        {
            switch (maskOption)
            {
                case AdjustMaskOptions.ResetMask:
                    // Reset room edges to the defaults at the new size
                    _room.LeftEdgeX = 0;
                    _room.RightEdgeX = _room.Width - 1;
                    _room.TopEdgeY = 0;
                    _room.BottomEdgeY = _room.Height - 1;
                    break;
                case AdjustMaskOptions.ScaleMaskImage:
                    float hfactor = (float)_room.Width / oldRoomSize.Width;
                    float vfactor = (float)_room.Height / oldRoomSize.Height;
                    _room.LeftEdgeX = (int)Math.Round(_room.LeftEdgeX * hfactor);
                    _room.RightEdgeX = (int)Math.Round(_room.RightEdgeX * hfactor);
                    _room.TopEdgeY = (int)Math.Round(_room.TopEdgeY * vfactor);
                    _room.BottomEdgeY = (int)Math.Round(_room.BottomEdgeY * vfactor);
                    break;
                default:
                    // Keep edges
                    break;
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

        /// <summary>
        /// Turns a layer on for editing (null: none). A selection made in the layer tree
        /// keeps the focus there, for its arrow keys.
        /// </summary>
        private void SelectLayer(IRoomEditorFilter layer, bool focusCanvas)
        {
            if (!_editorConstructed) return;
            if (layer == _layer) return;

            if (_layer != null) _layer.FilterOff();
            _layer = layer ?? _emptyLayer;

            SetDefaultPropertyGridList();
            SetPropertyGridObject(_room);
            lblTransparency.Visible = _layer.ShowTransparencySlider;
            sldTransparency.Visible = _layer.ShowTransparencySlider;
            chkObjectOffset.Visible = _layer is CharactersEditorFilter || _layer is ObjectsEditorFilter;

            if (chkObjectOffset.Visible)
                _state.DragFromAnchor = chkObjectOffset.Checked;
            else
                _state.DragFromAnchor = false;

            _layer.FilterOn();

            bufferedPanel1.Invalidate();
            // ensure that shortcut keys do not move the combobox
            if (focusCanvas)
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

        /// <summary>
        /// Tells whether a key belongs to the focused layer tree, not to the room.
        /// </summary>
        private bool IsLayerTreeKey(Keys keyData)
        {
            if (!_layerTreeView.ContainsFocus || (keyData & (Keys.Control | Keys.Alt)) != 0)
                return false;
            switch (keyData & Keys.KeyCode)
            {
                case Keys.Up:
                case Keys.Down:
                case Keys.Left:
                case Keys.Right:
                case Keys.Home:
                case Keys.End:
                case Keys.PageUp:
                case Keys.PageDown:
                case Keys.Add:
                case Keys.Subtract:
                case Keys.Multiply:
                case Keys.Enter:
                    return true;
                default:
                    return false;
            }
        }

		protected override bool HandleKeyPress(Keys keyData)
		{
            if (!DoesThisPanelHaveFocus() || IsLayerTreeKey(keyData))
            {
                return false;
            }

            if (_layer != null && !IsLocked(_layer) && _layer.KeyPressed(keyData, _state))
            {
                bufferedPanel1.Invalidate();
                Factory.GUIController.RefreshPropertyGrid();
                return true;
            }
            return ProcessPanKeyPress(keyData);
		}

        protected override bool HandleKeyRelease(Keys keyData)
        {
            if (!DoesThisPanelHaveFocus() || IsLayerTreeKey(keyData))
            {
                return false;
            }

            if (_layer != null && !IsLocked(_layer) && _layer.KeyReleased(keyData, _state))
            {
                return true;
            }
            else if (keyData == Keys.Apps || ((keyData & ~Keys.Modifiers) == Keys.F10 && (keyData & Keys.Modifiers) == Keys.Shift))
            {
                var showMenuAt = bufferedPanel1.PointToClient(MousePosition);
                if (bufferedPanel1.ClientRectangle.Contains(showMenuAt))
                {
                    ShowContextMenu(showMenuAt.X, showMenuAt.Y);
                }
                return true;
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

            if (needRefresh ||
                propertyName == RoomHotspot.PROPERTY_NAME_SCRIPT_NAME ||
                propertyName == RoomHotspot.PROPERTY_NAME_DESCRIPTION ||
                propertyName == RoomObject.PROPERTY_NAME_SCRIPT_NAME ||
                propertyName == RoomObject.PROPERTY_NAME_DESCRIPTION ||
                propertyName == Character.PROPERTY_NAME_SCRIPTNAME ||
                propertyName == Character.PROPERTY_NAME_DESCRIPTION ||
                propertyName == RoomRegion.PROPERTY_NAME_SCRIPT_NAME ||
                propertyName == RoomWalkableArea.PROPERTY_NAME_SCRIPT_NAME ||
                propertyName == RoomWalkBehind.PROPERTY_NAME_SCRIPT_NAME)
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
            else if (_layer is BaseAreasEditorFilter)
            {
                // Any property of an area can make it used or unused
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

        private void chkObjectOffset_CheckedChanged(object sender, EventArgs e)
        {
            _state.DragFromAnchor = chkObjectOffset.Checked;
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
            t.SetColor(new string[] { "room-editor/buffered-panel/background", "room-editor/box/background", "room-editor/background" }, c => bufferedPanel1.BackColor = c);
            t.SetColor(new string[] { "room-editor/buffered-panel/foreground", "room-editor/box/foreground", "room-editor/foreground" }, c => bufferedPanel1.ForeColor = c);

            t.ButtonHelper(btnChangeImage, "room-editor/btn-change-image");
            t.ButtonHelper(btnDelete, "room-editor/btn-delete");
            t.ButtonHelper(btnExport, "room-editor/btn-export");

            // Older themes have colours for the breadcrumb bar's drop-down
            t.SetColor(new string[] { "room-editor/tree/background", "room-editor/combo-view-type/drop-down/background",
                "sprite-selector/tree/background" }, c => _layerTreeView.BackColor = c);
            t.SetColor(new string[] { "room-editor/tree/foreground", "room-editor/combo-view-type/drop-down/foreground",
                "sprite-selector/tree/foreground" }, c => _layerTreeView.ForeColor = c);
            t.SetColor(new string[] { "room-editor/tree/line", "sprite-selector/tree/line" }, c => _layerTreeView.LineColor = c);

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
        internal bool DragFromAnchor;

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

        internal Rectangle RoomRectangleToWindow(Rectangle rect)
        {
            return new Rectangle(RoomXToWindow(rect.Left), RoomYToWindow(rect.Top), RoomSizeToWindow(rect.Width), RoomSizeToWindow(rect.Height));
        }

        internal Rectangle WindowRectangleToRoom(Rectangle rect)
        {
            return new Rectangle(WindowXToRoom(rect.Left), WindowYToRoom(rect.Top), WindowSizeToRoom(rect.Width), WindowSizeToRoom(rect.Height));
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
