using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;

namespace AGS.Types
{
    public enum GUIAlphaStyle
    {
        [Description("Classic")]
        Classic = 0,
        [Description("Additive Opacity, Copy Source Color")]
        AdditiveOpacity = 1,
        [Description("Multiplied Translucence, Blend Source Color")]
        MultiplyTranslucenceSrcBlend = 2
    }
}
