using System;

namespace AGS.Types
{
    /* deprecated TextAlignment, LabelTextAlignment, ListBoxTextAlignment */
    [DeserializeConvertValue("BottomMiddle", "BottomCenter")]
    [DeserializeConvertValue("Centre", "MiddleCenter")]
    [DeserializeConvertValue("Centred", "MiddleCenter")]
    [DeserializeConvertValue("Left", "TopLeft")]
    [DeserializeConvertValue("Right", "TopRight")]
    [DeserializeConvertValue("TopMiddle", "TopCenter")]
    [Flags]
    public enum FrameAlignment
    {
        TopLeft         = 0x0001,
        TopCenter       = 0x0002,
        TopRight        = 0x0004,
        MiddleLeft      = 0x0008,
        MiddleCenter    = 0x0010,
        MiddleRight     = 0x0020,
        BottomLeft      = 0x0040,
        BottomCenter    = 0x0080,
        BottomRight     = 0x0100
    }

    /* deprecated LabelTextAlignment */
    [DeserializeConvertValue("TopLeft", "Left")]
    [DeserializeConvertValue("TopMiddle", "Center")]
    [DeserializeConvertValue("TopRight", "Right")]
    /* deprecated ListBoxTextAlignment */
    [DeserializeConvertValue("Centre", "Center")]
    public enum HorizontalAlignment
    {
        Left            = FrameAlignment.TopLeft,
        Center          = FrameAlignment.TopCenter,
        Right           = FrameAlignment.TopRight,
    }
}
