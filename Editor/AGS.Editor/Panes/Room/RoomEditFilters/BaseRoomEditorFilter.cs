using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// BaseRoomEditorFilter may be used as a parent class for any other
    /// room filter, contains any functionality shared among the room filters.
    /// NOTE: did not implement IRoomEditorFilter for now, but could;
    /// would require adjustments in the derived filter classes though.
    /// </summary>
    public class BaseRoomEditorFilter
    {
        /// <summary>
        /// Generates a label for the Room's navigation bar.
        /// Almost replicates PropertyGridTitle, but skips the typename
        /// whenever either a script name or a description is available,
        /// because only items of the same type are displayed in a navbar's
        /// dropdown list.
        /// </summary>
        protected string MakeLayerItemName(string typeName, string scriptName, string descName, int numID)
        {
            if (string.IsNullOrEmpty(scriptName) && string.IsNullOrEmpty(descName))
                return $"{typeName} {numID}";
            if (string.IsNullOrEmpty(scriptName))
                return $"{descName} (ID {numID})";
            if (string.IsNullOrEmpty(descName))
                return $"{scriptName} (ID {numID})";
            return $"{scriptName}, {descName} (ID {numID})";
        }
    }
}
