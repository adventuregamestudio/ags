using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    [DeserializeConvertValue("Left", "TopLeft")]
    [DeserializeConvertValue("Centre", "Centered")]
    [DeserializeConvertValue("Centred", "Centered")]
    [DeserializeConvertValue("Right", "TopRight")]
    public enum FrameAlignment
    {
        TopLeft      = AlignmentFlags.Left    | AlignmentFlags.Top,
        TopMiddle    = AlignmentFlags.HCenter | AlignmentFlags.Top,
        TopRight     = AlignmentFlags.Right   | AlignmentFlags.Top,
        MiddleLeft   = AlignmentFlags.Left    | AlignmentFlags.VCenter,
        Centered     = AlignmentFlags.HCenter | AlignmentFlags.VCenter,
        MiddleRight  = AlignmentFlags.Right   | AlignmentFlags.VCenter,
        BottomLeft   = AlignmentFlags.Left    | AlignmentFlags.Bottom,
        BottomMiddle = AlignmentFlags.HCenter | AlignmentFlags.Bottom,
        BottomRight  = AlignmentFlags.Right   | AlignmentFlags.Bottom
    }
}
