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
        private int _lastX, _lastY; //used to prevent useless mouse move events
        private Point _panGrabPoint = Point.Empty;
        public event EventHandler PanGrabbed;
        public event EventHandler PanReleased;
        public bool IsPanning { get; private set; }
        public Cursor PanCursor { get; private set; } = Cursors.Hand; // We need a better hand cursor, using the "link" hand for now

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

        protected virtual void OnPanGrab(EventArgs e)
        {
            PanGrabbed?.Invoke(this, EventArgs.Empty);
        }

        protected virtual void OnPanRelease(EventArgs e)
        {
            PanReleased?.Invoke(this, EventArgs.Empty);
        }

        protected override void OnMouseDown(MouseEventArgs e)
        {
            // defines the shortcut for click pan Control+Shift+Left Click Hold or
            // Middle Click Hold without any modifier.
            if ( (e.Button == MouseButtons.Left   && ModifierKeys == (Keys.Control|Keys.Shift)) ||
                 (e.Button == MouseButtons.Middle && ModifierKeys == Keys.None) )
            {
                OnPanGrab(e);
                _panGrabPoint = e.Location;
                Cursor = PanCursor;
                IsPanning = true;
            }

            base.OnMouseDown(e);
        }

        protected override void OnMouseUp(MouseEventArgs e)
        {
            if (_panGrabPoint != Point.Empty)
            {
                _panGrabPoint = Point.Empty;
                Cursor = Cursors.Default;
                IsPanning = false;
                OnPanRelease(e);
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


