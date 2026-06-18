using System;
using System.ComponentModel;

namespace AGS.Types
{
    public enum TurnOrderPriority
    {
        [Description("Clockwise (default)")]
        Clockwise = 0,
        [Description("Counter-clockwise")]
        CounterClockwise = 1,
        Random = 2,
        [Description("Face Down (towards viewer)")]
        FaceDown = 3
    }
}
