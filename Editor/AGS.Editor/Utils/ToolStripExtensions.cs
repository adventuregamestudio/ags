using System;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    public static class ToolStripExtensions
    {
        /// <summary>
        /// Helper factory method for creating a ToolStripMenuItem using larger parameter list.
        /// </summary>
        public static ToolStripMenuItem CreateMenuItem(string text, Image image, EventHandler onClick, string name, Keys shortcutKeys)
        {
            ToolStripMenuItem item = new ToolStripMenuItem(text, image, onClick, name);
            item.ShortcutKeys = shortcutKeys;
            return item;
        }
    }
}
