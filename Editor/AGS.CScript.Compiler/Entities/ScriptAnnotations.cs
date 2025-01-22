using System;

namespace AGS.CScript.Compiler
{
    /// <summary>
    /// ScriptAnnotations is a helper class that constructs and parses script annotations.
    /// </summary>
    public static class ScriptAnnotations
    {
        public static string MakeNewScriptMarker(string scriptName)
        {
            return MakeNewScriptMarker(scriptName, string.Empty);
        }

        public static string MakeNewScriptMarker(string scriptName, string moduleName)
        {
            if (string.IsNullOrEmpty(moduleName))
                return $"{Constants.NEW_SCRIPT_MARKER}{scriptName.Replace(@"\", @"\\")}\"";
            else
                return $"{Constants.NEW_SCRIPT_MARKER}{scriptName.Replace(@"\", @"\\")};{moduleName}\"";
        }

        public static bool ParseNewScriptMarker(FastString marker, out string scriptName)
        {
            string stringPackage;
            return ParseNewScriptMarker(marker, out scriptName, out stringPackage);
        }

        public static bool ParseNewScriptMarker(FastString marker, out string scriptName, out string moduleName)
        {
            if (!marker.StartsWith(Constants.NEW_SCRIPT_MARKER))
            {
                scriptName = string.Empty;
                moduleName = string.Empty;
                return false;
            }

            int appendex_at = marker.IndexOf(';', Constants.NEW_SCRIPT_MARKER.Length);
            if (appendex_at >= 0)
            {
                scriptName = marker.Substring(Constants.NEW_SCRIPT_MARKER.Length, appendex_at - Constants.NEW_SCRIPT_MARKER.Length);
                moduleName = marker.Substring(appendex_at + 1, marker.Length - (appendex_at + 1) - 1);
            }
            else
            {
                scriptName = marker.Substring(Constants.NEW_SCRIPT_MARKER.Length, marker.Length - Constants.NEW_SCRIPT_MARKER.Length - 1);
                moduleName = string.Empty;
            }
            return true;
        }
    }
}
