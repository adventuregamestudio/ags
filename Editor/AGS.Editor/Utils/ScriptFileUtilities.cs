using AGS.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;

namespace AGS.Editor.Utils
{
    internal class ScriptFileUtilities
    {
        public static string[] FilterNonScriptFileNames(string[] fileNames)
        {
            return fileNames.Where(fileName => fileName.EndsWith(".asc", StringComparison.OrdinalIgnoreCase) || fileName.EndsWith(".ash", StringComparison.OrdinalIgnoreCase)).ToArray();
        }

        public static string[] FilterOutRoomScriptFileNames(string[] fileNames)
        {
            return fileNames.Where(fileName => !(Regex.Match(fileName, @"(?:.*[/\\])?room(\d+)\.asc$").Success)).ToArray();
        }

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

        public static bool IsKeyUnique(int uniqueKey, ScriptsAndHeaders gameScripts)
        {
            for (int i = 0; i < gameScripts.Count; i++)
            {
                ScriptAndHeader scriptAndHeader = gameScripts[i];
                if (scriptAndHeader.Script.UniqueKey == uniqueKey) return false;
            }
            return true;
        }

        public static List<Tuple<string, string>> FilterAlreadyInGameScripts(List<Tuple<string, string>> scriptHeaderPairs, ScriptsAndHeaders gameScripts)
        {
            List<Tuple<string, string>> filteredScriptHeaderPairs = new List<Tuple<string, string>>();

            foreach (var pair in scriptHeaderPairs)
            {
                string headerFileName = pair.Item1;
                string scriptFileName = pair.Item2;
                if (IsFileInGameScripts(headerFileName, gameScripts) || IsFileInGameScripts(scriptFileName, gameScripts))
                    continue;

                // it's not already in the game, add to filtered list
                filteredScriptHeaderPairs.Add(pair);
            }

            return filteredScriptHeaderPairs;
        }
    }
}
