using AGS.Types;
using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.AccessControl;
using System.Security.Principal;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    internal class Utilities
    {
        private const int ERROR_NO_MORE_FILES = 18;

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

        public static string SelectedReligion = "Not a Believer";

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
            List<string> foldersToCreate = new List<string> { "Speech", AudioClip.AUDIO_CACHE_DIRECTORY, "Compiled" };
            if ((AGSEditor.Instance.CurrentGame != null) && (AGSEditor.Instance.CurrentGame.Settings.TargetPlatforms != Targets.Platforms.Windows))
            {
                Targets.Platforms platform = AGSEditor.Instance.CurrentGame.Settings.TargetPlatforms;
                if ((platform & Targets.Platforms.Windows) != 0) foldersToCreate.Add(Path.Combine("Compiled", "Windows"));
                if ((platform & Targets.Platforms.Linux) != 0) foldersToCreate.Add(Path.Combine("Compiled", "Linux"));
                if ((platform & Targets.Platforms.OSX) != 0) foldersToCreate.Add(Path.Combine("Compiled", "OSX"));
                if ((platform & Targets.Platforms.Android) != 0) foldersToCreate.Add(Path.Combine("Compiled", "Android"));
                if ((platform & Targets.Platforms.iOS) != 0) foldersToCreate.Add(Path.Combine("Compiled", "iOS"));
            }
            foreach (string folderName in foldersToCreate)
            {
                if (!Directory.Exists(folderName))
                {
                    Directory.CreateDirectory(folderName);
                }
            }
        }

        public static void EnsurePlatformSubFoldersExist(Targets.Platforms platforms)
        {
            EnsurePlatformSubFoldersExist(platforms, true);
        }

        public static void EnsurePlatformSubFoldersExist(Targets.Platforms platforms, bool removeOthers)
        {
            foreach (Targets.Platforms platform in Enum.GetValues(typeof(Targets.Platforms)))
            {
                string platformFolder = Path.Combine(AGSEditor.Instance.CompiledRootDirectory, platform.ToString());
                // if platform is NOT only Windows, then create all necessary platform folders
                // otherwise, don't create any platform folders
                if (((platform & platforms) != 0) && (platforms != Targets.Platforms.Windows))
                {
                    if (!Directory.Exists(platformFolder))
                    {
                        Directory.CreateDirectory(platformFolder);
                    }
                }
                else if ((Directory.Exists(platformFolder)) && (removeOthers))
                {
                    Directory.Delete(platformFolder, true);
                }
            }
        }

        public static void AddAllMatchingFiles(IList<string> list, string fileMask)
        {
            AddAllMatchingFiles(list, fileMask, false);
        }

        public static void AddAllMatchingFiles(IList<string> list, string fileMask, bool fullPaths)
        {
            string currentDirectory = Directory.GetCurrentDirectory();
            foreach (string fileName in GetDirectoryFileList(currentDirectory, fileMask))
            {
                if (fullPaths)
                {
                    list.Add(fileName);
                }
                else
                {
                    list.Add(fileName.Substring(currentDirectory.Length + 1));
                }
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
                if (Marshal.GetLastWin32Error() == ERROR_NO_MORE_FILES)
                {
                    // On a network share the Framework can throw this if
                    // there are no matching files (reported by RickJ)...
                    // Seems to be a Win32 FindFirstFile bug in certain
                    // circumstances.
                    return new string[0];
                }
                throw;
            }
        }

		/// <summary>
		/// Returns whether the sourceFile is newer than the destinationFile or
		/// if the destinationFile doesn't exist.
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
            return (File.GetLastWriteTime(sourceFile) > File.GetLastWriteTime(destinationFile));
        }

		public static void DeleteFileIfExists(string fileName)
		{
			if (File.Exists(fileName))
			{
				File.Delete(fileName);
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

        public static bool DoesFileNameContainOnlyValidCharacters(string fileName)
        {
            foreach (char c in fileName)
            {
                foreach (char invalidChar in Path.GetInvalidFileNameChars())
                {
                    if (invalidChar == c)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        public static void CopyFont(int fromSlot, int toSlot)
        {
            if (fromSlot == toSlot)
            {
                return;
            }
            if (File.Exists("agsfnt" + fromSlot + ".wfn"))
            {
                File.Copy("agsfnt" + fromSlot + ".wfn", "agsfnt" + toSlot + ".wfn", true);
				// if a read-only file was copied, make the new one normal
				File.SetAttributes("agsfnt" + toSlot + ".wfn", FileAttributes.Archive);
            }
            else if (File.Exists("agsfnt" + fromSlot + ".ttf"))
            {
                File.Copy("agsfnt" + fromSlot + ".ttf", "agsfnt" + toSlot + ".ttf", true);
				File.SetAttributes("agsfnt" + toSlot + ".ttf", FileAttributes.Archive);
            }
            else
            {
                Factory.GUIController.ShowMessage("Unable to create new font: Font " + fromSlot + " not found.", System.Windows.Forms.MessageBoxIcon.Warning);
            }
        }

		public static Bitmap GetBitmapForSpriteResizedKeepingAspectRatio(Sprite sprite, int width, int height, bool centreInNewCanvas, bool drawOutline, Color backgroundColour)
		{
			float targetWidthHeightRatio = (float)width / (float)height;
			float spriteWidthHeightRatio = (float)sprite.Width / (float)sprite.Height;
			int newWidth, newHeight;

			if ((sprite.Width < width / 2) && (sprite.Height < height / 2))
			{
				newWidth = sprite.Width * 2;
				newHeight = sprite.Height * 2;
			}
			else if (spriteWidthHeightRatio > targetWidthHeightRatio)
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
			Graphics g = Graphics.FromImage(newBmp);
			g.Clear(backgroundColour);
			Bitmap bitmapToDraw = Factory.NativeProxy.GetBitmapForSprite(sprite.Number, newWidth, newHeight);

			int x = 0, y = 0;
			if (centreInNewCanvas)
			{
				x = width / 2 - bitmapToDraw.Width / 2;
				y = height - bitmapToDraw.Height;
			}

			g.DrawImage(bitmapToDraw, x, y, bitmapToDraw.Width, bitmapToDraw.Height);

			if (drawOutline)
			{
				g.DrawRectangle(Pens.Brown, x, y, newWidth - 1, newHeight - 1);
			}

			g.Dispose();
			return newBmp;
		}

        /// <summary>
        /// Gets the size at which the sprite will be rendered in the game.
        /// This will be the sprite size, but doubled if it is a 320-res sprite
        /// in a 640-res game.
        /// </summary>
        public static void GetSizeSpriteWillBeRenderedInGame(int spriteSlot, out int width, out int height)
        {
            width = Factory.NativeProxy.GetActualSpriteWidth(spriteSlot);
            height = Factory.NativeProxy.GetActualSpriteHeight(spriteSlot);

            if (Factory.AGSEditor.CurrentGame.IsHighResolution)
            {
                width *= Factory.NativeProxy.GetSpriteResolutionMultiplier(spriteSlot);
                height *= Factory.NativeProxy.GetSpriteResolutionMultiplier(spriteSlot);
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
            int sourceAddress = sourceData.Scan0.ToInt32();
            int destAddress = destData.Scan0.ToInt32();
            for (int y = 0; y < newImage.Height; y++)
            {
                Utilities.CopyMemory(new IntPtr(sourceAddress), new IntPtr(destAddress), destData.Stride);

                sourceAddress += sourceData.Stride;
                destAddress += destData.Stride;
            }
            source.UnlockBits(sourceData);
            newImage.UnlockBits(destData);

            if (source.PixelFormat == PixelFormat.Format8bppIndexed)
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

        public static RegistryKey OpenAGSRegistryKey()
        {
            RegistryKey key;
            try
            {
                key = Registry.CurrentUser.CreateSubKey(AGSEditor.AGS_REGISTRY_KEY);
                if (key == null)
                {
                    Factory.GUIController.ShowMessage("Unable to access registry key: " + AGSEditor.AGS_REGISTRY_KEY, System.Windows.Forms.MessageBoxIcon.Warning);
                }
            }
            catch (UnauthorizedAccessException ex)
            {
                Factory.GUIController.ShowMessage("Unable to write to the registry. Your user preferences cannot be saved. Please contact your system administrator.\n\nError: " + ex.Message, System.Windows.Forms.MessageBoxIcon.Warning);
                key = null;
            }

            return key;
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

        public static bool CreateHardLink(string destFileName, string sourceFileName)
        {
            return CreateHardLink(destFileName, sourceFileName, false);
        }

        public static bool CreateHardLink(string destFileName, string sourceFileName, bool overwrite)
        {
            if (File.Exists(destFileName))
            {
                if (overwrite) File.Delete(destFileName);
                else return false;
            }
            string fileName = "mklink";
            string args = string.Format("/h {0} {1}", destFileName, sourceFileName);
            if (IsMonoRunning())
            {
                fileName = "ln";
                args = string.Format("{0} {1}", sourceFileName, destFileName);
            }
            bool result = (Process.Start(fileName, args) != null);
            if (result)
            {
                // by default the new hard link will be accessible to the current user only
                // instead, we'll change it to be accessible to the entire "Users" group
                FileSecurity fsec = File.GetAccessControl(destFileName);
                fsec.AddAccessRule
                (
                    new FileSystemAccessRule
                    (
                        new SecurityIdentifier(WellKnownSidType.BuiltinUsersSid, null),
                        FileSystemRights.Modify,
                        AccessControlType.Allow
                    )
                );
                File.SetAccessControl(destFileName, fsec);
            }
            return result;
        }

        public static string GetFullPathFromProjectRelative(string relativePath)
        {
            if (AGSEditor.Instance.CurrentGame == null) return relativePath;
            return Path.Combine(AGSEditor.Instance.CurrentGame.DirectoryPath, relativePath);
        }

        public static string GetFullPathFromEditorRelative(string relativePath)
        {
            return Path.Combine(AGSEditor.Instance.EditorDirectory, relativePath);
        }
    }
}
