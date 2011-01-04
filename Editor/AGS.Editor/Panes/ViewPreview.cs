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
        private string _title;
        private Timer _animationTimer;
		private bool _dynamicUpdates = false;
		private int _thisFrameDelay = 0;

        private const int MILLISECONDS_IN_SECOND = 1000;
        private const int DEFUALT_FRAME_RATE = 40;

        public ViewPreview()
        {
            InitializeComponent();
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
                udFrame.Maximum = 99;
                udDelay.Minimum = 1;
                udDelay.Maximum = 100;
                udLoop_ValueChanged(null, null);
            }
        }

        private void previewPanel_Paint(object sender, PaintEventArgs e)
        {
            if ((_view != null) && (udLoop.Value < _view.Loops.Count) &&
                (udFrame.Value < _view.Loops[(int)udLoop.Value].Frames.Count))
            {
                ViewFrame thisFrame = _view.Loops[(int)udLoop.Value].Frames[(int)udFrame.Value];
                int spriteNum = thisFrame.Image;
                int spriteWidth = Factory.NativeProxy.GetRelativeSpriteWidth(spriteNum) * 2;
                int spriteHeight = Factory.NativeProxy.GetRelativeSpriteHeight(spriteNum) * 2;
                int x = 0, y;
                y = previewPanel.ClientSize.Height - spriteHeight;
                if (chkCentrePivot.Checked)
                {
                    x = previewPanel.ClientSize.Width / 2 - spriteWidth / 2;
                }
				if ((spriteWidth <= previewPanel.ClientSize.Width) &&
					(spriteHeight <= previewPanel.ClientSize.Height))
				{
					IntPtr hdc = e.Graphics.GetHdc();
					Factory.NativeProxy.DrawSprite(hdc, x, y, spriteNum, thisFrame.Flipped);
					e.Graphics.ReleaseHdc();
				}
				else
				{
					Bitmap bmp = Utilities.GetBitmapForSpriteResizedKeepingAspectRatio(new Sprite(spriteNum, spriteWidth, spriteHeight), previewPanel.ClientSize.Width, previewPanel.ClientSize.Height, chkCentrePivot.Checked, false, SystemColors.Control);

                    if (thisFrame.Flipped)
                    {
                        Point urCorner = new Point(0, 0);
                        Point ulCorner = new Point(bmp.Width, 0);
                        Point llCorner = new Point(bmp.Width, bmp.Height);
                        Point[] destPara = { ulCorner, urCorner, llCorner };
                        e.Graphics.DrawImage(bmp, destPara);
                    }
                    else
                    {
                        e.Graphics.DrawImage(bmp, 1, 1);
                    }

					bmp.Dispose();
				}
            }
        }

        private void udLoop_ValueChanged(object sender, EventArgs e)
        {
            if (udLoop.Value < _view.Loops.Count)
            {
                int frameCount = _view.Loops[(int)udLoop.Value].Frames.Count;
                udFrame.Minimum = 0;
                udFrame.Maximum = Math.Max(0, frameCount - 1);
            }
            previewPanel.Invalidate();
        }

        private void udFrame_ValueChanged(object sender, EventArgs e)
        {
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

			int loop = (int)udLoop.Value;
			int frame = (int)udFrame.Value;
			if ((loop < _view.Loops.Count) &&
				(frame < _view.Loops[loop].Frames.Count))
			{
				ViewFrame thisFrame = _view.Loops[loop].Frames[frame];
				_thisFrameDelay += thisFrame.Delay;
			}
		}

        private void chkAnimate_CheckedChanged(object sender, EventArgs e)
        {
            if (chkAnimate.Checked)
            {
                if (_animationTimer == null)
                {
                    _animationTimer = new Timer();
                    _animationTimer.Tick += new EventHandler(_animationTimer_Tick);
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

        private void udDelay_ValueChanged(object sender, EventArgs e)
        {
        }
    }
}
