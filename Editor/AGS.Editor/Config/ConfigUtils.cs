using System;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// ConfigUtils is a collection of helper methods for uniform
    /// reading and writing of object configurations.
    /// </summary>
    public static class ConfigUtils
    {
        /// <summary>
        /// Reads Control's position from config.
        /// Applies safety position clamping relative to the Control's Parent.
        /// </summary>
        public static void ReadControlPosition(IObjectConfig config, string objectKey, Control control)
            => ReadControlPosition(config, objectKey, control, new Rectangle(control.Parent.Location, control.Parent.Size));

        /// <summary>
        /// Reads Form's position from config.
        /// Applies safety position clamping relative to the Form's Owner, or desktop.
        /// </summary>
        public static void ReadFormPosition(IObjectConfig config, string objectKey, Form form)
        {
            Rectangle parentRect;
            if (form.Owner != null)
                parentRect = form.Owner.Bounds;
            else
                parentRect = new Rectangle(0, 0, SystemInformation.VirtualScreen.Width, SystemInformation.VirtualScreen.Height);

            ReadControlPosition(config, objectKey, form, parentRect);
        }

        private static void ReadControlPosition(IObjectConfig config, string objectKey, Control control, Rectangle parentRect)
        {
            int left = config.GetInt($"{objectKey}/Left", control.Left);
            int top = config.GetInt($"{objectKey}/Top", control.Top);
            int width = config.GetInt($"{objectKey}/Width", control.Width);
            int height = config.GetInt($"{objectKey}/Height", control.Height);

            control.Width = MathExtra.Clamp(width, 10, parentRect.Width);
            control.Height = MathExtra.Clamp(height, 10, parentRect.Height);
            control.Left = MathExtra.Clamp(left, parentRect.Left, parentRect.Width - control.Width / 2);
            control.Top = MathExtra.Clamp(top, parentRect.Top, parentRect.Height - control.Height / 2);
        }

        public static void WriteControlPosition(IObjectConfig config, string objectKey, Control control)
        {
            config.SetInt($"{objectKey}/Left", control.Left);
            config.SetInt($"{objectKey}/Top", control.Top);
            config.SetInt($"{objectKey}/Width", control.Width);
            config.SetInt($"{objectKey}/Height", control.Height);
        }

        public static void WriteFormPosition(IObjectConfig config, string objectKey, Form form)
            => WriteControlPosition(config, objectKey, form);
    }
}
