using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// Extension methods for AGS.Types.View and accompanying classes.
    /// </summary>
    public static class ViewUtils
    {
        public static ViewFrame CloneFlipped(this ViewFrame frame, bool flipped)
        {
            ViewFrame newFrame = frame.Clone();
            newFrame.Flip = (frame.Flip & ~SpriteFlipStyle.Horizontal)
                | (flipped ? SpriteFlipStyle.Horizontal : SpriteFlipStyle.None);
            return newFrame;
        }

        public static void CloneInto(this ViewLoop loop, ViewLoop target, bool flipped)
        {
            target.Frames.Clear();
            target.RunNextLoop = loop.RunNextLoop;
            foreach (ViewFrame frame in loop.Frames)
            {
                target.Frames.Add(frame.CloneFlipped(flipped));
            }
        }
    }
}
