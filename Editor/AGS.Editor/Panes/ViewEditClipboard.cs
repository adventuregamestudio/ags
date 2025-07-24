using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// ViewEditClipboard shares copied data among the ViewLoopEditor instances.
    /// </summary>
    public class ViewEditClipboard
    {
        /// <summary>
        /// Last sprite selected to be assigned to the view's frame.
        /// </summary>
        public int LastSelectedSprite { get; set; }

        /// <summary>
        /// A copied loop, can be pasted into the ViewLoopEditor.
        /// </summary>
        public ViewLoop CopiedLoop { get; set; }
    }
}
