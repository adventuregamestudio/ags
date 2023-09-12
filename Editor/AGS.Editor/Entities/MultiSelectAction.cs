using System;

namespace AGS.Editor
{
    /// <summary>
    /// Describes an action in a list control supporting multi-selection.
    /// CHECKME: does .NET have a standard type for this already?
    /// </summary>
    public enum MultiSelectAction
    {
        // Set a new single selection
        Set,
        // Add a single item to the selection list
        Add,
        // Remove a single item from the selection list
        Remove,
        // Add a range to the selection list
        AddRange,
        // Clear selection
        ClearAll,
    }
}
