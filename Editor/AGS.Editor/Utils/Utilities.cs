using AGS.Types;
using AGS.Editor.Components;
using AGS.Editor.Utils;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Reflection;

namespace AGS.Editor
{
    /// <summary>
    /// Utilities class contains all kinds of helper functions.
    /// FIXME: reorganize this in multiple classes, current class is a mess.
    /// </summary>
    internal class Utilities
    {
        private const int ERROR_NO_MORE_FILES = 18;

        [DllImport("Kernel32.dll", CharSet = CharSet.Unicode)]
        internal static extern bool CreateHardLink(string lpFileName, string lpExistingFileName, IntPtr lpSecurityAttributes);

        [DllImport("user32.dll", CharSet = CharSet.Auto, CallingConvention = CallingConvention.Winapi)]
        internal static extern IntPtr GetFocus();

        //[DllImport("kernel32.dll")]
        //internal static extern void RtlMoveMemory(IntPtr dest, IntPtr src, uint len);

        [DllImport("user32.dll")]
        internal static extern IntPtr GetForegroundWindow();

        [DllImport("user32.dll")]
        internal static extern IntPtr GetWindowThreadProcessId(IntPtr hWnd, out IntPtr lpdwProcessId);

        [DllImport("user32.dll", EntryPoint = "DestroyIcon")]
        private static extern bool DestroyIcon(IntPtr hIcon);

        private static char[] PathSeparators = new char[]
        { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar };

        public static string SelectedReligion = "Not a Believer";

        public static string NetRuntimeVersion
        {
            get
            {
                string runtime = IsMonoRunning() ? "Mono" : ".NET";
                // By getting the assembly directory location of integers or other basic types we
                // will obtain the assembly that hosts the .NET run-time.
                string version = FileVersionInfo.GetVersionInfo(typeof(int).Assembly.Location).ProductVersion;

                return string.Format("{0} {1}", runtime, version);
            }
        }

        public static void CopyMemory(IntPtr source, IntPtr destination, int numberOfBytes)
        {
            byte[] tmpArray = new byte[numberOfBytes];
            Marshal.Copy(source, tmpArray, 0, numberOfBytes);
            Marshal.Copy(tmpArray, 0, destination, numberOfBytes);
            //RtlMoveMemory(destination, source, (uint)numberOfBytes);
        }

        public static Process GetProcessForActiveApplication()
        {
            IntPtr activeProcessId;
            IntPtr activeWindowHandle = GetForegroundWindow();
            GetWindowThreadProcessId(activeWindowHandle, out activeProcessId);
            return Process.GetProcessById(activeProcessId.ToInt32());
        }

        public static bool IsMonoRunning()
        {
            return Type.GetType("Mono.Runtime") != null;
        }

        public static bool IsThisApplicationCurrentlyActive()
        {
            return (GetProcessForActiveApplication().Id == Process.GetCurrentProcess().Id);
        }

        public static bool IsShiftPressed()
        {
            return (System.Windows.Forms.Control.ModifierKeys & Keys.Shift) == Keys.Shift;
        }

        public static bool IsControlPressed()
        {
            return (System.Windows.Forms.Control.ModifierKeys & Keys.Control) == Keys.Control;
        }

        public static Control GetControlThatHasFocus()
        {
            Control focusControl = null;
            IntPtr focusHandle = GetFocus();
            if (focusHandle != IntPtr.Zero)
            {
                // returns null if handle is not to a .NET control
                focusControl = Control.FromHandle(focusHandle);
            }
            return focusControl;
        }

        public static void EnsureStandardSubFoldersExist()
        {
            // TODO: This list partially contradicts the Output Target concepts,
            // because it explicitly mentions Data target's directories
            List<string> foldersToCreate = new List<string> {
                SpeechComponent.SPEECH_DIRECTORY,
                AudioComponent.AUDIO_CACHE_DIRECTORY,
                AGSEditor.OUTPUT_DIRECTORY, Path.Combine(AGSEditor.OUTPUT_DIRECTORY, AGSEditor.DATA_OUTPUT_DIRECTORY) };
            foreach (string folderName in foldersToCreate)
            {
                if (!Directory.Exists(folderName))
                {
                    Directory.CreateDirectory(folderName);
                }
            }
        }

        public static void AddAllMatchingFiles(IList<string> list, string fileMask)
        {
            AddAllMatchingFiles(list, fileMask, false);
        }

        public static void AddAllMatchingFiles(IList<string> list, string fileMask, bool fullPaths)
        {
            AddAllMatchingFiles(list, Directory.GetCurrentDirectory(), fileMask, fullPaths);
        }

        public static void AddAllMatchingFiles(IList<string> list, string parentDir,
            string fileMask, bool fullPaths, SearchOption searchOption = SearchOption.TopDirectoryOnly)
        {
            var files = GetDirectoryFileList(parentDir, fileMask, searchOption);
            if (fullPaths)
            {
                foreach (string fileName in files)
                    list.Add(fileName);
            }
            else
            {
                foreach (string fileName in files)
                    list.Add(fileName.Substring(parentDir.Length + 1));
            }
        }

        public static string GetRelativeToBasePath(string absolutePath, string basePath)
        {
            if (String.IsNullOrEmpty(absolutePath) ||
                !absolutePath.Contains(basePath))
            {
                return absolutePath;
            }

            Uri basePathUri = new Uri(basePath + Path.DirectorySeparatorChar);
            Uri currentPathUri = new Uri(absolutePath);

            return Uri.UnescapeDataString(basePathUri.MakeRelativeUri(currentPathUri).OriginalString);
        }

        public static string GetRelativeToProjectPath(string absolutePath)
        {
            return GetRelativeToBasePath(absolutePath, Factory.AGSEditor.CurrentGame.DirectoryPath);
        }

        public static string[] GetRelativeToProjectPath(string[] absolutePaths)
        {
            string[] normalizedPaths = new string[absolutePaths.Length];
            for (int i = 0; i < absolutePaths.Length; i++)
            {
                normalizedPaths[i] = GetRelativeToProjectPath(absolutePaths[i]);
            }
            return normalizedPaths;
        }

        /// <summary>
        /// Wraps Path.GetFileName and catches any exceptions, returns empty string if there were any.
        /// </summary>
        public static string SafeGetFileName(string fileName)
        {
            try
            {
                return Path.GetFileName(fileName);
            }
            catch (Exception)
            {
                return string.Empty;
            }
        }

        public static string ResolveSourcePath(string sourcePath)
        {
            Uri baseUri = new Uri(Factory.AGSEditor.CurrentGame.DirectoryPath + Path.DirectorySeparatorChar);
            Uri absoluteUri;

            if (Uri.TryCreate(baseUri, sourcePath, out absoluteUri))
            {
                sourcePath = absoluteUri.LocalPath;
            }

            return sourcePath;
        }

        public static bool PathsAreEqual(string path1, string path2)
        {
            Uri uri1 = new Uri(path1);
            Uri uri2 = new Uri(path2);
            return uri1.Equals(uri2);
        }

        public static bool AnyPathsAreEqual(string path1, string path2)
        {
            return PathsAreEqual(ResolveSourcePath(path1), ResolveSourcePath(path2));
        }

        /// <summary>
        /// Tells if the filepath is nested within parentpath.
        /// </summary>
        public static bool PathIsParentOf(string parentpath, string filepath)
        {
            Uri parentUri = new Uri(parentpath + Path.DirectorySeparatorChar);
            Uri fileUri = new Uri(filepath);
            return parentUri.IsBaseOf(fileUri);
        }

        /// <summary>
        /// Tells if the given path equals to or subdirectory of a basepath.
        /// </summary>
        public static bool PathsAreSameOrNested(string path, string basepath)
        {
            Uri baseUri = new Uri(basepath + Path.DirectorySeparatorChar);
            Uri pathUri = new Uri(path + Path.DirectorySeparatorChar);
            return baseUri.IsBaseOf(pathUri);
        }

        /// <summary>
        /// Tells if the given path equals to or is a subdirectory of one
        /// of the given basepaths
        /// </summary>
        public static bool PathIsSameOrNestedAmong(string path, IEnumerable<string> basepaths)
        {
            return basepaths.Any(basep => Utilities.PathsAreSameOrNested(path, basep));
        }

        /// <summary>
        /// Tells if the path contains "." or ".." sections.
        /// </summary>
        public static bool DoesPathContainDotDirs(string path)
        {
            string[] parts = path.Split(PathSeparators);
            return parts.Any(p => p == "." || p == "..");
        }

        /// <summary>
        /// Replaces "oldBase" parent part of the "path" with the "newBase", assigns "newPath" and returns a result.
        /// If "path" does not contain "oldBase", then fails.
        /// </summary>
        public static bool ReplacePathBase(string path, string oldBase, string newBase, out string newPath)
        {
            Uri oldBaseUri = new Uri(oldBase + Path.DirectorySeparatorChar);
            Uri pathUri = new Uri(path);
            if (!oldBaseUri.IsBaseOf(pathUri))
            {
                newPath = path;
                return false;
            }

            Uri relativeUri = pathUri.MakeRelativeUri(oldBaseUri);
            Uri newBaseUri = new Uri(newBase + Path.DirectorySeparatorChar);
            Uri absoluteUri;
            if (Uri.TryCreate(newBaseUri, relativeUri, out absoluteUri))
            {
                if (pathUri.IsFile)
                    newPath = Path.Combine(absoluteUri.LocalPath, Path.GetFileName(path));
                else
                    newPath = absoluteUri.LocalPath;
                return true;
            }
            newPath = path;
            return false;
        }

        public static bool ReplacePathBaseProjectRelative(string path, string oldBase, string newBase, out string newPath)
        {
            string originalPath = path;
            if (!Path.IsPathRooted(oldBase))
                oldBase = Path.Combine(Factory.AGSEditor.CurrentGame.DirectoryPath, oldBase);
            if (!Path.IsPathRooted(newBase))
                newBase = Path.Combine(Factory.AGSEditor.CurrentGame.DirectoryPath, newBase);
            if (!Path.IsPathRooted(path))
                path = Path.Combine(Factory.AGSEditor.CurrentGame.DirectoryPath, path);

            if (!ReplacePathBase(path, oldBase, newBase, out newPath))
            {
                newPath = originalPath;
                return false;
            }

            newPath = GetRelativeToProjectPath(newPath);
            return true;
        }

        /// <summary>
        /// Wraps Directory.GetDirectories to deal with IO exceptions.
        /// </summary>
        public static string[] GetDirectoryDirList(string directory, string dirMask)
        {
            return GetDirectoryDirList(directory, dirMask, SearchOption.TopDirectoryOnly);
        }

        /// <summary>
        /// Returns results of Directory.GetDirectories, but cut to have only relative path.
        /// </summary>
        public static string[] GetDirectoryRelativeDirList(string directory, string dirMask)
        {
            var dirs = GetDirectoryDirList(directory, dirMask, SearchOption.TopDirectoryOnly);
            for (int i = 0; i< dirs.Length; ++i)
                dirs[i] = dirs[i].Substring(directory.Length + 1); // +1 is for separator
            return dirs;
        }

        /// <summary>
        /// Wraps Directory.GetDirectories to deal with IO exceptions.
        /// </summary>
        public static string[] GetDirectoryDirList(string directory, string dirMask, SearchOption searchOption)
        {
            try
            {
                return Directory.GetDirectories(directory, dirMask, searchOption);
            }
            catch (IOException)
            {
                return new string[] { };
            }
        }

        /// <summary>
        /// Wraps Directory.GetFiles in a handler to deal with an exception
        /// erroneously being thrown on Linux network shares if no files match.
        /// </summary>
        public static string[] GetDirectoryFileList(string directory, string fileMask)
        {
            return GetDirectoryFileList(directory, fileMask, SearchOption.TopDirectoryOnly);
        }

        /// <summary>
        /// Wraps Directory.GetFiles in a handler to deal with an exception
        /// erroneously being thrown on Linux network shares if no files match.
        /// </summary>
        public static string[] GetDirectoryFileList(string directory, string fileMask, SearchOption searchOption)
        {
            try
            {
                return Directory.GetFiles(directory, fileMask, searchOption);
            }
            catch (IOException)
            {
                return new string[] { };
            }
        }

        /// <summary>
        /// Finds a subdirectory variant that is not already present in the given base directory.
        /// Variants are formed by adding an optional string postfix and a number, i.e. "[name][postfix]X".
        /// </summary>
        public static string MakeUniqueDirectory(string baseDirectory, string subDirectory, string postfix = null, int minNumber = 0)
        {
            if (postfix == null)
                postfix = string.Empty;
            if (minNumber < 0)
                minNumber = 0;

            return Enumerable
                    .Range(minNumber, int.MaxValue - minNumber)
                    .Select(i => $"{subDirectory}{postfix}{i}")
                    .First(dir => !Directory.Exists(Path.Combine(baseDirectory, dir)));
        }

        /// <summary>
        /// Finds a filename variant that is not already present in the given directory.
        /// Variants are formed by adding an optional string postfix and a number, i.e. "[name][postfix]-X".
        /// </summary>
        public static string MakeUniqueFileName(string directory, string filename, string postfix = null, int minNumber = 0)
        {
            if (postfix == null)
                postfix = string.Empty;
            if (minNumber < 0)
                minNumber = 0;

            string fileNoExt = Path.GetFileNameWithoutExtension(filename);
            string fileExt = Path.GetExtension(filename);

            return Enumerable
                    .Range(minNumber, int.MaxValue - minNumber)
                    .Select(i => $"{fileNoExt}{postfix}{i}{fileExt}")
                    .First(testFilename => !File.Exists(Path.Combine(directory, testFilename)));
        }

        /// <summary>
        /// Returns whether the sourceFile is newer than the destinationFile or
        /// if the destinationFile doesn't exist.
        /// 
        /// Exceptions:
        ///  - ArgumentException: if source file does not exist
        /// </summary>
        public static bool DoesFileNeedRecompile(string sourceFile, string destinationFile)
        {
            if (!File.Exists(sourceFile))
            {
                throw new ArgumentException("Source file does not exist: " + sourceFile);
            }
            if (!File.Exists(destinationFile))
            {
                return true;
            }
            return (File.GetLastWriteTime(sourceFile) >= File.GetLastWriteTime(destinationFile));
        }

        /// <summary>
        /// Attempts to delete file, handling few common exception cases.
        /// 
        /// Exceptions:
        ///  - CannotDeleteFileException
        /// </summary>
        public static void TryDeleteFile(string fileName)
        {
            if (!File.Exists(fileName))
                return;

            try
            {
                try
                {
                    File.Delete(fileName);
                }
                catch (UnauthorizedAccessException)
                {
                    File.SetAttributes(fileName, FileAttributes.Normal);
                    File.Delete(fileName);
                }
            }
            catch (Exception ex)
            {
                throw new CannotDeleteFileException("Unable to delete the file '" + fileName + "'." + Environment.NewLine + ex.Message, ex);
            }
        }

        /// <summary>
        /// Copies the source to destination, and makes sure that the newly
        /// created destination file is not read-only.
        /// </summary>
        public static void CopyFileAndSetDestinationWritable(string sourceFileName, string destFileName)
        {
            File.Copy(sourceFileName, destFileName, true);
            File.SetAttributes(destFileName, FileAttributes.Archive);
        }

        public static bool SafeCopyFileOverwrite(string sourceFileName, string destFileName, bool makeDestinationWritable = false)
        {
            string bkpDestFileName = destFileName + ".bkp";
            if (File.Exists(destFileName))
            {
                File.Move(destFileName, bkpDestFileName);
            }
            try
            {
                File.Copy(sourceFileName, destFileName, true);
                if (makeDestinationWritable) File.SetAttributes(destFileName, FileAttributes.Archive);
            }
            catch
            {
                if (File.Exists(bkpDestFileName))
                {
                    File.Move(bkpDestFileName, destFileName);
                }
                return false;
            }
            finally
            {
                if (File.Exists(bkpDestFileName))
                {
                    File.Delete(bkpDestFileName);
                }
            }
            return true;
        }

        /// <summary>
        /// Safely copies all files from srcDirectory to dstDirectory and deletes them
        /// in the srcDirectory, ensuring that i/o exceptions are handled in the process.
        /// If any file in srcDir cannot be deleted, such file will be left in place.
        /// </summary>
        public static void SafeMoveDirectoryFiles(string srcDirectory, string dstDirectory)
        {
            DirectoryInfo roomDir = new DirectoryInfo(srcDirectory);
            DirectoryInfo backupDir = new DirectoryInfo(dstDirectory);
            roomDir.CopyAll(backupDir);
            roomDir.DeleteWithoutException(recursive: true);
        }

        /// <summary>
        /// // Copies N bytes from one stream into another; returns number of bytes actually written.
        /// </summary>
        /// <returns></returns>
        public static long CopyStream(Stream instream, Stream outstream, long length)
        {
            byte[] buf = new byte[256 * 1024]; // 256 KB buffer
            long wroteTotal = 0;
            while (length > 0)
            {
                int toRead = Math.Min(buf.Length, (int)Math.Min(int.MaxValue, length));
                int wasRead = instream.Read(buf, 0, toRead);
                if (wasRead == 0)
                    return wroteTotal;
                length -= wasRead;
                outstream.Write(buf, 0, wasRead);
                wroteTotal += wasRead;
            }
            return wroteTotal;
        }

        public static Bitmap GetBitmapForSpriteResizedKeepingAspectRatio(Sprite sprite, int width, int height, bool centreInNewCanvas, bool drawOutline, Color backgroundColour)
        {
            float targetWidthHeightRatio = (float)width / (float)height;
            float spriteWidthHeightRatio = (float)sprite.Width / (float)sprite.Height;
            int newWidth, newHeight;

            if (spriteWidthHeightRatio > targetWidthHeightRatio)
            {
                newWidth = width;
                newHeight = (int)(((float)width / (float)sprite.Width) * (float)sprite.Height);
            }
            else
            {
                newHeight = height;
                newWidth = (int)(((float)height / (float)sprite.Height) * (float)sprite.Width);
            }

            // correct size if a very wide/tall image (eg. 400x2) is shrunk
            if (newWidth < 1) newWidth = 1;
            if (newHeight < 1) newHeight = 1;

            Bitmap newBmp = new Bitmap(width, height, PixelFormat.Format32bppRgb);

            using (Graphics g = Graphics.FromImage(newBmp))
            {
                g.Clear(backgroundColour);
                int x = 0, y = 0;

                using (Bitmap bitmapToDraw = Factory.NativeProxy.GetSpriteBitmapAs32Bit(sprite.Number, newWidth, newHeight))
                {
                    Bitmap bmp = bitmapToDraw ?? SpriteTools.GetPlaceHolder();
                    if (centreInNewCanvas)
                    {
                        x = width / 2 - bmp.Width / 2;
                        y = height - bmp.Height;
                    }

                    g.DrawImage(bmp, x, y, bmp.Width, bmp.Height);
                }

                if (drawOutline)
                {
                    g.DrawRectangle(Pens.Brown, x, y, newWidth - 1, newHeight - 1);
                }
            }

            return newBmp;
        }

        /// <summary>
        /// Gets the size at which the sprite will be rendered in the game.
        /// This will be the sprite size, but doubled if it is a 320-res sprite
        /// in a 640-res game.
        /// </summary>
        public static void GetSizeSpriteWillBeRenderedInGame(int spriteSlot, out int width, out int height)
        {
            // CLNUP maybe remove, the scale factor shouldn't belong to the sprite itself
            SpriteInfo info = Factory.NativeProxy.GetSpriteInfo(spriteSlot);
            width = info.Width;
            height = info.Height;
        }

        /// <summary>
        /// Gets the size at which the sprite will be rendered in the game.
        /// This will be the sprite size, but doubled if it is a 320-res sprite
        /// in a 640-res game.
        /// </summary>
        /// <param name="spriteSlot">The sprite to get the size for.</param>
        /// <returns>A size instance for the sprite. Will be doubled if 320-res sprite.</returns>
        public static Size GetSizeSpriteWillBeRenderedInGame(int spriteSlot)
        {
            int width, height;
            GetSizeSpriteWillBeRenderedInGame(spriteSlot, out width, out height);
            return new Size { Width = width, Height = height };
        }

        /// <summary>
        /// Gets the maximal size at which the given view's frames will be
        /// rendered in game.
        /// </summary>
        public static Size GetSizeViewWillBeRenderedInGame(Types.View view)
        {
            return Factory.NativeProxy.GetMaxViewFrameSize(view);
        }

        /// <summary>
        /// Draws a Sprite on System.Drawing.Graphics (for the room editor)
        /// </summary>
        public static void DrawSpriteOnGraphics(Graphics graphics, int spriteSlot, int x, int y, int width, int height)
        {
            // TODO: optimize this by caching sprite bitmaps?
            using (Bitmap sprite = Factory.NativeProxy.GetSpriteBitmap(spriteSlot))
            {
                // 32-bit and 8-bit sprites can be drawn directly
                switch (sprite.PixelFormat)
                {
                    case PixelFormat.Format32bppArgb:
                    case PixelFormat.Format32bppRgb:
                    case PixelFormat.Format8bppIndexed:
                        graphics.DrawImage(sprite, x, y, width, height);
                        return;
                    default:
                        break;
                }

                // convert to 32bit and draw
                // TODO: optimize this by caching converted images?
                using (Bitmap sprite32bppAlpha = new Bitmap(sprite.Width, sprite.Height, PixelFormat.Format32bppArgb))
                {
                    sprite32bppAlpha.SetRawData(sprite.GetRawData(), sprite.PixelFormat);
                    
                    // handle 16bit sprite transparency
                    if (sprite.PixelFormat == PixelFormat.Format16bppRgb565)
                    {
                        Sprite gameSprite = Factory.AGSEditor.CurrentGame.RootSpriteFolder.FindSpriteByID(spriteSlot, true);
                        if (gameSprite.TransparentColour != SpriteImportTransparency.NoTransparency)
                            sprite32bppAlpha.MakeTransparent();
                    }

                    graphics.DrawImage(sprite32bppAlpha, x, y, width, height);
                }
            }
        }

        public static void CheckLabelWidthsOnForm(Control parentControl)
        {
            foreach (Control child in parentControl.Controls)
            {
                CheckLabelWidthsOnForm(child);

                if (child is Label)
                {
                    if (child.Right > parentControl.ClientRectangle.Right)
                    {
                        //System.Diagnostics.Trace.WriteLine("Control: " + child.ToString() + " X: " + child.Left + "  ParentRight: " + parentControl.ClientRectangle.Right);
                        //System.Diagnostics.Trace.WriteLine("MaxSize was: " + child.MaximumSize.ToString() + "; size was: " + child.Size.ToString());
                        if (child.MaximumSize.Width > 0)
                        {
                            child.MaximumSize = new Size(parentControl.ClientRectangle.Right - child.Left - 10, 0);
                        }
                        else
                        {
                            child.Size = new Size(parentControl.ClientRectangle.Right - child.Left - 10, child.Height);
                        }
                        //System.Diagnostics.Trace.WriteLine("MaxSize now: " + child.MaximumSize.ToString() + "; size now: " + child.Size.ToString());
                    }
                }
            }
        }

        public static Bitmap CreateCopyOfBitmapPreservingColourDepth(Bitmap source)
        {
            Bitmap newImage = new Bitmap(source.Width, source.Height, source.PixelFormat);

            Rectangle rect = new Rectangle(0, 0, source.Width, source.Height);
            BitmapData sourceData = source.LockBits(rect, ImageLockMode.ReadOnly, source.PixelFormat);
            BitmapData destData = newImage.LockBits(rect, ImageLockMode.WriteOnly, newImage.PixelFormat);
            IntPtr sourceAddress = sourceData.Scan0;
            IntPtr destAddress = destData.Scan0;
            for (int y = 0; y < newImage.Height; y++)
            {
                Utilities.CopyMemory(sourceAddress, destAddress, destData.Stride);

                sourceAddress += sourceData.Stride;
                destAddress += destData.Stride;
            }
            source.UnlockBits(sourceData);
            newImage.UnlockBits(destData);

            if (source.IsIndexed())
            {
                ColorPalette sourcePal = source.Palette;
                ColorPalette destPal = newImage.Palette;
                for (int i = 0; i < sourcePal.Entries.Length; i++)
                {
                    destPal.Entries[i] = sourcePal.Entries[i];
                }
                // The palette needs to be re-set onto the bitmap to force it
                // to update its internal storage of the colours
                newImage.Palette = destPal;
            }

            return newImage;
        }

        public static void CopyTextToClipboard(string text)
        {
            try
            {
                Clipboard.SetText(text);
            }
            catch (System.Runtime.InteropServices.ExternalException)
            {
                Factory.GUIController.ShowMessage("Unable to copy the text to the clipboard. The clipboard might be in use by another application.", System.Windows.Forms.MessageBoxIcon.Warning);
            }
        }

        /// <summary>
        /// Converts an image to icon.
        /// Code taken from comments section in:
        /// http://ryanfarley.com/blog/archive/2004/04/06/507.aspx
        /// </summary>
        /// <param name="image">The image</param>
        /// <returns>The icon</returns>
        public static Icon ImageToIcon(Image image)
        {
            Icon icon = null;

            Bitmap bitmap = new Bitmap(image);
            IntPtr UnmanagedIconHandle = bitmap.GetHicon();

            // Clone FromHandle result so we can destroy the unmanaged handle version of the icon before the converted object is passed out.
            icon = Icon.FromHandle(UnmanagedIconHandle).Clone() as Icon;

            //Unfortunately, GetHicon creates an unmanaged handle which must be manually destroyed otherwise a generic error will occur in GDI+.
            DestroyIcon(UnmanagedIconHandle);

            return icon;
        }

        public static bool IsWindowsXPOrHigher()
        {
            OperatingSystem os = Environment.OSVersion;
            // Windows XP reports platform as Win32NT and version number >= 5.1
            return ((os.Platform == PlatformID.Win32NT) && ((os.Version.Major > 5) || ((os.Version.Major == 5) && (os.Version.Minor == 1))));
        }

        public static bool IsWindowsVistaOrHigher()
        {
            OperatingSystem os = Environment.OSVersion;
            // Windows Vista reports platform as Win32NT and version number >= 6
            return ((os.Platform == PlatformID.Win32NT) && (os.Version.Major >= 6));
        }

        /// <summary>
        /// Creates hardlink, if failed then creates file's copy.
        /// </summary>
        /// <param name="destFileName">Destination file path, name of the created hardlink</param>
        /// <param name="sourceFileName">Source file path, what hardlink to</param>
        /// <param name="overwrite">Whether overwrite existing hardlink or not</param>
        /// <returns></returns>
        public static bool HardlinkOrCopy(string destFileName, string sourceFileName, bool overwrite)
        {
            sourceFileName = ResolveSourcePath(sourceFileName);
            destFileName = ResolveSourcePath(destFileName);

            if (File.Exists(destFileName))
            {
                if (!overwrite)
                {
                    return false;
                }

                TryDeleteFile(destFileName);
            }

            if (IsMonoRunning() || !CreateHardLink(destFileName, sourceFileName, IntPtr.Zero))
            {
                File.Copy(sourceFileName, destFileName, overwrite);
            }

            return true;
        }

        /// <summary>
        /// Iterates given array of font family names and return the first
        /// FontFamily object that is installed in the system. If none was
        /// found, returns generic Sans-Serif system font.
        /// </summary>
        /// <param name="fontNames"></param>
        /// <returns></returns>
        public static FontFamily FindExistingFont(string[] fontNames)
        {
            FontFamily fontfam = null;
            foreach (string name in fontNames)
            {
                try
                {
                    fontfam = new FontFamily(name);
                }
                catch (System.ArgumentException)
                {
                    continue;
                }
                if (fontfam.Name == name)
                    return fontfam;
            }
            return new FontFamily(GenericFontFamilies.SansSerif);
        }

        /// <summary>
        /// Returns relation between current graphics resolution and default DpiY (96.0).
        /// Can be used when arranging controls and resizing forms.
        /// </summary>
        /// <param name="control"></param>
        /// <returns></returns>
        public static float CalculateGraphicsProportion(Control control)
        {
            Graphics graphics = control.CreateGraphics();
            float proportion = (float)(graphics.DpiY / 96.0);
            graphics.Dispose();
            return proportion;
        }

        public static bool OpenFileOrDirInFileExplorer(string path)
        {
            if (File.Exists(path))
            {
                if (Utilities.IsMonoRunning())
                {
                    // FIXME - this probably needs to be more platform specific
                    Process.Start(path);
                }
                else
                {
                    Hacks.ShowInExplorer(path);
                }
                return true;
            }
            else if(Directory.Exists(path))
            {
                if (Utilities.IsMonoRunning())
                {
                    // FIXME - this probably needs to be more platform specific
                    Process.Start(path);
                }
                else
                {
                    Hacks.ShowInExplorer(path, null);
                }
                return true;
            }
            return false;
        }

        /// <summary>
        /// Tries to get a config value stored in particular section, under certain key.
        /// If either section or key does not exist, then returns provided default value.
        /// </summary>
        public static string GetConfigString(Dictionary<string, Dictionary<string, string>> cfg, string section, string key, string def)
        {
            Dictionary<string, string> secmap;
            if (!cfg.TryGetValue(section, out secmap))
                return def;
            string value;
            if (!secmap.TryGetValue(key, out value))
                return def;
            return value;
        }

        /// <summary>
        /// Do a simple shallow copy of properties that are both readableand writeable from one object to the other
        /// TO-DO: If we do add a Clone or Duplicate to the objects in AGS.Types, move this to Utilities there.
        /// </summary>
        public static void NaiveCopyProperties(object source_obj, object clone)
        {
            IEnumerable<PropertyInfo> properties = source_obj.GetType().GetProperties()
                .Where(f => f.CanWrite && f.CanRead);

            foreach (PropertyInfo prop in properties)
            {
                prop.SetValue(clone, prop.GetValue(source_obj));
            }
        }
    }
}
