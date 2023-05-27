using System;
using System.Collections.Generic;
using System.Drawing;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Text;

namespace AGS.Editor
{
    public static class Hacks
    {
        private const int WM_SETTEXT = 0xC;
        private const int WM_SETREDRAW = 0x0b;
        private const int WM_USER = 0x400;
        // RichTextBox messages
        private const int EM_SETSEL = 0x00B1;
        private const int EM_REPLACESEL = 0x00C2;
        private const int EM_GETSCROLLPOS = WM_USER + 221;
        private const int EM_SETSCROLLPOS = WM_USER + 222;
        // TreeView messages
        private const int TV_FIRST = 0x1100;
        private const int TVM_GETEDITCONTROL = (TV_FIRST + 15);

        [DllImport("user32", CharSet = CharSet.Auto)]
        static extern int SendMessage(IntPtr hwnd, int wMsg, int wParam, IntPtr lParam);

        /// <summary>
        /// Disable Control's redrawing itself.
        /// </summary>
        public static void SuspendDrawing(this Control control)
        {
            SendMessage(control.Handle, WM_SETREDRAW, 0, IntPtr.Zero);
        }

        /// <summary>
        /// Enable Controls' redrawing itself.
        /// </summary>
        public static void ResumeDrawing(this Control control)
        {
            SendMessage(control.Handle, WM_SETREDRAW, 1, IntPtr.Zero);
            control.Invalidate();
        }

        /// <summary>
        /// Gets current RichTextBox scroll position.
        /// </summary>
        public static Point GetScrollPos(this RichTextBox rtb)
        {
            IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf<Point>());
            SendMessage(rtb.Handle, EM_GETSCROLLPOS, 0, pnt);
            var pos = Marshal.PtrToStructure(pnt, typeof(Point));
            Marshal.FreeHGlobal(pnt);
            return (Point)pos;
        }

        /// <summary>
        /// Sets current RichTextBox scroll position.
        /// </summary>
        public static void SetScrollPos(this RichTextBox rtb, Point pos)
        {
            IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(pos));
            Marshal.StructureToPtr(pos, pnt, false);
            SendMessage(rtb.Handle, EM_SETSCROLLPOS, 0, pnt);
            Marshal.FreeHGlobal(pnt);
        }

        /// <summary>
        /// Erases a portion of the RichTextBox's text, starting from 'at' pos
        /// and following 'length' characters.
        /// WARNING: changes current selection.
        /// </summary>
        public static void EraseSelectedText(this RichTextBox rtb, int at, int length)
        {
            SendMessage(rtb.Handle, EM_SETSEL, at, (IntPtr)at + length);
            SendMessage(rtb.Handle, EM_REPLACESEL, 0, IntPtr.Zero);
        }

        /// <summary>
        /// Replaces a portion of the RichTextBox's text, starting from 'at' pos
        /// and following 'length' characters. The 'newText' will be pasted instead.
        /// WARNING: changes current selection.
        /// </summary>
        public static void ReplaceSelectedText(this RichTextBox rtb, int at, int length, string newText)
        {
            IntPtr strPtr = Marshal.StringToHGlobalAuto(newText);
            SendMessage(rtb.Handle, EM_SETSEL, at, (IntPtr)at + length);
            SendMessage(rtb.Handle, EM_REPLACESEL, 0, strPtr);
            Marshal.FreeHGlobal(strPtr);
        }

        // Hack to get around the fact that the BeforeLabelEdit event provides no way to
        // let you change the text they're about to edit
        public static void SetTreeViewEditText(TreeView tree, string myText)
        {
            IntPtr editHandle = (IntPtr)SendMessage(tree.Handle, TVM_GETEDITCONTROL, 0, IntPtr.Zero);
            IntPtr strPtr = Marshal.StringToHGlobalAuto(myText);
            SendMessage(editHandle, WM_SETTEXT, 0, strPtr);
            Marshal.FreeHGlobal(strPtr);
        }

        // The PropertyGrid doesn't provide a settable SelectedTab property. Well, it does now.
        public static void SetSelectedTabInPropertyGrid(PropertyGrid propGrid, int selectedTabIndex)
        {
            Type type = propGrid.GetType();

            // Compatibility for derived classes
            if (type != typeof(PropertyGrid))
            {
                type = type.BaseType;
            }

            if ((selectedTabIndex < 0) || (selectedTabIndex >= propGrid.PropertyTabs.Count))
            {
                throw new ArgumentException("Invalid tab index to select: " + selectedTabIndex);
            }

            FieldInfo buttonsField = type.GetField("viewTabButtons", BindingFlags.NonPublic | BindingFlags.Instance);
            ToolStripButton[] viewTabButtons = (ToolStripButton[])buttonsField.GetValue(propGrid);
            ToolStripButton viewTabButton = viewTabButtons[selectedTabIndex];

            type.InvokeMember("OnViewTabButtonClick", BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.InvokeMethod, null, propGrid, new object[] { viewTabButton, EventArgs.Empty });
        }
    }
}
