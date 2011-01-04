using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Text;

namespace AGS.Editor
{
    public static class Hacks
    {
        private const int WM_SETTEXT = 0xC;
        private const int TV_FIRST = 0x1100;
        private const int TVM_GETEDITCONTROL = (TV_FIRST + 15);

        [DllImport("user32", CharSet = CharSet.Auto)]
        static extern int SendMessage(IntPtr hwnd, int wMsg, int wParam, IntPtr lParam);

        // Hack to get around the fact that the BeforeLabelEdit event provides no way to
        // let you change the text they're about to edit
        public static void SetTreeViewEditText(TreeView tree, string myText)
        {
            IntPtr editHandle = (IntPtr)SendMessage(tree.Handle, TVM_GETEDITCONTROL, 0, IntPtr.Zero);
            SendMessage(editHandle, WM_SETTEXT, 0, Marshal.StringToHGlobalAuto(myText));
        }

        // The PropertyGrid doesn't provide a settable SelectedTab property. Well, it does now.
        public static void SetSelectedTabInPropertyGrid(PropertyGrid propGrid, int selectedTabIndex)
        {
            if ((selectedTabIndex < 0) || (selectedTabIndex >= propGrid.PropertyTabs.Count))
            {
                throw new ArgumentException("Invalid tab index to select: " + selectedTabIndex);
            }

            FieldInfo buttonsField = propGrid.GetType().GetField("viewTabButtons", BindingFlags.NonPublic | BindingFlags.Instance);
            ToolStripButton[] viewTabButtons = (ToolStripButton[])buttonsField.GetValue(propGrid);
            ToolStripButton viewTabButton = viewTabButtons[selectedTabIndex];

            propGrid.GetType().InvokeMember("OnViewTabButtonClick", BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.InvokeMethod, null, propGrid, new object[] { viewTabButton, EventArgs.Empty });
        }
    }
}
