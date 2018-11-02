using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Text;
using AGS.Types;

namespace AGS.Editor.Utils
{
    public class SpriteTools
    {
        public enum TilingDirection
        {
            Right,
            Down
        }

        public static IEnumerable<Bitmap> LoadSpritesFromFile(string fileName)
        {
            // We have to use this stream code because using "new Bitmap(filename)"
            // keeps the file open until the Bitmap is disposed
            FileStream fileStream = new FileStream(fileName, FileMode.Open, FileAccess.Read);
            Bitmap loadedBmp = (Bitmap)Bitmap.FromStream(fileStream);
            fileStream.Close();

            // Unfortunately the Bitmap.Clone method will crash later due to
            // a .NET bug when it's loaded from a stream. Therefore we need
            // to make a fresh copy.
            loadedBmp = Utilities.CreateCopyOfBitmapPreservingColourDepth(loadedBmp);

            //Bitmap loadedBmp = new Bitmap(fileName);
            if ((System.IO.Path.GetExtension(fileName).ToLower() == ".gif") &&
                (loadedBmp.PixelFormat != PixelFormat.Format8bppIndexed))
            {
                // The .NET Bitmap class has a bug, whereby it will convert
                // animated gifs to 32-bit when it loads them. This causes
                // us an issue, so use the custom GifDecoder instead when
                // this happens.
                loadedBmp.Dispose();
                GifDecoder decoder = new GifDecoder();

                if (decoder.Read(fileName) != GifDecoder.STATUS_OK)
                {
                    throw new AGS.Types.InvalidDataException("Unable to load GIF");
                }

                for (int i = 0; i < decoder.GetFrameCount(); i ++)
                {
                    yield return decoder.GetFrame(i);
                }
            }
            else
            {
                yield return loadedBmp;
            }
        }

        public static Bitmap LoadFirstImageFromFile(string fileName)
        {
            return LoadSpritesFromFile(fileName).FirstOrDefault();
        }

        public static int GetFrameCountEstimateFromFile(string fileName)
        {
            if (!File.Exists(fileName))
            {
                return 0;
            }

            GifDecoder decoder = new GifDecoder();

            if (decoder.Read(fileName) != GifDecoder.STATUS_OK)
            {
                // in the interest of speed, if decode fails assume 1 frame
                return 1;
            }

            // this is a GIF file so just return the frame count
            return decoder.GetFrameCount();
        }

        public static IEnumerable<Rectangle> GetSpriteSelections(Size size, Point offset, Size selection, Size margin, TilingDirection direction, int count)
        {
            Point start = new Point(offset.X, offset.Y);
            Rectangle rect = new Rectangle(start, selection);

            for (int i = 1; i <= count; i ++)
            {
                if (direction == TilingDirection.Right)
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
                else if (direction == TilingDirection.Down)
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

        public static string GetSpriteUsageReport(int spriteNumber, Game game)
        {
            StringBuilder usageReport = new StringBuilder(5000);
            FindSpriteUsageInViews(spriteNumber, usageReport, game.RootViewFolder);

            if (spriteNumber == 0)
            {
                usageReport.AppendLine("Sprite 0 is the default sprite and can never be deleted");
            }

            foreach (MouseCursor cursor in game.Cursors)
            {
                if (cursor.Image == spriteNumber)
                {
                    usageReport.AppendLine("Mouse cursor " + cursor.ID + " (" + cursor.Name + ")");
                }
            }

            foreach (InventoryItem item in game.RootInventoryItemFolder.AllItemsFlat)
            {
                if (item.Image == spriteNumber)
                {
                    usageReport.AppendLine("Inventory item " + item.ID + " (" + item.Name + ")");
                }
            }

            if (game.Settings.DialogOptionsBullet == spriteNumber)
            {
                usageReport.AppendLine("Dialog bullet point image");
            }

            if ((game.Settings.InventoryHotspotMarker.Style == InventoryHotspotMarkerStyle.Sprite) &&
                (game.Settings.InventoryHotspotMarker.Image == spriteNumber))
            {
                usageReport.AppendLine("Inventory hotspot dot");
            }

            foreach (GUI gui in game.RootGUIFolder.AllItemsFlat)
            {
                if (gui.BackgroundImage == spriteNumber)
                {
                    usageReport.AppendLine("GUI " + gui.Name + " background image");
                }

                foreach (GUIControl control in gui.Controls)
                {
                    GUIButton button = control as GUIButton;
                    if (button != null)
                    {                        
                        if ((button.Image == spriteNumber) ||
                            (button.MouseoverImage == spriteNumber) ||
                            (button.PushedImage == spriteNumber))
                        {
                            usageReport.AppendLine("GUI button " + control.Name + " on GUI " + gui.Name);
                        }
                    }
                    GUISlider slider = control as GUISlider;
                    if ((slider != null) && (slider.HandleImage == spriteNumber))
                    {
                        usageReport.AppendLine("GUI slider " + control.Name + " on GUI " + gui.Name);
                    }
                    GUITextWindowEdge edge = control as GUITextWindowEdge;
                    if ((edge != null) && (edge.Image == spriteNumber))
                    {
                        usageReport.AppendLine("Text window edge " + control.Name + " on GUI " + gui.Name);
                    }
                }
            }

            if (usageReport.Length > 0)
            {
                string resultText = "Sprite " + spriteNumber + " is used in the following places. It may also be used in text script commands and in rooms (for example, room object graphics); we cannot detect those uses automatically.";
                resultText += Environment.NewLine + Environment.NewLine + usageReport.ToString();
                return resultText;
            }
            return null;
        }

        private static void FindSpriteUsageInViews(int spriteNumber, StringBuilder report, ViewFolder folder)
        {
            foreach (AGS.Types.View view in folder.Views)
            {
                foreach (ViewLoop loop in view.Loops)
                {
                    foreach (ViewFrame frame in loop.Frames)
                    {
                        if (frame.Image == spriteNumber)
                        {
                            report.AppendLine(string.Format("View {0}, loop {1}, frame {2}", view.ID, loop.ID, frame.ID));
                        }
                    }
                }
            }

            foreach (ViewFolder subFolder in folder.SubFolders)
            {
                FindSpriteUsageInViews(spriteNumber, report, subFolder);
            }
        }

    }
}
