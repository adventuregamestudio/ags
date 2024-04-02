using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;

namespace AGS.Editor
{
    class LineInBetween : Control
    {
        public LineInBetween()
        {            
        }

        protected override void WndProc(ref Message m)
        {
            // see https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-nchittest
            const int WM_NCHITTEST = 0x0084;
            const int HTTRANSPARENT = -1;

            // Ensure clicks go through
            if (m.Msg == WM_NCHITTEST)
            {
                m.Result = new IntPtr(HTTRANSPARENT);
                return;
            }

            base.WndProc(ref m);
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);

            SolidBrush fore_brush = new SolidBrush(ForeColor);
            Pen fore_pen = new Pen(fore_brush);
            fore_pen.Width = 3;

            SolidBrush back_brush = new SolidBrush(BackColor);

            Pen back_pen = new Pen(back_brush);
            back_pen.Width = 5;

            e.Graphics.DrawLine(back_pen, 0, Height / 2, Width, Height / 2);
            e.Graphics.DrawLine(back_pen, 0, 0, 0, Height);
            e.Graphics.DrawLine(back_pen, Width, 0, Width, Height);

            e.Graphics.DrawLine(fore_pen, 0, Height / 2, Width, Height / 2);
            e.Graphics.DrawLine(fore_pen, 0, 0, 0, Height);
            e.Graphics.DrawLine(fore_pen, Width, 0, Width, Height);
        }

        public void ShowAndHideAt(int x, int y, int w, int h)
        {
            Top = y;
            Left = x;
            Size = new Size(w, h);

            Show();
        }
    }
}
