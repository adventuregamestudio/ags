using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;
using System.IO;
using AGS.Editor.Utils;
using AGS.Types;

namespace AGS.Editor
{
    public partial class SpriteImportWindow : Form
    {
        private Bitmap image;
        private List<string> imageLookup;
        private int zoomLevel = 1;

        // for dragging a rectangle
        private bool dragging = false;
        private Point start;
        private Point position;

        public bool RemapToGamePalette
        {
            get { return chkRemapCols.Checked; }
            set { chkRemapCols.Checked = value; }
        }

        public bool UseBackgroundSlots
        {
            get { return chkRoomBackground.Checked; }
            set { chkRoomBackground.Checked = value; }
        }

        public bool UseAlphaChannel
        {
            get { return chkUseAlphaChannel.Checked; }
            set { chkUseAlphaChannel.Checked = value; }
        }

        public bool TiledImport
        {
            get { return chkTiled.Checked; }
            set { chkTiled.Checked = value; }
        }

        public Size ImageSize
        {
            get { return new Size(image.Width, image.Height); }
        }

        public Point SelectionOffset
        {
            get { return new Point((int)numOffsetX.Value, (int)numOffsetY.Value); }
            set { numOffsetX.Value = value.X; numOffsetY.Value = value.Y; }
        }

        public Size SelectionSize
        {
            get { return new Size((int)numSizeX.Value, (int)numSizeY.Value); }
            set { numSizeX.Value = value.Width; numSizeY.Value = value.Height; }
        }

        public Size TilingMargin
        {
            get { return new Size((int)numMarginX.Value, (int)numMarginY.Value); }
            set { numMarginX.Value = value.Width; numMarginY.Value = value.Height; }
        }

        public SpriteImportTilingDirection TilingDirection
        {
            get { return (SpriteImportTilingDirection)cmbTileDirection.SelectedIndex; }
            set { cmbTileDirection.SelectedIndex = (int)value; }
        }

        public int MaxTiles
        {
            get { return (int)numMaxTiles.Value; }
            set { numMaxTiles.Value = value; }
        }

        public SpriteImportTransparency SpriteImportMethod
        {
            get
            {
                if (radTransColourIndex0.Checked) { return SpriteImportTransparency.PaletteIndex0; };
                if (radTransColourTopLeftPixel.Checked) { return SpriteImportTransparency.TopLeft; };
                if (radTransColourBottomLeftPixel.Checked) { return SpriteImportTransparency.BottomLeft; };
                if (radTransColourTopRightPixel.Checked) { return SpriteImportTransparency.TopRight; };
                if (radTransColourBottomRightPixel.Checked) { return SpriteImportTransparency.BottomRight; };
                if (radTransColourNone.Checked) { return SpriteImportTransparency.NoTransparency; };
                return SpriteImportTransparency.LeaveAsIs;
            }

            set
            {
                switch(SpriteImportMethod)
                {
                    case SpriteImportTransparency.PaletteIndex0:
                        radTransColourIndex0.Checked = true;
                        break;
                    case SpriteImportTransparency.TopLeft:
                        radTransColourTopLeftPixel.Checked = true;
                        break;
                    case SpriteImportTransparency.BottomLeft:
                        radTransColourBottomLeftPixel.Checked = true;
                        break;
                    case SpriteImportTransparency.TopRight:
                        radTransColourTopRightPixel.Checked = true;
                        break;
                    case SpriteImportTransparency.BottomRight:
                        radTransColourBottomRightPixel.Checked = true;
                        break;
                    case SpriteImportTransparency.NoTransparency:
                        radTransColourNone.Checked = true;
                        break;
                    default:
                        radTransColourLeaveAsIs.Checked = true;
                        break;
                }
            }
        }

        public SpriteImportWindow(string[] filenames)
        {
            InitializeComponent();

            cmbFilenames.Enabled = filenames.Length > 1;
            cmbFilenames.Items.Clear();
            imageLookup = new List<string>();

            foreach (string filename in filenames)
            {
                imageLookup.Add(filename);
                cmbFilenames.Items.Add(Path.GetFileName(filename));
            }

            cmbFilenames.SelectedIndex = 0;

            OneTimeControlSetup();
            PostImageLoad();
        }

        public SpriteImportWindow(Bitmap bmp)
        {
            InitializeComponent();

            image = bmp;
            cmbFilenames.Enabled = false;
            cmbFilenames.Items.Clear();

            OneTimeControlSetup();
            PostImageLoad();
        }

        private void OneTimeControlSetup()
        {
            bool gameUsesIndexedPalette = Factory.AGSEditor.CurrentGame.Settings.ColorDepth == AGS.Types.GameColorDepth.Palette;

            // if import method is for index 0 and this is not an indexed palette
            if (this.SpriteImportMethod == SpriteImportTransparency.PaletteIndex0 && !gameUsesIndexedPalette)
            {
                this.SpriteImportMethod = SpriteImportTransparency.LeaveAsIs;
            }

            // enable or disable things based on the colour depth
            panelIndex0.Enabled = gameUsesIndexedPalette;
            radTransColourIndex0.Enabled = gameUsesIndexedPalette;
            chkRoomBackground.Enabled = gameUsesIndexedPalette;
            chkRoomBackground.Checked = gameUsesIndexedPalette && Factory.AGSEditor.Settings.RemapPalettizedBackgrounds;

            // if tiling direction hasn't been set yet, just take the first option
            if (cmbTileDirection.SelectedIndex == -1)
            {
                cmbTileDirection.SelectedIndex = 0;
            }
        }

        private void PostImageLoad()
        {
            int framecount;

            try
            {
                framecount = SpriteTools.GetFrameCountEstimateFromFile(imageLookup[cmbFilenames.SelectedIndex]);
            }
            catch
            {
                framecount = 1;
            }

            string format = image.PixelFormat.ToString().Substring(6).ToUpper().Replace("BPP", " bit ").Replace("INDEXED", "indexed");
            string frames = framecount > 1 ? String.Format(", {0} frames", framecount) : "";
            lblImageDescription.Text = String.Format("{0} x {1}, {2}{3}", image.Width, image.Height, format, frames);

            // clear old labels to reset the scrollbars
            previewPanel.Controls.Clear();

            // this label is a hack to get the scroll bars to stretch to the size we want
            Label scrollWindowSizer = new Label();
            scrollWindowSizer.Location = new Point(image.Width * zoomLevel, image.Height * zoomLevel);
            scrollWindowSizer.Text = string.Empty;
            scrollWindowSizer.Width = 0;
            scrollWindowSizer.Height = 0;
            previewPanel.Controls.Add(scrollWindowSizer);

            // update colour preview
            try
            {
                panelIndex0.BackColor = image.Palette.Entries[0];
            }
            catch (IndexOutOfRangeException)
            {
                // not an indexed palette
            }

            previewPanel.Refresh();

            // if not doing a tiled import, update selection width and height
            if (!chkTiled.Checked)
            {
                numSizeX.Value = image.Width;
                numSizeY.Value = image.Height;
            }
        }

        private void updateCornerColours(Point point, Size size)
        {
            panelTopLeft.BackColor = image.GetPixel(point.X, point.Y);
            panelTopRight.BackColor = image.GetPixel(point.X + size.Width - 1, point.Y);
            panelBottomLeft.BackColor = image.GetPixel(point.X, point.Y + size.Height - 1);
            panelBottomRight.BackColor = image.GetPixel(point.X + size.Width - 1, point.Y + size.Height - 1);
        }

        private void zoomSlider_Scroll(object sender, EventArgs e)
        {
            int newVertical = (previewPanel.VerticalScroll.Value / zoomLevel) * zoomSlider.Value;
            int newHorizontal = (previewPanel.HorizontalScroll.Value / zoomLevel) * zoomSlider.Value;

            previewPanel.VerticalScroll.Value = Math.Min(newVertical, previewPanel.VerticalScroll.Maximum);
            previewPanel.HorizontalScroll.Value = Math.Min(newHorizontal, previewPanel.HorizontalScroll.Maximum);

            zoomLevel = zoomSlider.Value;
            lblZoom.Text = "Zoom: x " + zoomLevel;
            previewPanel.Controls[0].Location = new Point(image.Width * zoomLevel, image.Height * zoomLevel);
            previewPanel.Invalidate();
        }

        private void previewPanel_Paint(object sender, PaintEventArgs e)
        {
            // Disable anti-aliasing and associated bumpf
            e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
            e.Graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
            e.Graphics.CompositingQuality = CompositingQuality.HighSpeed;

            // Draw the image
            e.Graphics.DrawImage(image, -previewPanel.HorizontalScroll.Value,
                -previewPanel.VerticalScroll.Value, image.Width * zoomLevel, image.Height * zoomLevel);

            // Draw dragging indicator
            if (dragging)
            {
                numSizeX.Value = position.X / zoomLevel - numOffsetX.Value + 1;
                numSizeY.Value = position.Y / zoomLevel - numOffsetY.Value + 1;

                Size snappedSize = new Size((int)numSizeX.Value * zoomLevel, (int)numSizeY.Value * zoomLevel);
                Point snappedStart = new Point((int)numOffsetX.Value * zoomLevel - previewPanel.HorizontalScroll.Value, (int)numOffsetY.Value * zoomLevel - previewPanel.VerticalScroll.Value);
                Point snappedMiddle = new Point(snappedStart.X + snappedSize.Width / 2, snappedStart.Y + snappedSize.Height / 2);

                // draw selection box
                Brush brush = new HatchBrush(HatchStyle.Percent50, Color.HotPink, Color.Transparent);
                e.Graphics.FillRectangle(brush, new Rectangle(snappedStart, snappedSize));

                // draw selection size text
                StringFormat format = new StringFormat();
                format.LineAlignment = StringAlignment.Center;
                format.Alignment = StringAlignment.Center;
                string selection = String.Format("{0} x {1}", numSizeX.Value, numSizeY.Value);
                e.Graphics.DrawString(selection, this.Font, new SolidBrush(Color.Black), snappedMiddle, format);
            }
            else if (TiledImport)
            {
                Pen pen = new Pen(Color.HotPink, 1);
                bool first = true;

                foreach (Rectangle rect in SpriteTools.GetSpriteSelections(ImageSize, SelectionOffset,
                    SelectionSize, TilingMargin, TilingDirection, MaxTiles))
                {
                    if (first)
                    {
                        // update colours based on the selection
                        updateCornerColours(rect.Location, rect.Size);
                        first = false;
                    }

                    // multiply everything by the zoom level
                    Rectangle zoomed = rect;
                    zoomed.X = zoomed.X * zoomLevel - previewPanel.HorizontalScroll.Value;
                    zoomed.Y = zoomed.Y * zoomLevel - previewPanel.VerticalScroll.Value;
                    zoomed.Width *= zoomLevel;
                    zoomed.Height *= zoomLevel;
                    e.Graphics.DrawRectangle(pen, zoomed);
                }
            }
            else
            {
                // if not tiling, reset the colours
                updateCornerColours(new Point(0, 0), new Size(image.Width, image.Height));
            }
        }

        private void btnImport_Click(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.OK;
            this.Close();
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

        private void cmbFilenames_SelectedIndexChanged(object sender, EventArgs e)
        {
            string filename = imageLookup[cmbFilenames.SelectedIndex];
            image = SpriteTools.LoadFirstImageFromFile(filename);
            PostImageLoad();
        }

        private void chkTiled_CheckedChanged(object sender, EventArgs e)
        {
            numOffsetX.Enabled =
                numOffsetY.Enabled =
                numSizeX.Enabled =
                numSizeY.Enabled =
                numMarginX.Enabled =
                numMarginY.Enabled =
                cmbTileDirection.Enabled =
                numMaxTiles.Enabled = chkTiled.Checked;

            previewPanel.Invalidate();
        }

        private void previewPanel_MouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button != MouseButtons.Left && e.Button != MouseButtons.Right)
            {
                // ignore any clicks that aren't the left or right button
                return;
            }

            chkTiled.Checked = true;
            dragging = true;

            Point mouse = e.Location;
            mouse.X = mouse.X + previewPanel.HorizontalScroll.Value;
            mouse.Y = mouse.Y + previewPanel.VerticalScroll.Value;

            // if the first click was the right button
            bool origin = e.Button == MouseButtons.Right && start == null;

            if (origin)
            {
                start = position = new Point(0, 0);
            }
            else if (e.Button == MouseButtons.Left)
            {
                start = position = mouse;
            }

            // track start position if:
            // - this was the first click and it was a right click (forced to origin)
            // - or, this was a left click
            if (origin || e.Button == MouseButtons.Left)
            {
                numOffsetX.Value = start.X / zoomLevel;
                numOffsetY.Value = start.Y / zoomLevel;
            }
        }

        private void previewPanel_MouseMove(object sender, MouseEventArgs e)
        {
            if (dragging)
            {
                Point mouse = e.Location;
                mouse.X = mouse.X + previewPanel.HorizontalScroll.Value;
                mouse.Y = mouse.Y + previewPanel.VerticalScroll.Value;

                if (mouse.X < start.X)
                {
                    mouse.X = start.X;
                }

                if (mouse.Y < start.Y)
                {
                    mouse.Y = start.Y;
                }

                position = mouse;
                position.X = (position.X / zoomLevel) * zoomLevel;
                position.Y = (position.Y / zoomLevel) * zoomLevel;
                previewPanel.Invalidate();
            }
        }

        private void previewPanel_MouseUp(object sender, MouseEventArgs e)
        {
            if (dragging)
            {
                dragging = false;
                previewPanel.Invalidate();
            }
        }

        private void cmbTileDirection_SelectedIndexChanged(object sender, EventArgs e)
        {
            previewPanel.Invalidate();
        }

        private void InvalidateOn_KeyUp(object sender, KeyEventArgs e)
        {
            previewPanel.Invalidate();
        }

        private void InvalidateOn_ValueChanged(object sender, EventArgs e)
        {
            previewPanel.Invalidate();
        }
    }
}