using System;

namespace AGS.Types
{
    /// <summary>
    /// TypesHelper is a collection of internal helpers for use within AGS.Types.
    /// </summary>
    internal static class TypesHelper
    {
        /// <summary>
        /// Generates a standard object's label for the property grid.
        /// </summary>
        internal static string MakePropertyGridTitle(string typeName, string scriptName, string descName, int numID)
        {
            if (string.IsNullOrEmpty(scriptName) && string.IsNullOrEmpty(descName))
                return $"({typeName}; ID {numID})";
            if (string.IsNullOrEmpty(scriptName))
                return $"{descName} ({typeName}; ID {numID})";
            if (string.IsNullOrEmpty(descName))
                return $"{scriptName} ({typeName}; ID {numID})";
            return $"{scriptName}, {descName} ({typeName}; ID {numID})";
        }

        /// <summary>
        /// Generates a standard object's label for the property grid.
        /// </summary>
        internal static string MakePropertyGridTitle(string typeName, string scriptName, int numID)
        {
            return $"{scriptName} ({typeName}; ID {numID})";
        }

        /// <summary>
        /// Generates a standard object's label for the property grid.
        /// </summary>
        internal static string MakePropertyGridTitle(string typeName, int numID)
        {
            return $"{typeName}; ID {numID}";
        }
    }
}
