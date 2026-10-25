using System;

namespace AGS.Types
{
    /// <summary>
    /// FontFields is a flag set that tells which of the font's properties
    /// are valid. Used by FontOverride class.
    /// NOTE: Font's ID and Filename properties are not here, because
    /// they are valid if they have a valid value (ID >= 0 and non-empty string).
    /// </summary>
    [Flags]
    public enum FontFields
    {
        None = 0,
        // Either Size or SizeMultiplier
        Size                    = 1 << 0,
        // Also assumes OutlineFont field
        OutlineStyle            = 1 << 1,
        AutoOutlineStyle        = 1 << 2,
        AutoOutlineThickness    = 1 << 3,
        VerticalOffset          = 1 << 4,
        LineSpacing             = 1 << 5,
        CharacterSpacing        = 1 << 6,
        TTFMetricsFixup         = 1 << 7,
        // Also assumes CustomHeight field
        HeightDefinition        = 1 << 8
    }

    /// <summary>
    /// FontOverride defines Font properties that should override another font.
    /// FontFields property specifies which properties exactly should be overridden.
    /// </summary>
    public class FontOverride
    {
        public FontOverride(Font font, FontFields fields)
        {
            Font = font;
            Fields = fields;
        }

        public FontFields Fields { get; set; }
        public Font Font { get; set; }
    }
}
