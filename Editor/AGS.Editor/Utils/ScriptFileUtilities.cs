using AGS.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;

namespace AGS.Editor.Utils
{
    /// <summary>
    /// These are stateless utility methods for handling script, used by ScriptComponent.
    /// They are separated here for easy testing and possible reusability.
    /// </summary>
    internal class ScriptFileUtilities
    {
        /// <summary>
        /// Filters file names to include only those with ".as{hc}" (header, script) extensions.
        /// </summary>
        /// <param name="fileNames">Array of file names to filter.</param>
        /// <returns>A copy of the input without any non ags script extension files.</returns>
        public static string[] FilterNonScriptFileNames(string[] fileNames, out string[] deleted)
        {
            string[] r = fileNames.Where(fileName => fileName.EndsWith(".asc", StringComparison.OrdinalIgnoreCase) || fileName.EndsWith(".ash", StringComparison.OrdinalIgnoreCase)).ToArray();
            deleted = fileNames.Except(r).ToArray();
            return r;

        }

        /// <summary>
        /// Filters file names to include only those that aren't named "roomX.asc", presumably room scripts.
        /// </summary>
        /// <param name="fileNames">Array of file names to filter.</param>
        /// <returns>A copy of the input without any room script files.</returns>
        public static string[] FilterOutRoomScriptFileNames(string[] fileNames, out string[] deleted)
        {
            string[] r = fileNames.Where(fileName => !(Regex.Match(fileName, @"(?:.*[/\\])?room(\d+)\.asc$").Success)).ToArray();
            deleted = fileNames.Except(r).ToArray();
            return r;
        }

        /// <summary>
        /// Helper to check if a file exists in the provided collection of game scripts and headers.
        /// </summary>
        /// <param name="fileName">Filename to check.</param>
        /// <param name="gameScripts">Collection of game scripts and headers to search in.</param>
        /// <returns>True if the file exists in the game scripts or headers.</returns>
        private static bool IsFileInGameScripts(string fileName, ScriptsAndHeaders gameScripts)
        {
            for (int i = 0; i < gameScripts.Count; i++)
            {
                ScriptAndHeader scriptAndHeader = gameScripts[i];
                string gameScriptFileName = scriptAndHeader.Script.FileName;
                string gameHeaderFileName = scriptAndHeader.Header.FileName;

                if (Utilities.AnyPathsAreEqual(fileName, gameScriptFileName))
                    return true;
                if (Utilities.AnyPathsAreEqual(fileName, gameHeaderFileName))
                    return true;
            }
            return false;
        }

        /// <summary>
        /// Filters out files that are already present in the provided collection of game scripts and headers.
        /// </summary>
        /// <param name="fileNames">Array of file names to filter.</param>
        /// <param name="gameScripts">Collection of game scripts and headers to check against.</param>
        /// <returns>A copy of the input, without the files already in the provided collection of game scripts and headers.</returns>
        public static string[] FilterAlreadyInGameScripts(string[] fileNames, ScriptsAndHeaders gameScripts, out string[] deleted)
        {
            string[] r = fileNames.Where(fileName => !IsFileInGameScripts(fileName, gameScripts)).ToArray();
            deleted = fileNames.Except(r).ToArray();
            return r;
        }

        /// <summary>
        /// Pairs header files ("*.ash") with corresponding script files ("*.asc") from the provided array of file names, using the full path.
        /// Unpaired files will be returning an empty string.
        /// </summary>
        /// <param name="fileNames">Array of file names containing both header and script files.</param>
        /// <returns>A list of tuples where each tuple contains header and script file name, in this order, or an empty string if no corresponding header or script file is found.</returns>
        public static List<Tuple<string, string>> PairHeadersAndScriptFiles(string[] fileNames)
        {
            Dictionary<string, string> ashFiles = fileNames.Where(f => f.EndsWith(".ash"))
                                    .ToDictionary(f => f.Substring(0, f.Length - 4), f => f);

            Dictionary<string, string> ascFiles = fileNames.Where(f => f.EndsWith(".asc"))
                                    .ToDictionary(f => f.Substring(0, f.Length - 4), f => f);

            List<Tuple<string, string>> pairs = ashFiles.Select(ash =>
            {
                string ascValue = ascFiles.ContainsKey(ash.Key) ? ascFiles[ash.Key] : "";

                return new Tuple<string, string>(ash.Value, ascValue);
            }).ToList();

            foreach (var asc in ascFiles)
            {
                if (!ashFiles.ContainsKey(asc.Key))
                {
                    pairs.Add(new Tuple<string, string>("", asc.Value));
                }
            }

            return pairs;
        }

        /// <summary>
        /// Checks if a uniqueKey is already present in any of the script modules in the provided collection.
        /// </summary>
        /// <param name="uniqueKey">A script module uniqueKey, presumably a new one.</param>
        /// <param name="gameScripts">Collection of game scripts and headers to search in.</param>
        /// <returns>True if the script module uniqueKey doesn't exist in provided collection.</returns>
        public static bool IsKeyUnique(int uniqueKey, ScriptsAndHeaders gameScripts)
        {
            for (int i = 0; i < gameScripts.Count; i++)
            {
                ScriptAndHeader scriptAndHeader = gameScripts[i];
                if (scriptAndHeader.Script.UniqueKey == uniqueKey) return false;
            }
            return true;
        }
    }
}
