using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class FreePanControl : BufferedPanel
    {
        private const int WS_EX_COMPOSITED = 0x02000000;
        private MouseButtons _panButtons = MouseButtons.Middle; // which buttons trigger panning
        private int _lastX, _lastY; //used to prevent useless mouse move events
        private Point _panGrabPoint = Point.Empty;
        private MouseButtons _panGrabButton = MouseButtons.None; // which button was used last
        public event EventHandler PanGrabbed;
        public event EventHandler PanReleased;
        public bool IsPanning { get; private set; }
        public Cursor PanCursor { get; private set; } = Cursors.Hand; // We need a better hand cursor, using the "link" hand for now

        /// <summary>
        /// Which mouse buttons should be panning the image.
        /// </summary>
        public MouseButtons PanButtons
        {
            get { return _panButtons; }
            set
            {
                _panButtons = value;
                if (!_panButtons.HasFlag(_panGrabButton))
                {
                    OnPanRelease();
                }
            }
        }

        // Enabling Composite makes scrolling smoother
        protected override CreateParams CreateParams
        {
            get
            {
                CreateParams cp = base.CreateParams;
                cp.ExStyle |= WS_EX_COMPOSITED;
                return cp;
            }
        }

        protected virtual void OnPanGrab(Point grabPoint, MouseButtons grabButton)
        {
            _panGrabPoint = grabPoint;
            _panGrabButton = grabButton;
            Cursor = PanCursor;
            IsPanning = true;
            PanGrabbed?.Invoke(this, EventArgs.Empty);
        }

        protected virtual void OnPanRelease()
        {
            _panGrabPoint = Point.Empty;
            _panGrabButton = MouseButtons.None;
            Cursor = Cursors.Default;
            IsPanning = false;
            PanReleased?.Invoke(this, EventArgs.Empty);
        }

        protected override void OnMouseDown(MouseEventArgs e)
        {
            if (PanButtons.HasFlag(e.Button) && ModifierKeys == Keys.None)
            {
                OnPanGrab(e.Location, e.Button);
            }

            base.OnMouseDown(e);
        }

        protected override void OnMouseUp(MouseEventArgs e)
        {
            if (_panButtons.HasFlag(_panGrabButton))
            {
                OnPanRelease();
            }

            base.OnMouseUp(e);
        }

        protected override void OnMouseMove(MouseEventArgs e)
        {
            if ((e.X == _lastX) && (e.Y == _lastY))
            {
                return;
            }

            _lastX = e.X;
            _lastY = e.Y;

            if (_panGrabPoint != Point.Empty)
            {
                Point scrollPosition = AutoScrollPosition;
                scrollPosition.X = _panGrabPoint.X - e.X - scrollPosition.X;
                scrollPosition.Y = _panGrabPoint.Y - e.Y - scrollPosition.Y;
                AutoScrollPosition = scrollPosition;
                Refresh(); // this prevents the room image to look garbled when panning
                _panGrabPoint = e.Location; // <- without this the pan doesn't work
            }

            base.OnMouseMove(e);
        }

    }
}


