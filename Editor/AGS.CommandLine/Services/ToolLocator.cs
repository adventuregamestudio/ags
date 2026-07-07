using System;
using System.IO;

namespace AGS.CommandLine.Services
{
    /// <summary>
    /// Locates AGS build tools by searching common paths.
    /// </summary>
    public static class ToolLocator
    {
        /// <summary>
        /// Returns the full path of a tool executable, or null if not found.
        /// Search order: exe directory → parent dirs (up to 4 levels) → PATH.
        /// </summary>
        public static string FindTool(string toolName)
        {
            // Normalise: add .exe on Windows if needed
            string name = toolName;
            if (!name.EndsWith(".exe", StringComparison.OrdinalIgnoreCase))
                name = name + ".exe";

            // 1. Same directory as this exe
            string exeDir = Path.GetDirectoryName(
                System.Reflection.Assembly.GetExecutingAssembly().Location);
            string candidate = TryFind(exeDir, name);
            if (candidate != null) return candidate;

            // 2. Walk up up to 4 parent directories
            string dir = exeDir;
            for (int i = 0; i < 4; i++)
            {
                dir = Path.GetDirectoryName(dir);
                if (dir == null) break;
                candidate = TryFind(dir, name);
                if (candidate != null) return candidate;
            }

            // 3. PATH
            string pathEnv = Environment.GetEnvironmentVariable("PATH") ?? string.Empty;
            foreach (string segment in pathEnv.Split(';'))
            {
                if (string.IsNullOrWhiteSpace(segment)) continue;
                candidate = TryFind(segment.Trim(), name);
                if (candidate != null) return candidate;
            }

            return null;
        }

        private static string TryFind(string dir, string name)
        {
            try
            {
                string full = Path.Combine(dir, name);
                return File.Exists(full) ? full : null;
            }
            catch { return null; }
        }
    }
}
