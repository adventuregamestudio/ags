using System;
using System.IO;
using AGS.Types;
using AGS.Native;

namespace AGS.CommandLine.Services
{
    /// <summary>
    /// Loads AGS room binary (.crm) files via the AGS.Native C++/CLI bridge.
    /// Always call UnloadRoom() after use to release native memory.
    /// Room operations are NOT thread-safe — one at a time only.
    /// </summary>
    public static class NativeRoomLoader
    {
        private static NativeMethods _native;
        private static readonly object _lock = new object();

        private static NativeMethods GetNative(Game game, string projectPath)
        {
            if (_native == null)
            {
                _native = new NativeMethods(AGS.Types.Version.AGS_EDITOR_VERSION);
                _native.Initialize();
            }
            // Set working dir so native code finds sprite/font files
            _native.NewWorkingDirSet(projectPath);
            // Inform native layer about the game (loads sprite dimensions etc.)
            _native.NewGameLoaded(game, new CompileMessages());
            return _native;
        }

        /// <summary>
        /// Load a full Room from its .crm file. Returns null if the file doesn't exist.
        /// IMPORTANT: call UnloadRoom() when done.
        /// </summary>
        public static Room LoadRoom(UnloadedRoom unloadedRoom, Game game, string projectPath)
        {
            string crmPath = Path.Combine(projectPath, unloadedRoom.FileName);
            if (!File.Exists(crmPath))
                return null;

            lock (_lock)
            {
                string previousDir = Directory.GetCurrentDirectory();
                try
                {
                    Directory.SetCurrentDirectory(projectPath);
                    NativeMethods native = GetNative(game, projectPath);
                    return native.LoadRoom(unloadedRoom, game.TextEncoding);
                }
                finally
                {
                    Directory.SetCurrentDirectory(previousDir);
                }
            }
        }

        /// <summary>
        /// Unload a room loaded by LoadRoom(), freeing native memory.
        /// </summary>
        public static void UnloadRoom(Room room)
        {
            if (room == null) return;
            lock (_lock)
            {
                try { _native?.UnloadRoom(room); }
                catch { /* ignore unload errors */ }
            }
        }

        /// <summary>
        /// Create a blank default .crm file for a new room.
        /// Uses the blank_room.crm embedded resource — a minimal 320x200 32-bit
        /// room with no objects, hotspots or background content, created by AGS Editor 3.6.x.
        /// Works on any project without needing existing .crm files.
        /// </summary>
        public static void CreateDefaultRoom(UnloadedRoom unloadedRoom, Game game, string projectPath)
        {
            string destCrm = Path.Combine(projectPath, unloadedRoom.FileName);
            if (File.Exists(destCrm)) return; // already exists

            // Try embedded resource first (works on any project)
            System.Reflection.Assembly asm = System.Reflection.Assembly.GetExecutingAssembly();
            using (Stream resourceStream = asm.GetManifestResourceStream("AGS.CommandLine.Resources.blank_room.crm"))
            {
                if (resourceStream != null)
                {
                    using (FileStream fs = new FileStream(destCrm, FileMode.Create, FileAccess.Write))
                        resourceStream.CopyTo(fs);
                    return;
                }
            }

            // Fallback: copy the smallest existing .crm in the project
            string templateCrm = null;
            long smallest = long.MaxValue;
            foreach (string f in Directory.GetFiles(projectPath, "room*.crm"))
            {
                long size = new FileInfo(f).Length;
                if (size < smallest) { smallest = size; templateCrm = f; }
            }
            if (templateCrm != null)
                File.Copy(templateCrm, destCrm);
            // If still no template, leave absent — Editor creates it on first open
        }
        public static void SaveRoom(Room room, Game game, string projectPath)
        {
            lock (_lock)
            {
                string previousDir = Directory.GetCurrentDirectory();
                try
                {
                    Directory.SetCurrentDirectory(projectPath);
                    NativeMethods native = GetNative(game, projectPath);
                    native.SaveRoomFile(room);
                }
                finally
                {
                    Directory.SetCurrentDirectory(previousDir);
                }
            }
        }
    }
}
