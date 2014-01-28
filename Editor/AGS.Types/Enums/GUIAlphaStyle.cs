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
        [Description("Additive Opacity")]
        AdditiveOpacity = 1,
        [Description("Proper Alpha Blending")]
        MultiplyTranslucenceSrcBlend = 2
    }
}
