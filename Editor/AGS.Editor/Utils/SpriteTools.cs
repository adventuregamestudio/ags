using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using AGS.Types;

namespace AGS.Editor.Utils
{
    public class SpriteTools
    {
        public static IEnumerable<Bitmap> LoadSpritesFromFile(string fileName, int start = 0)
        {
            // We have to use this stream code because using "new Bitmap(filename)"
            // keeps the file open until the Bitmap is disposed
            FileStream fileStream;

            try
            {
                fileStream = new FileStream(fileName, FileMode.Open, FileAccess.Read);
            }
            catch
            {
                throw new Types.InvalidDataException($"Unable to open file '{fileName}'");
            }

            Bitmap loadedBmp;

            try
            {
                loadedBmp = (Bitmap)Bitmap.FromStream(fileStream);
            }
            catch
            {
                throw new Types.InvalidDataException($"Unable to load image data from '{fileName}'");
            }
            finally
            {
                fileStream.Close();
            }

            // Unfortunately the Bitmap.Clone method will crash later due to
            // a .NET bug when it's loaded from a stream. Therefore we need
            // to make a fresh copy.
            Bitmap oldBmp = loadedBmp;
            loadedBmp = Utilities.CreateCopyOfBitmapPreservingColourDepth(loadedBmp);
            oldBmp.Dispose();

            if ((System.IO.Path.GetExtension(fileName).ToLower() == ".gif") &&
                (loadedBmp.PixelFormat != PixelFormat.Format8bppIndexed))
            {
                // The .NET Bitmap class has a bug, whereby it will convert
                // animated gifs to 32-bit when it loads them. This causes
                // us an issue, so use the custom GifDecoder instead when
                // this happens.
                loadedBmp.Dispose();

                using (GifDecoder decoder = new GifDecoder(fileName))
                {
                    for (int i = start; i < decoder.GetFrameCount(); i++)
                    {
                        loadedBmp = (Bitmap)decoder.GetFrame(i).Clone();
                        yield return loadedBmp;
                        loadedBmp.Dispose();
                    }
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

        public static Bitmap LoadFrameImageFromFile(string fileName, int frame)
        {
            return LoadSpritesFromFile(fileName, frame).FirstOrDefault();
        }

        public static int GetFrameCountEstimateFromFile(string fileName)
        {
            if (!File.Exists(fileName))
            {
                return 0;
            }

            int count;

            try
            {
                using (GifDecoder decoder = new GifDecoder(fileName))
                {
                    count = decoder.GetFrameCount();
                }
            }
            catch (Types.InvalidDataException)
            {
                count = 1;
            }

            return count;
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

        /// <summary>
        /// Tries to create bitmap from file, following Sprite's import settings.
        /// Returns null on failure.
        /// </summary>
        public static Bitmap LoadBitmapForSprite(Sprite spr, string filename, int frame)
        {
            Bitmap bmp;
            try
            {
                bmp = LoadFrameImageFromFile(filename, frame);
                if (bmp == null)
                    return null;
            }
            catch(Exception)
            {
                return null;
            }

            if (spr.ImportAsTile)
            {
                SpriteSheet spritesheet;
                spritesheet = new SpriteSheet(new Point(spr.OffsetX, spr.OffsetY), new Size(spr.ImportWidth, spr.ImportHeight));
                Rectangle selection = spritesheet.GetFirstSpriteSelection(new Size(bmp.Width, bmp.Height));
                if (selection.IsEmpty)
                    return null;
                return bmp.Clone(selection, bmp.PixelFormat);
            }
            return bmp;
        }

        /// <summary>
        /// Tries to create bitmap from the Sprite's source reference,
        /// following Sprite's import settings.
        /// Returns null on failure.
        /// </summary>
        public static Bitmap LoadBitmapFromSource(Sprite spr)
        {
            if (string.IsNullOrEmpty(spr.SourceFile))
                return null;
            return LoadBitmapForSprite(spr, spr.SourceFile, spr.Frame);
        }

        public static void ReplaceSprite(Sprite sprite, Bitmap bmp, bool alpha, bool remapColours, bool useRoomBackground,
            SpriteImportTransparency transparency, string filename, int frame, Rectangle selection, bool tile)
        {
            // ignore alpha channel if not 32 bit ARGB
            bool useAlphaChannel = bmp.PixelFormat != PixelFormat.Format32bppArgb ||
                Factory.AGSEditor.CurrentGame.Settings.ColorDepth != GameColorDepth.TrueColor ? false : alpha;

            // ignore palette remap options if not using an indexed palette
            if (bmp.PixelFormat != PixelFormat.Format8bppIndexed)
            {
                remapColours = false;
                useRoomBackground = false;
            }

            // do replacement
            Factory.NativeProxy.ReplaceSpriteWithBitmap(sprite, bmp, transparency, remapColours, useRoomBackground, useAlphaChannel);

            sprite.TransparentColour = transparency;
            sprite.RemapToGamePalette = remapColours;
            sprite.RemapToRoomPalette = useRoomBackground;
            sprite.SourceFile = Utilities.GetRelativeToProjectPath(filename);
            sprite.Frame = frame;
            sprite.OffsetX = selection.Left;
            sprite.OffsetY = selection.Top;
            sprite.ImportWidth = selection.Width;
            sprite.ImportHeight = selection.Height;
            sprite.ImportAlphaChannel = alpha;
            sprite.ImportAsTile = tile;
        }

        public static void ReplaceSprite(Sprite sprite, Bitmap bmp, bool alpha, bool remapColours, bool useRoomBackground,
            SpriteImportTransparency transparency, string filename, int frame, SpriteSheet spritesheet)
        {
            bool tiled = spritesheet != null;

            if (tiled)
            {
                Rectangle selection = spritesheet.GetFirstSpriteSelection(new Size(bmp.Width, bmp.Height));
                
                if (!selection.IsEmpty)
                {
                    Bitmap replacement = bmp.Clone(selection, bmp.PixelFormat);
                    ReplaceSprite(sprite, replacement, alpha, remapColours, useRoomBackground, transparency, filename,
                        frame, selection, tiled);
                    replacement.Dispose();
                }
                else
                {
                    string message = String.Format("Tiled selection for sprite {0} was out-of-bounds for image '{1}'", sprite.Number, sprite.SourceFile);
                    throw new InvalidOperationException(message);
                }
            }
            else
            {
                Rectangle selection = new Rectangle(0, 0, bmp.Width, bmp.Height);
                ReplaceSprite(sprite, bmp, alpha, remapColours, useRoomBackground, transparency, filename, frame, selection, tiled);
            }
        }

        public static void ReplaceSprite(Sprite sprite, string filename, int frame, bool alpha, bool remapColours, bool useRoomBackground,
            SpriteImportTransparency transparency, SpriteSheet spritesheet)
        {
            Bitmap bmp = LoadFrameImageFromFile(filename, frame);
            ReplaceSprite(sprite, bmp, alpha, remapColours, useRoomBackground, transparency, filename, frame, spritesheet);
            bmp.Dispose();
        }

        public static void ImportNewSprite(SpriteFolder folder, Bitmap bmp, bool alpha, bool remapColours, bool useRoomBackground,
            SpriteImportTransparency transparency, string filename, int frame, Rectangle selection, bool tile)
        {
            // ignore alpha channel if not 32 bit ARGB
            bool useAlphaChannel = bmp.PixelFormat != PixelFormat.Format32bppArgb ||
                Factory.AGSEditor.CurrentGame.Settings.ColorDepth != GameColorDepth.TrueColor ? false : alpha;

            // ignore palette remap options if not using an indexed palette
            if (bmp.PixelFormat != PixelFormat.Format8bppIndexed)
            {
                remapColours = false;
                useRoomBackground = false;
            } 

            // do import
            Sprite sprite = Factory.NativeProxy.CreateSpriteFromBitmap(bmp, transparency, remapColours, useRoomBackground, useAlphaChannel);
            
            sprite.TransparentColour = transparency;
            sprite.RemapToGamePalette = remapColours;
            sprite.RemapToRoomPalette = useRoomBackground;
            sprite.SourceFile = Utilities.GetRelativeToProjectPath(filename);
            sprite.Frame = frame;
            sprite.OffsetX = selection.Left;
            sprite.OffsetY = selection.Top;
            sprite.ImportWidth = selection.Width;
            sprite.ImportHeight = selection.Height;
            sprite.ImportAlphaChannel = alpha;
            sprite.ImportAsTile = tile;

            folder.Sprites.Add(sprite);
        }

        public static void ImportNewSprites(SpriteFolder folder, Bitmap bmp, bool alpha, bool remapColours, bool useRoomBackground,
            SpriteImportTransparency transparency, string filename, int frame, SpriteSheet spritesheet)
        {
            bool tiled = spritesheet != null;

            if (tiled)
            {
                foreach (Rectangle selection in spritesheet.GetSpriteSelections(new Size(bmp.Width, bmp.Height)))
                {
                    Bitmap import = bmp.Clone(selection, bmp.PixelFormat);
                    ImportNewSprite(folder, import, alpha, remapColours, useRoomBackground, transparency, filename, frame, selection, tiled);
                    import.Dispose();
                }
            }
            else
            {
                Rectangle selection = new Rectangle(0, 0, bmp.Width, bmp.Height);
                ImportNewSprite(folder, bmp, alpha, remapColours, useRoomBackground, transparency, filename, frame, selection, tiled);
            }
        }

        public static void ImportNewSprites(SpriteFolder folder, string filename, bool alpha, bool remapColours, bool useRoomBackground,
            SpriteImportTransparency transparency, SpriteSheet spritesheet = null)
        {
            Progress progress = new Progress(GetFrameCountEstimateFromFile(filename), String.Format("Importing frames from {0}", filename));
            progress.Show();
            int frame = 0;

            foreach (Bitmap bmp in LoadSpritesFromFile(filename))
            {
                progress.SetProgressValue(frame);
                ImportNewSprites(folder, bmp, alpha, remapColours, useRoomBackground, transparency, filename, frame, spritesheet);
                bmp.Dispose();
                frame ++;
            }

            progress.Hide();
            progress.Dispose();
        }

        public static void ImportNewSprites(SpriteFolder folder, string[] filenames, bool alpha, bool remapColours, bool useRoomBackground,
            SpriteImportTransparency transparency, SpriteSheet spritesheet = null)
        {
            Progress progress = new Progress(filenames.Length, String.Format("Importing sprites from {0} files", filenames.Length));
            progress.Show();

            for (int index = 0; index < filenames.Length; index ++)
            {
                progress.SetProgressValue(index);
                ImportNewSprites(folder, filenames[index], alpha, remapColours, useRoomBackground, transparency, spritesheet);
            }

            progress.Hide();
            progress.Dispose();
        }

        private static string ExpandExportPath(object obj, string path)
        {
            Type type = obj.GetType();
            PropertyInfo[] properties = type.GetProperties();
            Dictionary<string, string> replace = new Dictionary<string, string>();

            foreach (PropertyInfo property in properties)
            {
                string token = String.Format("%{0}%", property.Name.ToLower());
                object value = property.GetValue(obj);
                replace.Add(token, value != null ? value.ToString() : "");
            }

            Regex regex = new Regex("%[a-zA-Z0-9]+%");

            return regex.Replace(path, delegate(Match m) {
                string lookup = m.Value.ToLower();

                if (replace.ContainsKey(lookup))
                {
                    return replace[lookup];
                }

                return m.Value; });
        }

        public static void ExportSprite(Sprite sprite, string path, bool updateSourcePath, bool resetTileSettings)
        {
            path = ExpandExportPath(sprite, path);

            // stop if the export path is empty (file extension isn't appended yet)
            if (String.IsNullOrWhiteSpace(path.Trim(new char[] { Path.DirectorySeparatorChar })))
            {
                throw new InvalidOperationException("Export path cannot be empty");
            }

            // for a relative path, the parent directory is the game folder
            if (!Path.IsPathRooted(path))
            {
                path = Path.Combine(Factory.AGSEditor.CurrentGame.DirectoryPath, path);
            }

            // create directory if it doesn't exist
            DirectoryInfo parent = Directory.GetParent(path);

            if (parent != null)
            {
                parent.Create();
            }

            ImageFormat format = sprite.ColorDepth < 32 && !sprite.AlphaChannel ? ImageFormat.Bmp : ImageFormat.Png;
            Bitmap bmp = Factory.NativeProxy.GetBitmapForSprite(sprite.Number, sprite.Width, sprite.Height);
            path = string.Format("{0}.{1}", path, format.ToString().ToLower());
            bmp.Save(path, format);
            bmp.Dispose();

            if (updateSourcePath)
            {
                sprite.SourceFile = Utilities.GetRelativeToProjectPath(path);
            }

            if (resetTileSettings)
            {
                sprite.ImportAsTile = false;
                sprite.OffsetX = 0;
                sprite.OffsetY = 0;
            }
        }

        [Flags]
        public enum SkipIf
        {
            SourceValid     = 0x0001,
            SourceLocal     = 0x0002
        }

        public struct ExportSpritesOptions
        {
            public string ExportPath;
            public bool Recursive;
            public SkipIf SkipIf;
            public bool UpdateSourcePath;
            public bool ResetTileSettings;

            public ExportSpritesOptions(string path, bool recurse,
                SkipIf skipIf, bool updateSourcePath, bool resetTileSettings)
            {
                ExportPath = path;
                Recursive = recurse;
                SkipIf = skipIf;
                UpdateSourcePath = updateSourcePath;
                ResetTileSettings = resetTileSettings;
            }
        }

        public static void ExportSprites(SpriteFolder folder, ExportSpritesOptions options, IWorkProgress progress)
        {
            if (!progress.Total.HasValue)
            {
                progress.Total = folder.CountSpritesInAllSubFolders();
                progress.Current = 0;
            }

            var skipIf = options.SkipIf;

            foreach (Sprite sprite in folder.Sprites)
            {
                if (skipIf > 0)
                {
                    string checkPath = Path.IsPathRooted(sprite.SourceFile) ?
                        sprite.SourceFile :
                        Path.Combine(Factory.AGSEditor.CurrentGame.DirectoryPath, sprite.SourceFile);

                    if (skipIf.HasFlag(SkipIf.SourceValid) && File.Exists(checkPath))
                    {
                        progress.Current++;
                        continue; // skip if source is valid
                    }
                    else if (skipIf.HasFlag(SkipIf.SourceLocal) &&
                        File.Exists(checkPath) &&
                        Utilities.PathsAreSameOrNested(checkPath, Factory.AGSEditor.CurrentGame.DirectoryPath))
                    {
                        progress.Current++;
                        continue; // skip if source is valid and local (inside the project folder)
                    }
                }

                ExportSprite(sprite, options.ExportPath, options.UpdateSourcePath, options.ResetTileSettings);
                progress.Current++;
            }

            if (options.Recursive)
            {
                foreach (SpriteFolder subFolder in folder.SubFolders)
                {
                    ExportSprites(subFolder, options, progress);
                }
            }
        }

        public static void ExportSprites(ExportSpritesOptions options, IWorkProgress progress)
        {
            SpriteFolder folder = Factory.AGSEditor.CurrentGame.RootSpriteFolder;
            ExportSprites(folder, options, progress);
        }

        public static Bitmap GetPlaceHolder(int width = 12, int height = 7)
        {
            if (width <= 0 || height <= 0)
            {
                throw new ArgumentOutOfRangeException("Cannot generate a zero-sized bitmap");
            }

            // fine art as a byte array
            byte[][] cup =
            {
                new byte[12] { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                new byte[12] { 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 },
                new byte[12] { 0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00 },
                new byte[12] { 0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x00, 0x01, 0x00 },
                new byte[12] { 0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00 },
                new byte[12] { 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00 },
                new byte[12] { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
            };

            Bitmap bitmap = new Bitmap(width, height, PixelFormat.Format8bppIndexed);
            BitmapData bitmapData = bitmap.LockBits(new Rectangle(0, 0, bitmap.Width, bitmap.Height), ImageLockMode.WriteOnly, bitmap.PixelFormat);
            long ptr = bitmapData.Scan0.ToInt64();
            int y = 0;

            for (int row = 0; row < height; row++)
            {
                int x = 0;
                int span = cup[y].Length;

                while (span == cup[y].Length)
                {
                    if (x + span >= width)
                    {
                        // align with the right edge of the bitmap
                        span = width - x;
                    }

                    Marshal.Copy(cup[y], 0, new IntPtr(ptr), span);
                    x += span;
                    ptr += span;
                }

                // align with bitmap stride
                ptr += bitmapData.Stride - x;

                // next image row
                if (y == cup.Length - 1)
                {
                    y = 0;
                }
                else
                {
                    y++;
                }
            }

            bitmap.UnlockBits(bitmapData);

            // set colours
            ColorPalette palette = bitmap.Palette;
            palette.Entries[0] = Color.FromArgb(0, 0, 0);
            palette.Entries[1] = Color.FromArgb(85, 85, 255);
            palette.Entries[2] = Color.FromArgb(0, 0, 170);
            bitmap.Palette = palette;

            return bitmap;
        }

        /// <summary>
        /// Writes a dummy sprite file with one 1x1 clear sprite at index 0.
        /// </summary>
        public static void WriteDummySpriteFile(string filename)
        {
            int storeFlags = 0;
            if (Factory.AGSEditor.CurrentGame.Settings.OptimizeSpriteStorage)
                storeFlags |= (int)Native.SpriteFileWriter.StorageFlags.OptimizeForSize;
            var compressSprites = Factory.AGSEditor.CurrentGame.Settings.CompressSpritesType;

            var writer = new Native.SpriteFileWriter(filename);
            writer.Begin(storeFlags, compressSprites);
            var bmp = new Bitmap(1, 1);
            writer.WriteBitmap(bmp);
            bmp.Dispose();
            writer.End();
        }

        /// <summary>
        /// Writes the sprite file, importing all the existing sprites either from the
        /// their sources, or sprite cache, - whatever is present (in that order).
        /// </summary>
        public static void WriteSpriteFileFromSources(string filename, IWorkProgress progress)
        {
            int storeFlags = 0;
            if (Factory.AGSEditor.CurrentGame.Settings.OptimizeSpriteStorage)
                storeFlags |= (int)Native.SpriteFileWriter.StorageFlags.OptimizeForSize;
            var compressSprites = Factory.AGSEditor.CurrentGame.Settings.CompressSpritesType;

            SpriteFolder folder = Factory.AGSEditor.CurrentGame.RootSpriteFolder;
            var sprites = folder.GetAllSpritesFromAllSubFolders();
            var orderedSprites = sprites.Distinct().OrderBy(sprite => sprite.Number);

            progress.Total = orderedSprites.Count();
            progress.Current = 0;

            var writer = new Native.SpriteFileWriter(filename);
            writer.Begin(storeFlags, compressSprites);
            int spriteIndex = 0;
            int realSprites = 0;
            foreach (Sprite sprite in orderedSprites)
            {
                // NOTE: we must add empty slots to fill all the gaps in sprite IDs!
                for (; spriteIndex < sprite.Number; ++spriteIndex)
                {
                    writer.WriteEmptySlot();
                }

                // Try get the image, first from source, then from editor's cache
                var bmp = LoadBitmapFromSource(sprite);
                // TODO: this is quite suboptimal, find a way to retrieve a native handle instead?
                if (bmp == null)
                    bmp = Factory.NativeProxy.GetBitmapForSprite(sprite.Number, sprite.Width, sprite.Height);

                if (bmp != null)
                {
                    writer.WriteBitmap(bmp, sprite.TransparentColour, sprite.RemapToGamePalette,
                        sprite.RemapToRoomPalette, sprite.AlphaChannel);
                    bmp.Dispose();
                }
                else
                {
                    bmp = new Bitmap(sprite.Width, sprite.Height);
                    writer.WriteBitmap(bmp);
                    bmp.Dispose();
                }
                progress.Current = ++realSprites;
                spriteIndex++;
            }
            writer.End();
        }
    }
}
