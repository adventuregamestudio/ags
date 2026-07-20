using System;
using System.Drawing;

namespace AGS.Controls
{
    /// <summary>
    /// ContextMenuTriggerEventArgs issues a command to open a context menu
    /// in the given position of the control's coordinates.
    /// </summary>
    public class ContextMenuTriggerEventArgs
    {
        public ContextMenuTriggerEventArgs(Point position)
        {
            Position = position;
        }

        /// <summary>
        /// Position where the context menu should open, in control's coordinates.
        /// </summary>
        public Point Position { get; private set; }
    }
}
