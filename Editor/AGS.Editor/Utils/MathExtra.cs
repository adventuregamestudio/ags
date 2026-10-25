using AGS.Types;
using System;
using System.Drawing;

namespace AGS.Editor
{
    public static class MathExtra
    {
        public static T Clamp<T>(this T val, T min, T max) where T : IComparable<T>
        {
            if (min.CompareTo(max) > 0)
                throw new ArgumentOutOfRangeException("Max is less than min");

            if (val.CompareTo(min) < 0) return min;
            else if (val.CompareTo(max) > 0) return max;
            else return val;
        }

        public static Size SafeScale(this Size size, float scale)
        {
            return new Size(Math.Max(1, (int)(size.Width * scale)),
                     Math.Max(1, (int)(size.Height * scale)));
        }

        public static Point AlignInRect(Rectangle rect, Point off, FrameAlignment align)
        {
            int x, y;
            // x alignment
            switch (align)
            {
                case FrameAlignment.TopCenter:
                case FrameAlignment.MiddleCenter:
                case FrameAlignment.BottomCenter:
                    x = rect.Width / 2 + off.X; break;
                case FrameAlignment.TopRight:
                case FrameAlignment.MiddleRight:
                case FrameAlignment.BottomRight:
                    x = rect.Width - 1 + off.X; break;
                case FrameAlignment.TopLeft:
                case FrameAlignment.MiddleLeft:
                case FrameAlignment.BottomLeft:
                default:
                    x = off.X; break;
            }
            // y alignment
            switch (align)
            {
                case FrameAlignment.MiddleLeft:
                case FrameAlignment.MiddleCenter:
                case FrameAlignment.MiddleRight:
                    y = rect.Height / 2 + off.Y; break;
                case FrameAlignment.BottomLeft:
                case FrameAlignment.BottomCenter:
                case FrameAlignment.BottomRight:
                    y = rect.Height - 1 + off.Y; break;
                case FrameAlignment.TopLeft:
                case FrameAlignment.TopCenter:
                case FrameAlignment.TopRight:
                default:
                    y = off.Y; break;
            }
            return new Point(x, y);
        }
    }
}
