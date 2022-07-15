using System;
using System.Drawing;

namespace AGS.Editor
{
    public static class MathExtra
    {
        public static T Clamp<T>(this T val, T min, T max) where T : IComparable<T>
        {
            if (val.CompareTo(min) < 0) return min;
            else if (val.CompareTo(max) > 0) return max;
            else return val;
        }

        public static Size SafeScale(this Size size, float scale)
        {
            return new Size(Math.Max(1, (int)(size.Width * scale)),
                     Math.Max(1, (int)(size.Height * scale)));
        }
    }
}
