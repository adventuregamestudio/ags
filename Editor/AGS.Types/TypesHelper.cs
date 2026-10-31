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

        /// <summary>
        /// Converts transparency percentage (0 = opaque, 100 = invisible)
        /// into alpha opacity (0 = invisible, 255 = opaque).
        /// </summary>
        internal static int Trans100ToAlpha255(int transparency)
        {
            transparency = Math.Max(0, Math.Min(100, transparency));
            return ((100 - transparency) * 255) / 100;
        }

        /// <summary>
        /// Converts alpha opacity (0 = invisible, 255 = opaque) into
        /// transparency percentage (0 = opaque, 100 = invisible).
        /// </summary>
        internal static int Alpha255ToTrans100(int alpha)
        {
            alpha = Math.Max(0, Math.Min(255, alpha));
            return 100 - ((alpha * 100) / 255);
        }
    }
}
