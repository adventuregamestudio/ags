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

namespace AGS.Editor
{
    public partial class RoomSettingsEditor : EditorContentPanel
    {
        private const int SCROLLBAR_WIDTH_BUFFER = 40;

        public delegate bool SaveRoomHandler(Room room);
        public event SaveRoomHandler SaveRoom;
        public delegate void AbandonChangesHandler(Room room);
        public event AbandonChangesHandler AbandonChanges;

        private Room _room;
        private IRoomEditorFilter _filter;
        private List<IRoomEditorFilter> _filters = new List<IRoomEditorFilter>();
        private RoomEditorState _state = new RoomEditorState();
        private bool _editorConstructed = false;
        private int _lastX, _lastY;
        private bool _mouseIsDown = false;

        public RoomSettingsEditor(Room room)
        {
			this.SetStyle(ControlStyles.Selectable, true);

            InitializeComponent();
            _room = room;
            if (_room.Resolution == RoomResolution.LowRes)
            {
                _state.ScaleFactor = 2;
            }
            else
            {
                _state.ScaleFactor = 1;
            }
            sldZoomLevel.Value = _state.ScaleFactor;
            _state.ScrollOffsetX = 0;
            _state.ScrollOffsetY = 0;
            _state.CurrentCursor = Cursors.Default;

            _filters.Add(new EmptyEditorFilter(bufferedPanel1, _room));
            _filters.Add(new EdgesEditorFilter(bufferedPanel1, _room));
            _filters.Add(new CharactersEditorFilter(bufferedPanel1, _room, Factory.AGSEditor.CurrentGame));
            _filters.Add(new ObjectsEditorFilter(bufferedPanel1, _room));
            _filters.Add(new HotspotsEditorFilter(bufferedPanel1, _room));
            _filters.Add(new WalkableAreasEditorFilter(bufferedPanel1, _room));
            _filters.Add(new WalkBehindsEditorFilter(bufferedPanel1, _room));
            _filters.Add(new RegionsEditorFilter(bufferedPanel1, _room));
            _filter = _filters[0];
            _filter.FilterOn();

            cmbViewType.Items.Add("Nothing");
            cmbViewType.Items.Add("Edges");
            cmbViewType.Items.Add("Characters");
            cmbViewType.Items.Add("Objects");
            cmbViewType.Items.Add("Hotspots");
            cmbViewType.Items.Add("Walkable areas");
            cmbViewType.Items.Add("Walk-behinds");
            cmbViewType.Items.Add("Regions");

            cmbViewType.SelectedIndex = 0;

            RepopulateBackgroundList(0);
            UpdateScrollableWindowSize();
			this.MouseWheel += new MouseEventHandler(RoomSettingsEditor_MouseWheel);
			this.bufferedPanel1.MouseWheel += new MouseEventHandler(RoomSettingsEditor_MouseWheel);
            this.sldZoomLevel.MouseWheel += new MouseEventHandler(RoomSettingsEditor_MouseWheel);

            _editorConstructed = true;
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

		private void UpdateScrollableWindowSize()
        {
            bufferedPanel1.AutoScrollMinSize = new Size(_room.Width * _state.ScaleFactor, _room.Height * _state.ScaleFactor);
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
            _state.ScrollOffsetX = -(bufferedPanel1.AutoScrollPosition.X / _state.ScaleFactor) * _state.ScaleFactor;
            _state.ScrollOffsetY = -(bufferedPanel1.AutoScrollPosition.Y / _state.ScaleFactor) * _state.ScaleFactor;

			int scaleFactor = _state.ScaleFactor;
            if ((_room.Resolution == RoomResolution.LowRes) &&
                (Factory.AGSEditor.CurrentGame.IsHighResolution))
            {
                // low-res room in high-res game, scale up since object
                // co-ordinates will be high-res
                scaleFactor *= 2;
            }

            int backgroundNumber = cmbBackgrounds.SelectedIndex;
            if (backgroundNumber < _room.BackgroundCount)
            {
				e.Graphics.SetClip(new Rectangle(0, 0, _room.Width * _state.ScaleFactor, _room.Height * _state.ScaleFactor));
                IntPtr hdc = e.Graphics.GetHdc();
				Factory.NativeProxy.CreateBuffer(bufferedPanel1.ClientSize.Width, bufferedPanel1.ClientSize.Height);
				// Adjust co-ordinates using original scale factor so that it lines
				// up with objects, etc
				int drawOffsX = -(_state.ScrollOffsetX / _state.ScaleFactor) * _state.ScaleFactor;
				int drawOffsY = -(_state.ScrollOffsetY / _state.ScaleFactor) * _state.ScaleFactor;
				lock (_room)
				{
					Factory.NativeProxy.DrawRoomBackground(hdc, _room, drawOffsX, drawOffsY, backgroundNumber, scaleFactor, _filter.MaskToDraw, _filter.SelectedArea, sldTransparency.Value);
				}
                _filter.PaintToHDC(hdc, _state);
                Factory.NativeProxy.RenderBufferToHDC(hdc);
                e.Graphics.ReleaseHdc(hdc);
                _filter.Paint(e.Graphics, _state);
            }
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
            string selectedFile = Factory.GUIController.ShowOpenFileDialog("Select background to import...", GUIController.IMAGE_FILE_FILTER);
            if (selectedFile != null)
            {
				Bitmap bmp = null;
                try
                {
                    bmp = new Bitmap(selectedFile);
                    bmp = ExtendBitmapIfSmallerThanScreen(bmp);
                    bool doImport = true;
                    bool deleteExtraFrames = false;
					RoomResolution newResolution;
					if (bgIndex > 0)
					{
						newResolution = _room.Resolution;
					}
                    // CHECKME: WTF is this??? How to deal with it?
					else if (//(bmp.Width > 640) && (bmp.Height > 400) &&
						(!Factory.AGSEditor.CurrentGame.Settings.LowResolution))
					{
						newResolution = RoomResolution.HighRes;
					}
					else
					{
						newResolution = RoomResolution.LowRes;
					}

					if ((bmp.Width != _room.Width) || (bmp.Height != _room.Height) ||
						(newResolution != _room.Resolution))
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
						_room.Resolution = newResolution;
                        _room.Width = bmp.Width;
                        _room.Height = bmp.Height;
                        Factory.NativeProxy.ImportBackground(_room, bgIndex, bmp, !Factory.AGSEditor.Preferences.RemapPalettizedBackgrounds, false);
                        _room.Modified = true;

                        if (deleteExtraFrames)
                        {
                            while (_room.BackgroundCount > 1)
                            {
                                Factory.NativeProxy.DeleteBackground(_room, 1);
                            }
                        }

						sldZoomLevel.Value = 3 - (int)_room.Resolution;
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
            string fileName = Factory.GUIController.ShowSaveFileDialog("Export background as...", GUIController.IMAGE_FILE_FILTER);
            if (fileName != null)
            {
                Bitmap bmp = Factory.NativeProxy.GetBitmapForBackground(_room, cmbBackgrounds.SelectedIndex);
                ImportExport.ExportBitmapToFile(fileName, bmp);
                bmp.Dispose();
            }
        }

        private void bufferedPanel1_MouseDown(object sender, MouseEventArgs e)
        {
            _filter.MouseDown(e, _state);
            _mouseIsDown = true;
		}

        private void bufferedPanel1_MouseUp(object sender, MouseEventArgs e)
        {
            if (_mouseIsDown)
            {
                _filter.MouseUp(e, _state);
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
			_filter.DoubleClick(_state);
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
            _state.CurrentCursor = Cursors.Default;

            int mouseXPos = ((e.X + _state.ScrollOffsetX) / _state.ScaleFactor);
            int mouseYPos = ((e.Y + _state.ScrollOffsetY) / _state.ScaleFactor);
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

            if (_filter.MouseMove(e.X, e.Y, _state))
            {
                bufferedPanel1.Invalidate();
            }
            bufferedPanel1.Cursor = _state.CurrentCursor;
        }

        private void cmbViewType_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (_editorConstructed)
            {
                _filter.FilterOff();
                _filter = _filters[cmbViewType.SelectedIndex];

                SetDefaultPropertyGridList();
                Factory.GUIController.SetPropertyGridObject(_room);
				lblTransparency.Visible = _filter.ShowTransparencySlider;
				sldTransparency.Visible = _filter.ShowTransparencySlider;
                chkCharacterOffset.Visible = ((string)cmbViewType.SelectedItem == "Characters");

                _filter.FilterOn();

                bufferedPanel1.Invalidate();
				// ensure that shortcut keys do not move the combobox
				bufferedPanel1.Focus();
            }
        }

        private void SetDefaultPropertyGridList()
        {
            Dictionary<string, object> defaultPropertyObjectList = new Dictionary<string, object>();
            defaultPropertyObjectList.Add(_room.PropertyGridTitle, _room);
            Factory.GUIController.SetPropertyGridObjectList(defaultPropertyObjectList);
        }

		protected override string OnGetHelpKeyword()
		{
			return _filter.HelpKeyword;
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
            if (_filter.KeyPressed(keyData))
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
            if ((canCancel) && (_room.Modified))
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
                        cancelClose = !SaveRoom(_room);
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

			if ((propertyName == RoomHotspot.PROPERTY_NAME_SCRIPT_NAME) ||
				(propertyName == RoomObject.PROPERTY_NAME_SCRIPT_NAME))
			{
				// Force the filter to refresh its property list with the new name
				_filter.FilterOff();
				_filter.FilterOn();
			}
		}

		protected override void OnWindowActivated()
		{
			SetFocusToAllowArrowKeysToWork();
			Factory.GUIController.ShowCuppit("This is the Room Editor. The first thing to do here is import a background using the 'Change' button. Various room properties can be changed in the property grid to the right.", "Room editor introduction");
		}

        protected override void OnCommandClick(string command)
        {
            _filter.CommandClick(command);
        }

        protected override void OnDispose()
        {
            _filter.FilterOff();
            foreach (IRoomEditorFilter filter in _filters)
            {
                filter.Dispose();
            }
        }

		private void sldZoomLevel_Scroll(object sender, EventArgs e)
		{
            int oldPosX = bufferedPanel1.HorizontalScroll.Value / _state.ScaleFactor;
            int oldPosY = bufferedPanel1.VerticalScroll.Value / _state.ScaleFactor;

            _state.ScaleFactor = sldZoomLevel.Value;
            UpdateScrollableWindowSize();

            bufferedPanel1.HorizontalScroll.Value = Math.Min(oldPosX * _state.ScaleFactor, bufferedPanel1.HorizontalScroll.Maximum);
            bufferedPanel1.VerticalScroll.Value = Math.Min(oldPosY * _state.ScaleFactor, bufferedPanel1.VerticalScroll.Maximum);
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
    }

    public class RoomEditorState
    {
        internal int ScaleFactor;
        internal int ScrollOffsetX;
        internal int ScrollOffsetY;
        internal Cursor CurrentCursor;
        internal bool DragFromCenter;
    }
}
