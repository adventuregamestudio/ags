using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public partial class ViewPreview : UserControl
    {
        private AGS.Types.View _view;
        private AGS.Types.ViewLoop _loop;
        private AGS.Types.ViewFrame _frame;
        private string _title;
        private Timer _animationTimer;
		private bool _dynamicUpdates = false;
		private int _thisFrameDelay = 0;
		private float _zoomLevel = 1.0f;
        private readonly Size _defaultFrameSize; // default picture frame size
        private bool _autoResize = false;
        private bool _autoFrameFill = false;
        private Point _defaultFramePos;
        private Size _maxLoopFrameSize;
        private Point _loopFrameLeftTopOrigin;

        private const int MILLISECONDS_IN_SECOND = 1000;
        private const int DEFUALT_FRAME_RATE = 40;

        public ViewPreview()
        {
            InitializeComponent();
            _defaultFramePos = panelAutoScroll.Location;
            _defaultFrameSize = previewPanel.ClientSize;
        }

        public string Title
        {
            get { return _title; }
            set { _title = value; mainGroupBox.Text = _title; }
        }

        public AGS.Types.View ViewToPreview
        {
            get { return _view; }
            set { _view = value; UpdateFromView(_view);  }
        }

        public bool IsCharacterView
        {
            get { return chkCentrePivot.Checked; }
            set { chkCentrePivot.Checked = value; }
        }

		public bool DynamicUpdates
		{
			get { return _dynamicUpdates; }
			set { _dynamicUpdates = value; }
		}

        public float ZoomLevel
        {
            get
            {
                return _zoomLevel;
            }
            set
            {
                _zoomLevel = value;
                UpdateSize();
            }
        }

        /// <summary>
        /// Whether the preview frame should automatically fill
        /// available control space, disregarding zoom level.
        /// </summary>
        public bool AutoFrameFill
        {
            get
            {
                return _autoFrameFill;
            }
            set
            {
                _autoFrameFill = value;
                if (_autoFrameFill)
                {
                    panelAutoScroll.Location = _defaultFramePos;
                    // Use Left offset as a reference to "center" the previewPanel inside its parent control
                    panelAutoScroll.Size = new Size(
                        panelAutoScroll.Parent.Width - panelAutoScroll.Left * 2,
                        panelAutoScroll.Parent.Height - panelAutoScroll.Top - panelAutoScroll.Left);
                    panelAutoScroll.Anchor = AnchorStyles.Left | AnchorStyles.Top | AnchorStyles.Right | AnchorStyles.Bottom;
                    previewPanel.Dock = DockStyle.Fill;
                }
                else
                {
                    panelAutoScroll.Location = _defaultFramePos;
                    panelAutoScroll.Anchor = AnchorStyles.Left | AnchorStyles.Top;
                    previewPanel.Dock = DockStyle.None;
                }
                UpdateSize();
            }
        }

        /// <summary>
        /// Whether ViewPreview control should automatically resize itself
        /// whenever preview frame gets too large or small.
        /// </summary>
        public bool AutoResize
        {
            get
            {
                return _autoResize;
            }
            set
            {
                _autoResize = value;
                UpdateSize();
            }
        }

        public AGS.Types.ViewLoop Loop
        {
            get { return _loop; }
        }

        public AGS.Types.ViewFrame Frame
        {
            get { return _frame; }
        }

        public void ReleaseResources()
		{
			StopTimer();
			chkAnimate.Checked = false;
		}

		public void ViewUpdated()
		{
			UpdateFromView(_view);
		}

		protected override void OnEnter(EventArgs e)
		{
			base.OnEnter(e);

			if (_dynamicUpdates)
			{
				ViewUpdated();
			}
		}

        private void panelAutoScroll_Resize(object sender, EventArgs e)
        {
            previewPanel.Invalidate(); // in case was resized by rearranging panels
        }

        private void UpdateFromView(AGS.Types.View view)
        {
            if (view == null)
            {
                udLoop.Enabled = false;
                udFrame.Enabled = false;
                chkAnimate.Enabled = false;
                chkAnimate.Checked = false;
                StopTimer();
                previewPanel.Invalidate();
            }
            else
            {
                udLoop.Enabled = true;
                udFrame.Enabled = true;
                chkAnimate.Enabled = true;
                udLoop.Minimum = 0;
                udLoop.Maximum = (view.Loops.Count == 0) ? 0 : view.Loops.Count - 1;
                udFrame.Minimum = 0;
                udFrame.Maximum = (view.Loops.Count == 0) ? 0 : 
                    ((view.Loops[(int)udLoop.Value].Frames.Count == 0) ? 0 :
                        view.Loops[(int)udLoop.Value].Frames.Count - 1);
                udDelay.Minimum = 1;
                udDelay.Maximum = 100;
                udLoop_ValueChanged(null, null);
                UpdateSize();
            }
        }

        private bool IsFrameValid
        {
            get
            {
                return _frame != null;
            }
        }

        private void UpdateSize()
        {
            if (_view == null)
                return;

            if (_autoFrameFill)
            {
                previewPanel.Invalidate();
                return;
            }

            // Calculate the maximal view frame size,
            // see if the frame may be resized within the control's client size
            Size viewSize = Utilities.GetSizeViewWillBeRenderedInGame(_view);
            viewSize = MathExtra.SafeScale(viewSize, _zoomLevel);
            if (previewPanel.ClientSize.Width < viewSize.Width ||
                previewPanel.ClientSize.Height < viewSize.Height)
            {
                previewPanel.ClientSize = new Size(
                    Math.Max(previewPanel.ClientSize.Width, viewSize.Width),
                    Math.Max(previewPanel.ClientSize.Height, viewSize.Height));
            }
            else
            {
                previewPanel.ClientSize = new Size(
                    Math.Max(_defaultFrameSize.Width, viewSize.Width),
                    Math.Max(_defaultFrameSize.Height, viewSize.Height));
            }
            previewPanel.Invalidate();

            if (_autoResize)
            {
                // Try to calculate necessary size, knowing the previewPanel's
                // location relative to client edges, and its new size
                // (actually use panelAutoScroll as a reference here, as it's
                // previewPanel's immediate parent).
                this.ClientSize = new Size(
                    panelAutoScroll.Left + (this.ClientRectangle.Right - panelAutoScroll.Right) + previewPanel.Width,
                    panelAutoScroll.Top + (this.ClientRectangle.Bottom - panelAutoScroll.Bottom) + previewPanel.Height);
            }
        }

        private void PrecalculateMaxLoopFrameSize()
        {
            _maxLoopFrameSize = Size.Empty;
            _loopFrameLeftTopOrigin = Point.Empty;
            if (_loop == null)
                return;

            int minLeft = 0, maxRight = 0, minTop = 0, maxBottom = 0;
            foreach (var frame in _loop.Frames)
            {
                Size spriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(frame.Image);
                minLeft = Math.Min(minLeft, frame.XOffset);
                maxRight = Math.Max(maxRight, spriteSize.Width + frame.XOffset);
                minTop = Math.Min(minTop, frame.YOffset);
                maxBottom = Math.Max(maxBottom, spriteSize.Height + frame.YOffset);
            }

            _loopFrameLeftTopOrigin.X = -minLeft;
            _loopFrameLeftTopOrigin.Y = -minTop;
            _maxLoopFrameSize.Width = maxRight - minLeft;
            _maxLoopFrameSize.Height = maxBottom - minTop;
        }

        private void previewPanel_Paint(object sender, PaintEventArgs e)
        {
            if (!IsFrameValid)
                return;

            int spriteNum = _frame.Image;
            Size spriteSize = Utilities.GetSizeSpriteWillBeRenderedInGame(spriteNum);
            Size spriteCanvasSize = _maxLoopFrameSize;

            if (_autoFrameFill)
            {
                spriteCanvasSize = MathExtra.SafeScale(spriteCanvasSize, (float)previewPanel.ClientSize.Height / spriteCanvasSize.Height);
            }
            else
            {
                spriteCanvasSize = MathExtra.SafeScale(spriteCanvasSize, _zoomLevel);
            }
                        
            int targetX, targetY, targetWidth, targetHeight;
            if (spriteCanvasSize.Width <= previewPanel.ClientSize.Width && spriteCanvasSize.Height <= previewPanel.ClientSize.Height)
            {
                targetX = chkCentrePivot.Checked ? previewPanel.ClientSize.Width / 2 - spriteCanvasSize.Width / 2 : 0;
                targetY = previewPanel.ClientSize.Height - spriteCanvasSize.Height;
                targetWidth = spriteCanvasSize.Width;
                targetHeight = spriteCanvasSize.Height;
            }
            else
            {
                targetX = 0;
                targetY = 0;
                targetWidth = previewPanel.ClientSize.Width;
                targetHeight = previewPanel.ClientSize.Height;
            }

            Size targetSize = Utilities.ResizeKeepingAspectRatio(spriteCanvasSize, new Size(targetWidth, targetHeight));

            using (Bitmap spriteBmp = Utilities.GetBitmapForSprite(new Sprite(spriteNum, spriteSize.Width, spriteSize.Height)))
            {
                Bitmap useBmp = spriteBmp;
                e.Graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.NearestNeighbor;

                if (_maxLoopFrameSize != spriteBmp.Size)
                {
                    useBmp = new Bitmap(_maxLoopFrameSize.Width, _maxLoopFrameSize.Height);
                    using (Graphics g = Graphics.FromImage(useBmp))
                    {
                        Utilities.DrawFlipped(g, spriteBmp,
                            _loopFrameLeftTopOrigin.X + _frame.XOffset,
                            _loopFrameLeftTopOrigin.Y + _frame.YOffset, _frame.Flip);
                    }
                    e.Graphics.DrawImage(useBmp, new Rectangle(targetX, targetY, targetSize.Width, targetSize.Height));
                }
                else
                {
                    Utilities.DrawFlipped(e.Graphics, spriteBmp,
                            targetX, targetY, targetSize.Width, targetSize.Height, _frame.Flip);
                }
            }
        }

        private void udLoop_ValueChanged(object sender, EventArgs e)
        {
            if (udLoop.Value < _view.Loops.Count)
            {
                _loop = _view.Loops[(int)udLoop.Value];
                int frameCount = _loop.Frames.Count;
                udFrame.Minimum = 0;
                udFrame.Maximum = Math.Max(0, frameCount - 1);
                udFrame_ValueChanged(null, null);
                PrecalculateMaxLoopFrameSize();
            }
            else
            {
                _loop = null;
                _frame = null;
                _maxLoopFrameSize = Size.Empty;
            }
            previewPanel.Invalidate();
        }

        private void udFrame_ValueChanged(object sender, EventArgs e)
        {
            if (_loop != null && udFrame.Value < _loop.Frames.Count)
            {
                _frame = _loop.Frames[(int)udFrame.Value];
            }
            else
            {
                _frame = null;
            }
            
            previewPanel.Invalidate();
        }

        private void chkCentrePivot_CheckedChanged(object sender, EventArgs e)
        {
            previewPanel.Invalidate();
        }

        private void StopTimer()
        {
            if (_animationTimer != null)
            {
                _animationTimer.Stop();
                _animationTimer.Dispose();
                _animationTimer = null;
            }
        }

		private void UpdateDelayForThisFrame()
		{
			_thisFrameDelay = (int)udDelay.Value;

			if (_frame != null)
			{
				_thisFrameDelay += _frame.Delay;
			}
		}

        private void chkAnimate_CheckedChanged(object sender, EventArgs e)
        {
            if (chkAnimate.Checked)
            {
                if (_animationTimer == null)
                {
                    _animationTimer = new Timer();
                    _animationTimer.Tick += _animationTimer_Tick;
		            _animationTimer.Interval = MILLISECONDS_IN_SECOND / DEFUALT_FRAME_RATE;
                }
				UpdateDelayForThisFrame();
                _animationTimer.Start();
            }
            else
            {
                StopTimer();
            }
        }

        private void _animationTimer_Tick(object sender, EventArgs e)
        {
			if (_thisFrameDelay > 0)
			{
				_thisFrameDelay--;
				return;
			}

			if (_dynamicUpdates)
			{
				ViewUpdated();
			}

            if (udFrame.Value < udFrame.Maximum)
            {
                udFrame.Value++;
            }
            else if ((chkSkipFrame0.Checked) && (udFrame.Maximum >= 1))
            {
                udFrame.Value = 1;
            }
            else
            {
                udFrame.Value = 0;
            }
			UpdateDelayForThisFrame();
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.GroupBoxHelper(mainGroupBox, "view-preview");
            t.SetColor("view-preview/numeric-loop/background", c => udLoop.BackColor = c);
            t.SetColor("view-preview/numeric-loop/foreground", c => udLoop.ForeColor = c);
            t.SetColor("view-preview/numeric-frame/background", c => udFrame.BackColor = c);
            t.SetColor("view-preview/numeric-frame/foreground", c => udFrame.ForeColor = c);
            t.SetColor("view-preview/numeric-delay/background", c => udDelay.BackColor = c);
            t.SetColor("view-preview/numeric-delay/foreground", c => udDelay.ForeColor = c);
        }

        private void ViewPreview_Load(object sender, EventArgs e)
        {
            if (!DesignMode)
            {
                Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
            }
        }
    }
}
