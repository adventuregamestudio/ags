using System;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Controls
{
    /// <summary>
    /// ToolStripDropDownButtonEx is a customized variant of ToolStripDropDownButton.
    /// 
    /// References:
    /// 1. ToolStripDropDownButton (and friends) source code:
    /// https://github.com/dotnet/winforms/tree/main/src/System.Windows.Forms/System/Windows/Forms/Controls/ToolStrips/ToolStripDropDownButton.cs
    /// </summary>
    public class ToolStripDropDownButtonEx : ToolStripDropDownButton
    {
        public ToolStripDropDownButtonEx()
        {
        }

        public ToolStripDropDownButtonEx(string text)
            : base(text)
        {
        }

        public ToolStripDropDownButtonEx(Image image)
            : base(image)
        {
        }

        public ToolStripDropDownButtonEx(string text, Image image)
            : base(text, image)
        {
        }

        public ToolStripDropDownButtonEx(string text, Image image, EventHandler onClick)
            : base(text, image, onClick)
        {
        }

        public ToolStripDropDownButtonEx(string text, Image image, EventHandler onClick, string name)
            : base(text, image, onClick, name)
        {
        }

        public ToolStripDropDownButtonEx(string text, Image image, params ToolStripItem[] dropDownItems)
            : base(text, image, dropDownItems)
        {
        }

        /// <summary>
        /// OnDropDownHide is called whenever the ToolStripDropDownButton
        /// is auto-hiding its own dropdown menu.
        /// -------------------------------------------------------------------
        /// Explanation:
        /// 
        /// When the ToolStripDropDownMenu is closing, it fires a Closing event,
        /// and passes the ToolStripDropDownCloseReason there. Idea is that the external
        /// code can check that reason and decide what to do in each exact case.
        /// The problem is, that for some reason, it does not work. Either our setup breaks
        /// it, or there's a mistake in the version of the .NET Framework we are using.
        /// In any case, the event receives only AppFocusChange reason all the time,
        /// regardless of what have happened.
        /// See:
        /// https://github.com/dotnet/winforms/tree/main/src/System.Windows.Forms/System/Windows/Forms/Controls/ToolStrips/ToolStripDropDown.cs
        /// 
        /// Here we hack the system by presetting ItemClicked reason whenever the Button
        /// is told to auto-hide its menu. Judging by the internal code, this only happens
        /// when another item was "selected", which occurs during the hot-tracking.
        /// </summary>
        protected override void OnDropDownHide(EventArgs e)
        {
            var dropDownMenu = DropDown as ToolStripDropDownMenuEx;
            if (dropDownMenu != null)
            {
                // Double check that some other item was selected on the parent toolstrip.
                foreach (ToolStripItem item in Parent.Items)
                {
                    if (item != null && item != this && item.Selected)
                    {
                        dropDownMenu.SetCloseReason(ToolStripDropDownCloseReason.ItemClicked);
                        break;
                    }
                }
            }
            base.OnDropDownHide(e);
        }
    }
}
