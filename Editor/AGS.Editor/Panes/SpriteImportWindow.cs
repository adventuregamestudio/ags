using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Text;
using System.Windows.Forms;
using AGS.Editor.Preferences;

namespace AGS.Editor
{
    public partial class SpriteImportWindow : Form
    {
		private readonly string[] TRANSPARENCY_MODE_DESCRIPTIONS = new string[]{
			"Pixels of index 0 will be transparent (256-colour games only)",
			"The top-left pixel will be the transparent colour for this sprite",
			"The bottom-left pixel will be the transparent colour for this sprite",
			"The top-right pixel will be the transparent colour for this sprite",
			"The bottom-right pixel will be the transparent colour for this sprite",
			"AGS will leave the sprite's pixels as they are. Any pixels that match the AGS Transparent Colour will be invisible.",
			"AGS will remove all transparent pixels by changing them to a very similar non-transparent colour"
		};
		private static bool _initialized = false;
		private static SpriteImportMethod _spriteImportMethod;

		public static SpriteImportMethod SpriteImportMethod
		{
			get 
			{
				if (!_initialized)
				{
					_initialized = true;
					_spriteImportMethod = Factory.AGSEditor.Settings.SpriteImportMethod;
				}
				return _spriteImportMethod; 
			}
			set { _spriteImportMethod = value; }
		}

		private static int _SelectionWidth = 0;
		private static int _SelectionHeight = 0;

        private Bitmap _image;
        private int _startDraggingX;
        private int _startDraggingY;
        private bool _draggingRectangle = false;
        private bool _doingTiledImport = false;
        private int _mouseX;
        private int _mouseY;
        private int _firstFrameX;
        private int _firstFrameY;
        private int _framesHorizontal;
        private int _framesVertical;
        private int _zoomLevel = 1;
        private List<Bitmap> _selectedBitmaps = new List<Bitmap>();
		private ToolTip _tooltip = new ToolTip();

        public List<Bitmap> SelectedBitmaps
        {
            get { return _selectedBitmaps; }
        }

        public bool RemapToGamePalette
        {
            get { return chkRemapCols.Checked; }
        }

        public bool UseBackgroundSlots
        {
            get { return chkRoomBackground.Checked; }
        }

        public SpriteImportWindow(Bitmap bmpToImport)
        {
            InitializeComponent();

            _image = bmpToImport;
            lblImageSize.Text = "Image size: " + _image.Width + " x " + _image.Height;
            cmbTransparentCol.SelectedIndex = (int)SpriteImportMethod;

			if ((_SelectionWidth > _image.Width) ||
				(_SelectionHeight > _image.Height))
			{
				_SelectionWidth = 0;
				_SelectionHeight = 0;
			}

            chkRoomBackground.Visible = (Factory.AGSEditor.CurrentGame.Settings.ColorDepth == AGS.Types.GameColorDepth.Palette);
            chkRemapCols.Visible = (Factory.AGSEditor.CurrentGame.Settings.ColorDepth == AGS.Types.GameColorDepth.Palette);

            // this label is a hack to get the scroll bars to stretch to the size we want
            Label scrollWindowSizer = new Label();
            scrollWindowSizer.Location = new Point(_image.Width, _image.Height);
            scrollWindowSizer.Text = string.Empty;
            scrollWindowSizer.Width = 0;
            scrollWindowSizer.Height = 0;
            previewPanel.Controls.Add(scrollWindowSizer);
        }

        private void zoomSlider_Scroll(object sender, EventArgs e)
        {
            int newVertical = (previewPanel.VerticalScroll.Value / _zoomLevel) * zoomSlider.Value;
            int newHorizontal = (previewPanel.HorizontalScroll.Value / _zoomLevel) * zoomSlider.Value;

            previewPanel.VerticalScroll.Value = Math.Min(newVertical, previewPanel.VerticalScroll.Maximum);
            previewPanel.HorizontalScroll.Value = Math.Min(newHorizontal, previewPanel.HorizontalScroll.Maximum);

            _zoomLevel = zoomSlider.Value;
            lblZoom.Text = "Zoom: x " + _zoomLevel;
            previewPanel.Controls[0].Location = new Point(_image.Width * _zoomLevel, _image.Height * _zoomLevel);
            previewPanel.Invalidate();
        }

        private void PaintTiledImportRectangles(Graphics g)
        {
            Pen pen = Pens.Blue;
            int framesHorizontal = ((_mouseX + previewPanel.HorizontalScroll.Value) / _zoomLevel - _firstFrameX) / _SelectionWidth + 1;
            int framesVertical = ((_mouseY + previewPanel.VerticalScroll.Value) / _zoomLevel - _firstFrameY) / _SelectionHeight + 1;
            if (framesHorizontal < 1)
            {
                framesHorizontal = 1;
            }
            if (framesVertical < 1)
            {
                framesVertical = 1;
            }
            if (_firstFrameX + framesHorizontal * _SelectionWidth > _image.Width)
            {
                framesHorizontal = (_image.Width - _firstFrameX) / _SelectionWidth;
            }
            if (_firstFrameY + framesVertical * _SelectionHeight > _image.Height)
            {
                framesVertical = (_image.Height - _firstFrameY) / _SelectionHeight;
            }
            _framesHorizontal = framesHorizontal;
            _framesVertical = framesVertical;

            int rectX = _firstFrameX - previewPanel.HorizontalScroll.Value / _zoomLevel;
            for (int x = 0; x < framesHorizontal; x++)
            {
                int rectY = _firstFrameY - previewPanel.VerticalScroll.Value / _zoomLevel;
                for (int y = 0; y < framesVertical; y++)
                {
                    g.DrawRectangle(pen, rectX * _zoomLevel, rectY * _zoomLevel, _SelectionWidth * _zoomLevel, _SelectionHeight * _zoomLevel);
                    rectY += _SelectionHeight;
                }
                rectX += _SelectionWidth;
            }
        }

        private void previewPanel_Paint(object sender, PaintEventArgs e)
        {
            // Disable anti-aliasing and associated bumpf
            e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
            e.Graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
            e.Graphics.CompositingQuality = CompositingQuality.HighSpeed;
            e.Graphics.DrawImage(_image, -previewPanel.HorizontalScroll.Value, -previewPanel.VerticalScroll.Value, _image.Width * _zoomLevel, _image.Height * _zoomLevel);

            if (_draggingRectangle)
            {
                int width = Math.Abs(_mouseX - _startDraggingX);
                int height = Math.Abs(_mouseY - _startDraggingY);
                int x = Math.Min(_mouseX, _startDraggingX);
                int y = Math.Min(_mouseY, _startDraggingY);

                Brush brush = new HatchBrush(HatchStyle.Percent50, Color.Yellow, Color.Transparent);
                e.Graphics.FillRectangle(brush, x, y, width, height);
            }
            else if ((_SelectionWidth > 0) && (_SelectionHeight > 0))
            {
                if (_doingTiledImport)
                {
                    PaintTiledImportRectangles(e.Graphics);
                }
                else
                {
                    Brush brush = new HatchBrush(HatchStyle.Percent50, Color.Yellow, Color.Transparent);
                    e.Graphics.FillRectangle(brush, _mouseX, _mouseY, _SelectionWidth * _zoomLevel, _SelectionHeight * _zoomLevel);
                }
            }
        }

		private void ConvertMousePositionToNearestPixel(MouseEventArgs e, ref int mouseX, ref int mouseY)
		{
			mouseX = (e.X + previewPanel.HorizontalScroll.Value) / _zoomLevel;
			mouseY = (e.Y + previewPanel.VerticalScroll.Value) / _zoomLevel;
			lblMousePos.Text = "Mouse location: " + mouseX + ", " + mouseY;
			mouseX = (mouseX * _zoomLevel) - previewPanel.HorizontalScroll.Value;
			mouseY = (mouseY * _zoomLevel) - previewPanel.VerticalScroll.Value;
		}

        private void previewPanel_MouseMove(object sender, MouseEventArgs e)
        {
			ConvertMousePositionToNearestPixel(e, ref _mouseX, ref _mouseY);

            if ((_draggingRectangle) || (_SelectionWidth > 0))
            {
                previewPanel.Invalidate();
            }
        }

        private void DoImport(Bitmap bmp)
        {
			if (VerifySpriteImportMethod())
			{
				SpriteImportMethod = (SpriteImportMethod)cmbTransparentCol.SelectedIndex;
				if (bmp != null)
				{
					_selectedBitmaps.Add(bmp);
				}
				this.DialogResult = DialogResult.OK;
				this.Close();
			}
        }

		private bool VerifySpriteImportMethod()
		{
			if ((Factory.AGSEditor.CurrentGame.Settings.ColorDepth != AGS.Types.GameColorDepth.Palette) &&
				(cmbTransparentCol.SelectedIndex == 0))
			{
				MessageBox.Show("You cannot use the 'Index 0' transparency with hi-colour games.", "Transparency selection", MessageBoxButtons.OK, MessageBoxIcon.Warning);
				return false;
			}
			return true;
		}

		private void btnImportWholeImage_Click(object sender, EventArgs e)
		{
			DoImport(_image);
		}

        private void previewPanel_MouseDown(object sender, MouseEventArgs e)
        {
            if ((e.Button == MouseButtons.Right) && (!_doingTiledImport))
            {
				ConvertMousePositionToNearestPixel(e, ref _startDraggingX, ref _startDraggingY);
                _draggingRectangle = true;
            }
            else if ((e.Button == MouseButtons.Left) && (_doingTiledImport))
            {
				for (int y = 0; y < _framesVertical; y++)
                {
					for (int x = 0; x < _framesHorizontal; x++)
					{
                        int xp = x * _SelectionWidth + _firstFrameX;
						int yp = y * _SelectionHeight + _firstFrameY;
						Bitmap subBitmap = _image.Clone(new Rectangle(xp, yp, _SelectionWidth, _SelectionHeight), _image.PixelFormat);
                        _selectedBitmaps.Add(subBitmap);
                    }
                }
                DoImport(null);
            }
            else if ((e.Button == MouseButtons.Left) && (!_draggingRectangle))
            {
				if ((_SelectionWidth < 1) || (_SelectionHeight < 1))
                {
                    MessageBox.Show("You must right-drag to set the selection rectangle size before importing.", "Import error", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else
                {
                    int x = (_mouseX + previewPanel.HorizontalScroll.Value) / _zoomLevel;
                    int y = (_mouseY + previewPanel.VerticalScroll.Value) / _zoomLevel;
                    if ((x < _image.Width) && (y < _image.Height))
                    {
                        if (chkTiled.Checked)
                        {
                            _doingTiledImport = true;
                            _firstFrameX = x;
                            _firstFrameY = y;
                            chkTiled.Enabled = false;
                            btnImportWholeImage.Enabled = false;
                            lblHelpText.Text = "Move the mouse to select the tiles that you want to import as sprites.";
                            previewPanel.Invalidate();
                        }
                        else
                        {
                            int width = _SelectionWidth;
							int height = _SelectionHeight;
                            if (x + _SelectionWidth > _image.Width)
                            {
                                width = _image.Width - x;
                            }
							if (y + _SelectionHeight > _image.Height)
                            {
                                height = _image.Height - y;
                            }
                            Bitmap subBitmap = _image.Clone(new Rectangle(x, y, width, height), _image.PixelFormat);
                            DoImport(subBitmap);
                        }
                    }
                }
            }
        }

        private void previewPanel_MouseUp(object sender, MouseEventArgs e)
        {
            if ((e.Button == MouseButtons.Right) && (!_doingTiledImport))
            {
                _draggingRectangle = false;
                _SelectionWidth = Math.Abs(_mouseX - _startDraggingX) / _zoomLevel;
				_SelectionHeight = Math.Abs(_mouseY - _startDraggingY) / _zoomLevel;

                Point cursorPos = Cursor.Position;
                if (_mouseX > _startDraggingX)
                {
                    cursorPos.X -= (_mouseX - _startDraggingX);
                }
                if (_mouseY > _startDraggingY)
                {
                    cursorPos.Y -= (_mouseY - _startDraggingY);
                }
                Cursor.Position = cursorPos;
            }
        }

        private void previewPanel_Scroll(object sender, ScrollEventArgs e)
        {
            previewPanel.Invalidate();
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

		private void cmbTransparentCol_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (_tooltip.Active)
			{
				_tooltip.Hide(this);
			}
			_tooltip.Show(TRANSPARENCY_MODE_DESCRIPTIONS[cmbTransparentCol.SelectedIndex], this, cmbTransparentCol.Left + 20, cmbTransparentCol.Top, 5000);
		}
    }
}