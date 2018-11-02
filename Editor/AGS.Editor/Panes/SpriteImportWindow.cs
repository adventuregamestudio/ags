using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;
using System.IO;
using AGS.Editor.Utils;

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
        }

        public bool UseBackgroundSlots
        {
            get { return chkRoomBackground.Checked; }
        }

        public bool UseAlphaChannel
        {
            get { return chkUseAlphaChannel.Checked; }
        }

        public bool TiledImport
        {
            get { return chkTiled.Checked; }
        }

        public Size ImageSize
        {
            get { return new Size(image.Width, image.Height); }
        }

        public Point SelectionOffset
        {
            get { return new Point((int)numOffsetX.Value, (int)numOffsetY.Value); }
        }

        public Size SelectionSize
        {
            get { return new Size((int)numSizeX.Value, (int)numSizeY.Value); }
        }

        public Size TilingMargin
        {
            get { return new Size((int)numMarginX.Value, (int)numMarginY.Value); }
        }

        public AGS.Types.SpriteImportTilingDirection TilingDirection
        {
            get { return (AGS.Types.SpriteImportTilingDirection)cmbTileDirection.SelectedIndex; }
        }

        public int MaxTiles
        {
            get { return (int)numMaxTiles.Value; }
        }

        public int SpriteImportMethod
        {
            get
            {
                // "Pixels of index 0 will be transparent (256-colour games only)"
                if (radTransColourIndex0.Checked) { return 0; };

                // "The top-left pixel will be the transparent colour for this sprite"
                if (radTransColourTopLeftPixel.Checked) { return 1; };

                // "The bottom-left pixel will be the transparent colour for this sprite"
                if (radTransColourBottomLeftPixel.Checked) { return 2; };

                // "The top-right pixel will be the transparent colour for this sprite"
                if (radTransColourTopRightPixel.Checked) { return 3; };

                // "The bottom-right pixel will be the transparent colour for this sprite"
                if (radTransColourBottomRightPixel.Checked) { return 4; };

                // "AGS will remove all transparent pixels by changing them to a very similar non-transparent colour"
                if (radTransColourNone.Checked) { return 6; };

                // "AGS will leave the sprite's pixels as they are. Any pixels that match the AGS Transparent Colour will be invisible."
                return 5;
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
            // enable or disable things based on the colour depth
            panelIndex0.Enabled = Factory.AGSEditor.CurrentGame.Settings.ColorDepth == AGS.Types.GameColorDepth.Palette;
            radTransColourIndex0.Enabled = Factory.AGSEditor.CurrentGame.Settings.ColorDepth == AGS.Types.GameColorDepth.Palette;
            chkRoomBackground.Enabled = Factory.AGSEditor.CurrentGame.Settings.ColorDepth == AGS.Types.GameColorDepth.Palette;
            chkRemapCols.Enabled = Factory.AGSEditor.CurrentGame.Settings.ColorDepth == AGS.Types.GameColorDepth.Palette;

            // pick a default tiling direction
            cmbTileDirection.SelectedIndex = 0;
        }

        private void PostImageLoad()
        {
            int framecount = SpriteTools.GetFrameCountEstimateFromFile(imageLookup[cmbFilenames.SelectedIndex]);
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

            // update colour previews
            panelTopLeft.BackColor = image.GetPixel(0, 0);
            panelTopRight.BackColor = image.GetPixel(image.Width - 1, 0);
            panelBottomLeft.BackColor = image.GetPixel(0, image.Height - 1);
            panelBottomRight.BackColor = image.GetPixel(image.Width - 1, image.Height - 1);

            previewPanel.Refresh();

            // if not doing a tiled import, update selection width and height
            if (!chkTiled.Checked)
            {
                numSizeX.Value = image.Width;
                numSizeY.Value = image.Height;
            }
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

                foreach (Rectangle rect in SpriteTools.GetSpriteSelections(ImageSize, SelectionOffset,
                    SelectionSize, TilingMargin, TilingDirection, MaxTiles))
                {
                    // multiply everything by the zoom level
                    Rectangle zoomed = rect;
                    zoomed.X = zoomed.X * zoomLevel - previewPanel.HorizontalScroll.Value;
                    zoomed.Y = zoomed.Y * zoomLevel - previewPanel.VerticalScroll.Value;
                    zoomed.Width *= zoomLevel;
                    zoomed.Height *= zoomLevel;
                    e.Graphics.DrawRectangle(pen, zoomed);
                }
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
            chkTiled.Checked = true;
            dragging = true;

            Point mouse = e.Location;
            mouse.X = mouse.X + previewPanel.HorizontalScroll.Value;
            mouse.Y = mouse.Y + previewPanel.VerticalScroll.Value;

            start = position = mouse;
            numOffsetX.Value = start.X / zoomLevel;
            numOffsetY.Value = start.Y / zoomLevel;
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