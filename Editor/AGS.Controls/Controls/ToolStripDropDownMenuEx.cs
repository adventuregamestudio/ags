using System;
using System.Collections.Generic;
using System.Drawing;
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
        /// as learnt from the WinForms source code. Used here as a reference.
        /// </summary>
        private const int DefaultScrollButtonHeight = 9;

        private delegate ToolStripControlHost GetScrollButtonDelegate(ToolStripDropDownMenu m);
        /// <summary>
        /// UpScrollButton is a retrieved private property getter of a
        /// ToolStripDropDownMenu, that returns an instance of a up-scrolling item.
        /// </summary>
        private static GetScrollButtonDelegate UpScrollButton
            = (GetScrollButtonDelegate)Delegate.CreateDelegate(typeof(GetScrollButtonDelegate),
                typeof(ToolStripDropDownMenu).GetProperty("UpScrollButton",
                  System.Reflection.BindingFlags.NonPublic
                | System.Reflection.BindingFlags.Instance).GetMethod);
        /// <summary>
        /// UpScrollButton is a retrieved private property getter of a
        /// ToolStripDropDownMenu, that returns an instance of a down-scrolling item.
        /// </summary>
        private static GetScrollButtonDelegate DownScrollButton
            = (GetScrollButtonDelegate)Delegate.CreateDelegate(typeof(GetScrollButtonDelegate),
                typeof(ToolStripDropDownMenu).GetProperty("DownScrollButton",
                  System.Reflection.BindingFlags.NonPublic
                | System.Reflection.BindingFlags.Instance).GetMethod);

        /// <summary>
        /// ScrollInternal is a retrieved private method of a ToolStrip
        /// that controls scrolling of items within the client area.
        /// </summary>
        private static Action<ToolStrip, int> ScrollInternal
            = (Action<ToolStrip, int>)Delegate.CreateDelegate(typeof(Action<ToolStrip, int>),
                typeof(ToolStrip).GetMethod("ScrollInternal",
                  System.Reflection.BindingFlags.NonPublic
                | System.Reflection.BindingFlags.Instance));


        /// <summary>
        /// Gets/sets up/down scroll buttons height.
        /// </summary>
        public int ScrollButtonHeight
        {
            get; set;
        }

        public ToolStripDropDownMenuEx()
        {
            ScrollButtonHeight = 24;
        }

        /// <summary>
        /// Resets the collection of displayed and overflow items after a layout is done.
        /// We do our additional customization here.
        /// </summary>
        protected override void SetDisplayedItems()
        {
            UpScrollButton(this).Control.MinimumSize = new Size(0, ScrollButtonHeight);
            DownScrollButton(this).Control.MinimumSize = new Size(0, ScrollButtonHeight);

            base.SetDisplayedItems();

            // Update the initial scroll, because the default one seem to rely
            // on the default scroll button height.
            DoItemScroll(DefaultScrollButtonHeight - ScrollButtonHeight);
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
            var topPosition = ScrollButtonHeight;
            var bottomPosition = Height - ScrollButtonHeight;

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
                // Enable/disable up and down scroll buttons depending on scroll pos
                UpScrollButton(this).Control.Enabled = firstItem.Bounds.Top < topPosition;
                DownScrollButton(this).Control.Enabled = lastItem.Bounds.Bottom > bottomPosition;
            }
        }
    }
}
