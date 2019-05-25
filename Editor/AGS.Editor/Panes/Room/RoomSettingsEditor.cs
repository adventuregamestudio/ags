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

namespace AGS.Editor
{
    public partial class RoomSettingsEditor : EditorContentPanel
    {
        private const int SCROLLBAR_WIDTH_BUFFER = 40;

        // NOTE: the reason we need to pass editor reference to the SaveRoom hander is that
        // currently design-time properties of room items are stored inside editor filter classes.
        public delegate bool SaveRoomHandler(Room room, RoomSettingsEditor editor);
        public event SaveRoomHandler SaveRoom;
        public delegate void AbandonChangesHandler(Room room);
        public event AbandonChangesHandler AbandonChanges;

        private Room _room;
        private IRoomEditorFilter _layer;
        private RoomEditNode _layersRoot;
        private List<IRoomEditorFilter> _layers = new List<IRoomEditorFilter>();
        private CharactersEditorFilter _characterLayer; // need to store the reference for special processing
        private bool _editorConstructed = false;
        private int _lastX, _lastY;
        private bool _mouseIsDown = false;

        private int ZOOM_STEP_VALUE = 25;
        private int ZOOM_MAX_VALUE = 600;
        private RoomEditorState _state = new RoomEditorState();

        /// <summary>
        /// Room editor item layers.
        /// </summary>
        public IEnumerable<IRoomEditorFilter> Layers { get { return _layers; } }

        /// <summary>
        /// Tells if the design-time properties of the room were modified since last save.
        /// </summary>
        public bool DesignModified
        {
            get
            {
                foreach (IRoomEditorFilter layer in _layers)
                    if (layer.Modified) return true;
                return false;
            }
            set
            { // Kind of ugly, I know...
                foreach (IRoomEditorFilter layer in _layers)
                    layer.Modified = value;
            }
        }


        public RoomSettingsEditor(Room room)
        {
			this.SetStyle(ControlStyles.Selectable, true);

            InitializeComponent();
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            _room = room;
            sldZoomLevel.Maximum = ZOOM_MAX_VALUE / ZOOM_STEP_VALUE;
            sldZoomLevel.Value = 100 / ZOOM_STEP_VALUE;
            // TODO: choose default zoom based on the room size vs window size?
            SetZoomSliderToMultiplier(_room.Width <= 320 ? 2 : 1);

            _layers.Add(new EdgesEditorFilter(bufferedPanel1, _room));
            _characterLayer = new CharactersEditorFilter(bufferedPanel1, _room, Factory.AGSEditor.CurrentGame);
            _layers.Add(_characterLayer);
            _layers.Add(new ObjectsEditorFilter(bufferedPanel1, _room));
            _layers.Add(new HotspotsEditorFilter(bufferedPanel1, _room));
            _layers.Add(new WalkableAreasEditorFilter(bufferedPanel1, _room));
            _layers.Add(new WalkBehindsEditorFilter(bufferedPanel1, _room));
            _layers.Add(new RegionsEditorFilter(bufferedPanel1, _room));

            foreach (IRoomEditorFilter layer in _layers)
            {
                layer.OnItemsChanged += layer_OnItemsChanged;
                layer.OnSelectedItemChanged += layer_OnSelectedItemChanged;
            }
            
            RefreshLayersTree();
            _editAddressBar.SelectionChange += editAddressBar_SelectionChange;

            RepopulateBackgroundList(0);
            UpdateScrollableWindowSize();
			this.MouseWheel += new MouseEventHandler(RoomSettingsEditor_MouseWheel);
			this.bufferedPanel1.MouseWheel += new MouseEventHandler(RoomSettingsEditor_MouseWheel);
            this.sldZoomLevel.MouseWheel += new MouseEventHandler(RoomSettingsEditor_MouseWheel);

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
            if (layer == null) return;
            IAddressNode node = _editAddressBar.RootNode.GetChild(GetLayerItemUniqueID(layer, e.Item), true);
            if (node == null) return;
            _editAddressBar.CurrentNode = node;
            SelectLayer(layer);
            //selecting hotspot from designer Panel, then cant Draw more hotspots...
        }

        private string GetLayerItemUniqueID(IRoomEditorFilter layer, string itemName)
        {
            return layer.DisplayName + itemName;
        }
                
        private void RoomSettingsEditor_MouseWheel(object sender, MouseEventArgs e)
		{
			int movement = e.Delta;
			if (movement > 0)
			{
				if (sldZoomLevel.Value < sldZoomLevel.Maximum)
				{
					sldZoomLevel.Value++;
				}
			}
			else
			{
				if (sldZoomLevel.Value > sldZoomLevel.Minimum)
				{
					sldZoomLevel.Value--;
				}
			}
			sldZoomLevel_Scroll(null, null);
            // Ridiculous solution, found on stackoverflow.com
            // TODO: check again later, how reliable this is?!
            HandledMouseEventArgs ee = (HandledMouseEventArgs)e;
            ee.Handled = true;
		}

        private void SetZoomSliderToMultiplier(int factor)
        {
            sldZoomLevel.Value = (100 * factor) / ZOOM_STEP_VALUE;
            sldZoomLevel_Scroll(null, null);
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
            _state.UpdateScroll(bufferedPanel1.AutoScrollPosition);

            int scaleFactor = 1;

            int backgroundNumber = cmbBackgrounds.SelectedIndex;
            if (backgroundNumber < _room.BackgroundCount)
            {
                e.Graphics.SetClip(new Rectangle(0, 0, _state.RoomSizeToWindow(_room.Width), _state.RoomSizeToWindow(_room.Height)));
                IntPtr hdc = e.Graphics.GetHdc();
                Factory.NativeProxy.CreateBuffer(bufferedPanel1.ClientSize.Width + SystemInformation.VerticalScrollBarWidth,
                    bufferedPanel1.ClientSize.Height + SystemInformation.HorizontalScrollBarHeight);
                // Adjust co-ordinates using original scale factor so that it lines
                // up with objects, etc
                int drawOffsX = _state.RoomXToWindow(0);
                int drawOffsY = _state.RoomYToWindow(0);
                IRoomEditorFilter maskFilter = GetCurrentMaskFilter();
				lock (_room)
				{
					Factory.NativeProxy.DrawRoomBackground(hdc, _room, drawOffsX, drawOffsY, backgroundNumber, _state.Scale * scaleFactor,
                        maskFilter == null ? RoomAreaMaskType.None : maskFilter.MaskToDraw, 
                        maskFilter == null ? 0 : maskFilter.SelectedArea, sldTransparency.Value);
				}
                foreach (IRoomEditorFilter layer in _layers)
                {                    
                    if (!IsVisible(layer)) continue;
                    layer.PaintToHDC(hdc, _state);
                }
                Factory.NativeProxy.RenderBufferToHDC(hdc);
                e.Graphics.ReleaseHdc(hdc);
                foreach (IRoomEditorFilter layer in _layers)
                {
                    if (!IsVisible(layer)) continue;
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
                    (Factory.GUIController.ShowQuestion("You are about to import an extra background into this room, thus creating animated backgrounds. Are you sure this is what you want to do?\n\nIf you just want to change the background image, use the 'Change' button instead.", MessageBoxIcon.Question) == DialogResult.Yes))
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
                        _room.Width = bmp.Width;
                        _room.Height = bmp.Height;
                        Factory.NativeProxy.ImportBackground(_room, bgIndex, bmp, !Factory.AGSEditor.Settings.RemapPalettizedBackgrounds, false);
                        _room.Modified = true;

                        if (deleteExtraFrames)
                        {
                            while (_room.BackgroundCount > 1)
                            {
                                Factory.NativeProxy.DeleteBackground(_room, 1);
                            }
                        }

                        // TODO: choose default zoom based on the room size vs window size?
                        SetZoomSliderToMultiplier(_room.Width <= 320 ? 2 : 1);
						sldZoomLevel_Scroll(null, null);
						UpdateScrollableWindowSize();
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
				lock (_room)
				{
					Factory.NativeProxy.DeleteBackground(_room, cmbBackgrounds.SelectedIndex);
					RepopulateBackgroundList(0);
				}
                _room.Modified = true;
            }
        }

        private void btnExport_Click(object sender, EventArgs e)
        {
            string fileName = Factory.GUIController.ShowSaveFileDialog("Export background as...", Constants.IMAGE_FILE_FILTER);
            if (fileName != null)
            {
                Bitmap bmp = Factory.NativeProxy.GetBitmapForBackground(_room, cmbBackgrounds.SelectedIndex);
                ImportExport.ExportBitmapToFile(fileName, bmp);
                bmp.Dispose();
            }
        }

        private void bufferedPanel1_MouseDown(object sender, MouseEventArgs e)
        {
            foreach (IRoomEditorFilter layer in _layers)
            {
                if (IsLocked(layer)) continue;
                layer.MouseDownAlways(e, _state);
            }
            foreach (IRoomEditorFilter layer in _layers)
            {
                if (IsLocked(layer)) continue;
                if (layer.MouseDown(e, _state)) break;
            }
            _mouseIsDown = true;
		}

        private void bufferedPanel1_MouseUp(object sender, MouseEventArgs e)
        {
            if (_mouseIsDown)
            {
                foreach (IRoomEditorFilter layer in _layers)
                {
                    if (IsLocked(layer)) continue;
                    if (layer.MouseUp(e, _state)) break;
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
            foreach (IRoomEditorFilter layer in _layers)
            {
                if (IsLocked(layer)) continue;
                if (layer.DoubleClick(_state)) break;
            }
			Factory.GUIController.RefreshPropertyGrid();
			bufferedPanel1.Invalidate();
		}

		private void bufferedPanel1_MouseMove(object sender, MouseEventArgs e)
        {
            if ((e.X == _lastX) && (e.Y == _lastY))
            {
                return;
            }

            _lastX = e.X;
            _lastY = e.Y;            

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
            lblMousePos.Text = "Mouse Position: " + xPosText + ", " + yPosText;

            SelectCursor(e.X, e.Y, _state);
            foreach (IRoomEditorFilter layer in _layers)
            {
                if (IsLocked(layer)) continue;
                if (layer.MouseMove(e.X, e.Y, _state))
                {
                    bufferedPanel1.Invalidate();                    
                    break;
                }
            }            
        }

        private void SelectCursor(int x, int y, RoomEditorState state)
        {
            state.CurrentCursor = Cursors.Default;            
            if (_layer != null) state.CurrentCursor = _layer.GetCursor(x, y, state);
            else state.CurrentCursor = null;   
            if (state.CurrentCursor != null)
            {
                bufferedPanel1.Cursor = state.CurrentCursor;
                return;
            }            
            for (int layerIndex = _layers.Count - 1; layerIndex >= 0; layerIndex--)
            {
                IRoomEditorFilter layer = _layers[layerIndex];
                if (IsLocked(layer)) continue;
                Cursor tmpCursor = layer.GetCursor(x, y, state);
                if (tmpCursor != null) state.CurrentCursor = tmpCursor;
            }            
            bufferedPanel1.Cursor = state.CurrentCursor ?? Cursors.Default;
        }

        private void editAddressBar_SelectionChange(object sender, NodeChangedArgs e)
        {
            if (_layersRoot.UniqueID.Equals(e.OUniqueID))
            {
                Factory.GUIController.SetPropertyGridObject(_room);
                return;
            }

            RoomEditNode node = _layersRoot.GetChild(e.OUniqueID, true) as RoomEditNode;
            if (node == null) return;
            RoomEditNode layerNode = node;
            while (layerNode != null && layerNode.Layer == null)
            {
                layerNode = layerNode.Parent as RoomEditNode;
            }
            if (layerNode == null) return;

            layerNode.IsVisible = true;
            SelectLayer(layerNode.Layer);

            // only select the item if the room editor is the active tab; this could
            // be a refresh from another tab e.g. changing a character's starting room
            if (Factory.GUIController.ActivePane.Control == this)
            {
                layerNode.Layer.SelectItem(node == layerNode ? null : node.RoomItemID);
            }
        }

        private void SelectLayer(IRoomEditorFilter layer)
        {
            if (!_editorConstructed) return;
            if (layer == _layer) return;

            if (_layer != null) _layer.FilterOff();
            _layer = layer;

            SetDefaultPropertyGridList();
            Factory.GUIController.SetPropertyGridObject(_room);
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
            Dictionary<string, object> defaultPropertyObjectList = new Dictionary<string, object>();
            defaultPropertyObjectList.Add(_room.PropertyGridTitle, _room);
            Factory.GUIController.SetPropertyGridObjectList(defaultPropertyObjectList);
        }

		protected override string OnGetHelpKeyword()
		{
            if (_layer == null) return "";
			return _layer.HelpKeyword;
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

		private bool ProcessZoomAndPanKeyPresses(Keys keyData)
		{
			if (keyData == Keys.Down)
			{
				bufferedPanel1.AutoScrollPosition = new Point(Math.Abs(bufferedPanel1.AutoScrollPosition.X), Math.Abs(bufferedPanel1.AutoScrollPosition.Y) + 50);
			}
			else if (keyData == Keys.Up)
			{
				bufferedPanel1.AutoScrollPosition = new Point(Math.Abs(bufferedPanel1.AutoScrollPosition.X), Math.Abs(bufferedPanel1.AutoScrollPosition.Y) - 50);
			}
			else if (keyData == Keys.Right)
			{
				bufferedPanel1.AutoScrollPosition = new Point(Math.Abs(bufferedPanel1.AutoScrollPosition.X) + 50, Math.Abs(bufferedPanel1.AutoScrollPosition.Y));
			}
			else if (keyData == Keys.Left)
			{
				bufferedPanel1.AutoScrollPosition = new Point(Math.Abs(bufferedPanel1.AutoScrollPosition.X) - 50, Math.Abs(bufferedPanel1.AutoScrollPosition.Y));
			}
			else if (keyData == Keys.Space)
			{
				if (sldZoomLevel.Value < sldZoomLevel.Maximum)
				{
					sldZoomLevel.Value++;
				}
				else
				{
					sldZoomLevel.Value = sldZoomLevel.Minimum;
				}
				sldZoomLevel_Scroll(null, null);
			}
			else
			{
				return false;
			}

			return true;
		}

		protected override bool HandleKeyPress(Keys keyData)
		{
			bool returnHandled = true;

            if (!DoesThisPanelHaveFocus())
            {
                return false;
            }
            if (_layer != null && _layer.KeyPressed(keyData))
            {
                bufferedPanel1.Invalidate();
                Factory.GUIController.RefreshPropertyGrid();
            }
            else if (!ProcessZoomAndPanKeyPresses(keyData))
            {
                returnHandled = false;
            }			
			return returnHandled;
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

        private void sldZoomLevel_Scroll(object sender, EventArgs e)
		{
            lblZoomInfo.Text = String.Format("{0}%", sldZoomLevel.Value * ZOOM_STEP_VALUE);

            int oldPosX = _state.WindowSizeToRoom(bufferedPanel1.HorizontalScroll.Value);
            int oldPosY = _state.WindowSizeToRoom(bufferedPanel1.VerticalScroll.Value);

            _state.Scale = sldZoomLevel.Value * ZOOM_STEP_VALUE * 0.01f;
            UpdateScrollableWindowSize();
            
            bufferedPanel1.HorizontalScroll.Value = _state.RoomSizeToWindow(oldPosX);
            bufferedPanel1.VerticalScroll.Value = _state.RoomSizeToWindow(oldPosY);
            bufferedPanel1.Invalidate();
		}

		private void sldTransparency_Scroll(object sender, EventArgs e)
		{
			bufferedPanel1.Invalidate();
		}

		private void chkCharacterOffset_CheckedChanged(object sender, EventArgs e)
        {
            _state.DragFromCenter = chkCharacterOffset.Checked;
        }

        private void LoadColorTheme(ColorTheme t)
        {
            BackColor = t.GetColor("room-editor/background");
            ForeColor = t.GetColor("room-editor/foreground");
            mainFrame.BackColor = t.GetColor("room-editor/box/background");
            mainFrame.ForeColor = t.GetColor("room-editor/box/foreground");
            bufferedPanel1.BackColor = mainFrame.BackColor;
            bufferedPanel1.ForeColor = mainFrame.ForeColor;
            btnChangeImage.BackColor = t.GetColor("room-editor/btn-change-image/background");
            btnChangeImage.ForeColor = t.GetColor("room-editor/btn-change-image/foreground");
            btnChangeImage.FlatStyle = (FlatStyle)t.GetInt("room-editor/btn-change-image/flat/style");
            btnChangeImage.FlatAppearance.BorderSize = t.GetInt("room-editor/btn-change-image/flat/border/size");
            btnChangeImage.FlatAppearance.BorderColor = t.GetColor("room-editor/btn-change-image/flat/border/color");
            btnDelete.BackColor = t.GetColor("room-editor/btn-delete/background");
            btnDelete.ForeColor = t.GetColor("room-editor/btn-delete/foreground");
            btnDelete.FlatStyle = (FlatStyle)t.GetInt("room-editor/btn-delete/flat/style");
            btnDelete.FlatAppearance.BorderSize = t.GetInt("room-editor/btn-delete/flat/border/size");
            btnDelete.FlatAppearance.BorderColor = t.GetColor("room-editor/btn-delete/flat/border/color");
            btnExport.BackColor = t.GetColor("room-editor/btn-export/background");
            btnExport.ForeColor = t.GetColor("room-editor/btn-export/foreground");
            btnExport.FlatStyle = (FlatStyle)t.GetInt("room-editor/btn-export/flat/style");
            btnExport.FlatAppearance.BorderSize = t.GetInt("room-editor/btn-export/flat/border/size");
            btnExport.FlatAppearance.BorderColor = t.GetColor("room-editor/btn-export/flat/border/color");
            _editAddressBar.BackColor = t.GetColor("room-editor/combo-view-type/background");
            _editAddressBar.ForeColor = t.GetColor("room-editor/combo-view-type/foreground");
            _editAddressBar.DropDownBackColor = t.GetColor("room-editor/combo-view-type/drop-down/background");
            _editAddressBar.DropDownForeColor = t.GetColor("room-editor/combo-view-type/drop-down/foreground");
            mainFrame.Controls.Remove(cmbBackgrounds);
            cmbBackgrounds= t.GetComboBox("room-editor/combo-backgrounds", cmbBackgrounds);
            cmbBackgrounds.SelectedIndexChanged += cmbBackgrounds_SelectedIndexChanged;
            mainFrame.Controls.Add(cmbBackgrounds);
        }
    }

    // TODO: refactor this to make code shared with the GUI Editor
    public class RoomEditorState
    {
        // Multiplier, defining convertion between room and editor coords.
        private float _scale;
        // Offsets, in window coordinates.
        private int _scrollOffsetX;
        private int _scrollOffsetY;

        internal Cursor CurrentCursor;
        internal bool DragFromCenter;

        internal RoomEditorState()
        {
            Scale = 1f;
        }

        internal int WindowXToRoom(int x)
        {
            return (int)((x + _scrollOffsetX) / _scale);
        }

        internal int WindowYToRoom(int y)
        {
            return (int)((y + _scrollOffsetY) / _scale);
        }

        internal int RoomXToWindow(int x)
        {
            return (int)(x * _scale) - _scrollOffsetX;
        }

        internal int RoomYToWindow(int y)
        {
            return (int)(y * _scale) - _scrollOffsetY;
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
            set
            {
                float oldScale = _scale;
                _scale = value;
                _scrollOffsetX = (int)((_scrollOffsetX / oldScale) * _scale);
                _scrollOffsetY = (int)((_scrollOffsetY / oldScale) * _scale);
            }
        }

        /// <summary>
        /// Updates offset using current scrollbar position.
        /// </summary>
        /// <param name="scrollPt">Scroll position in window coordinates.</param>
        internal void UpdateScroll(Point scrollPt)
        {
            _scrollOffsetX = -scrollPt.X;
            _scrollOffsetY = -scrollPt.Y;
        }
    }
}
