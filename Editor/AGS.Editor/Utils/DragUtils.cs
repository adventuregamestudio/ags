using System;
using System.Drawing;

namespace AGS.Editor
{
    /// <summary>
    /// Helper methods for various operations performed by dragging.
    /// </summary>
    public static class DragUtils
    {
        /// <summary>
        /// Returns alignment depending on relative position around rectangle.
        /// </summary>
        public static ContentAlignment? GetSideFromPosition(Rectangle rect, int atx, int aty, int leeway)
        {
            if (atx >= rect.Left - leeway && atx <= rect.Left + leeway)
            {
                if (aty >= rect.Top - leeway && aty <= rect.Top + leeway)
                {
                    return ContentAlignment.TopLeft;
                }
                else if (aty >= rect.Bottom - leeway && aty <= rect.Bottom + leeway)
                {
                    return ContentAlignment.BottomLeft;
                }
                else
                {
                    return ContentAlignment.MiddleLeft;
                }
            }
            else if (atx >= rect.Right - leeway && atx <= rect.Right + leeway)
            {
                if (aty >= rect.Top - leeway && aty <= rect.Top + leeway)
                {
                    return ContentAlignment.TopRight;
                }
                else if (aty >= rect.Bottom - leeway && aty <= rect.Bottom + leeway)
                {
                    return ContentAlignment.BottomRight;
                }
                else
                {
                    return ContentAlignment.MiddleRight;
                }
            }
            else
            {
                if (aty >= rect.Top - leeway && aty <= rect.Top + leeway)
                {
                    return ContentAlignment.TopCenter;
                }
                else if (aty >= rect.Bottom - leeway && aty <= rect.Bottom + leeway)
                {
                    return ContentAlignment.BottomCenter;
                }
            }
            return null;
        }

        /// <summary>
        /// Returns a relative point of the given rectangle, based on alignment.
        /// </summary>
        public static Point GetReferencePoint(Rectangle rect, ContentAlignment side)
        {
            switch (side)
            {
                case ContentAlignment.TopLeft:
                    return new Point(rect.Left, rect.Top);
                case ContentAlignment.TopCenter:
                    return new Point(rect.Left + rect.Width / 2, rect.Top);
                case ContentAlignment.TopRight:
                    return new Point(rect.Left + rect.Width - 1, rect.Top);
                case ContentAlignment.MiddleLeft:
                    return new Point(rect.Left, rect.Top + rect.Height / 2);
                case ContentAlignment.MiddleCenter:
                    return new Point(rect.Left + rect.Width / 2, rect.Top + rect.Height / 2);
                case ContentAlignment.MiddleRight:
                    return new Point(rect.Left + rect.Width - 1, rect.Top + rect.Height / 2);
                case ContentAlignment.BottomLeft:
                    return new Point(rect.Left, rect.Top + rect.Height - 1);
                case ContentAlignment.BottomCenter:
                    return new Point(rect.Left + rect.Width / 2, rect.Top + rect.Height - 1);
                case ContentAlignment.BottomRight:
                    return new Point(rect.Left + rect.Width - 1, rect.Top + rect.Height - 1);
                default:
                    return Point.Empty;
            }
        }

        public static Rectangle ResizeRectangle(Rectangle rect, ContentAlignment side, Point newRefPt, Rectangle constraint)
        {
            int l = rect.Left, r = rect.Right, t = rect.Top, b = rect.Bottom;
            switch (side)
            {
                case ContentAlignment.TopLeft:
                case ContentAlignment.MiddleLeft:
                case ContentAlignment.BottomLeft:
                    l = Math.Min(newRefPt.X, r - 1); break;
                case ContentAlignment.TopRight:
                case ContentAlignment.MiddleRight:
                case ContentAlignment.BottomRight:
                    r = Math.Max(newRefPt.X, l + 1); break;
                default: break;
            }
            switch (side)
            {
                case ContentAlignment.TopLeft:
                case ContentAlignment.TopCenter:
                case ContentAlignment.TopRight:
                    t = Math.Min(newRefPt.Y, b - 1); break;
                case ContentAlignment.BottomLeft:
                case ContentAlignment.BottomCenter:
                case ContentAlignment.BottomRight:
                    b = Math.Max(newRefPt.Y, t + 1); break;
                default: break;
            }
            if (!constraint.IsEmpty)
            {
                l = MathExtra.Clamp(l, constraint.Left, constraint.Right - 1);
                r = MathExtra.Clamp(r, l + 1, constraint.Right);
                t = MathExtra.Clamp(t, constraint.Top, constraint.Bottom - 1);
                b = MathExtra.Clamp(b, t + 1, constraint.Bottom);
            }
            return Rectangle.FromLTRB(l, t, r, b);
        }
    }
}
