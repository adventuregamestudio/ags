using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AGS.Types;

namespace AGS.Editor.Utils
{
    public class SpriteSheet
    {
        private Point offset;
        private Size selection;
        private Size margin;
        private SpriteImportTilingDirection direction;
        private int count;

        public SpriteSheet(Point offset, Size selection, Size margin, SpriteImportTilingDirection direction, int count)
        {
            this.offset = offset;
            this.selection = selection;
            this.margin = margin;
            this.direction = direction;
            this.count = count;
        }

        public SpriteSheet(Point offset, Size selection)
        {
            this.offset = offset;
            this.selection = selection;
            this.margin = new Size(0, 0);
            this.direction = SpriteImportTilingDirection.Right;
            this.count = 1;
        }

        public Rectangle GetFirstSpriteSelection(Size size)
        {
            return GetSpriteSelections(size).FirstOrDefault();
        }

        public IEnumerable<Rectangle> GetSpriteSelections(Size size)
        {
            Point start = new Point(offset.X, offset.Y);
            Rectangle rect = new Rectangle(start, selection);

            if (!SelectionFitsWithinBounds(rect, size))
            {
                yield break;
            }

            for (int i = 1; i <= count; i++)
            {
                if (direction == SpriteImportTilingDirection.Right)
                {
                    if (i > 1)
                    {
                        rect.X += selection.Width + margin.Width;
                    }

                    if (rect.X + rect.Width > size.Width)
                    {
                        rect.X = offset.X;
                        rect.Y += selection.Height + margin.Height;

                        if (rect.Y + rect.Height > size.Height)
                        {
                            yield break;
                        }
                    }
                }
                else if (direction == SpriteImportTilingDirection.Down)
                {
                    if (i > 1)
                    {
                        rect.Y += selection.Height + margin.Height;
                    }

                    if (rect.Y + rect.Height > size.Height)
                    {
                        rect.Y = offset.Y;
                        rect.X += selection.Width + margin.Width;

                        if (rect.X + rect.Width > size.Width)
                        {
                            yield break;
                        }
                    }
                }

                yield return rect;
            }
        }

        public static bool SelectionFitsWithinBounds(Rectangle selection, Size bounds)
        {
            return selection.Left >= 0 &&
                selection.Left + selection.Width <= bounds.Width &&
                selection.Top >= 0 &&
                selection.Top + selection.Height <= bounds.Height;
        }

        public static bool SelectionFitsWithinBitmap(Rectangle selection, Bitmap bmp)
        {
            return SelectionFitsWithinBounds(selection, new Size(bmp.Width, bmp.Height));
        }
    }
}
