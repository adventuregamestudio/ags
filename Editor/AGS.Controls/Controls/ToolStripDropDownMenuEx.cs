using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace AGS.Controls
{
    /// <summary>
    /// ToolStripDropDownMenuEx is a customized variant of ToolStripDropDownMenu.
    /// Note that some behavior here is achieved using "hacks", such as retrieving
    /// private base class methods via reflection.
    /// 
    /// References:
    /// 1. ToolStrip (and friends) source code:
    /// https://referencesource.microsoft.com/#system.windows.forms/winforms/Managed/System/WinForms/ToolStripDropDownMenu.cs
    /// 2. Ideas on coding ToolStrip scroll with a mousewheel:
    /// https://stackoverflow.com/questions/13139074/mouse-wheel-scrolling-toolstrip-menu-items
    /// </summary>
    public class ToolStripDropDownMenuEx : ToolStripDropDownMenu
    {
        /// <summary>
        /// DefaultScrollButtonHeight is the default scrolling button's height,
        /// as learnt from the WinForms source code.
        /// </summary>
        private const int DefaultScrollButtonHeight = 9;

        /// <summary>
        /// ScrollInternal is a retrieved private method of a ToolStrip
        /// that controls scrolling of items within the client area.
        /// </summary>
        private static Action<ToolStrip, int> ScrollInternal
            = (Action<ToolStrip, int>)Delegate.CreateDelegate(typeof(Action<ToolStrip, int>),
                typeof(ToolStrip).GetMethod("ScrollInternal",
                  System.Reflection.BindingFlags.NonPublic
                | System.Reflection.BindingFlags.Instance));


        public ToolStripDropDownMenuEx()
        {
        }

        /// <summary>
        /// Performs item scrolling by mouse wheel.
        /// Raises the System.Windows.Forms.Control.MouseWheel event.
        /// </summary>
        /// <param name="e"></param>
        protected override void OnMouseWheel(MouseEventArgs e)
        {
            // Apply "scroll speed" and negate
            DoItemScroll(-(e.Delta / 4));
            base.OnMouseWheel(e); // base class will fire MouseWheel event
        }

        /// <summary>
        /// Perform item scroll by the given delta pixels.
        /// </summary>
        /// <param name="delta">pixels to scroll by, negative means scroll up, positive means scroll down.</param>
        private void DoItemScroll(int delta)
        {
            // ToolStrip scrolling code idea by Bryce Wagner
            // https://stackoverflow.com/questions/13139074/mouse-wheel-scrolling-toolstrip-menu-items

            if (Items.Count == 0)
                return; // no items

            var firstItem = Items[0];
            var lastItem = Items[Items.Count - 1];
            var topPosition = DefaultScrollButtonHeight;
            var bottomPosition = Height - DefaultScrollButtonHeight;

            // Note that in the vertical scrolling strip the item positions
            // are surpassing top and bottom parent control borders.
            // If they are all kept within control's borders, this means
            // that all items are visible, no need to scroll.
            if (lastItem.Bounds.Bottom < Height && firstItem.Bounds.Top > 0)
                return;

            // Clamp to top and bottom positions.
            // Scroll up until topmost item *moving down* is matching top parent position
            if (delta < 0 && firstItem.Bounds.Top - delta > topPosition)
            {
                delta = firstItem.Bounds.Top - topPosition;
            }
            // Scroll down until bottom item *moving up* is matching bottom parent position
            else if (delta > 0 && lastItem.Bounds.Bottom - delta < bottomPosition)
            {
                delta = lastItem.Bounds.Bottom - bottomPosition;
            }

            if (delta != 0)
            {
                ScrollInternal(this, delta);
            }
        }
    }
}
