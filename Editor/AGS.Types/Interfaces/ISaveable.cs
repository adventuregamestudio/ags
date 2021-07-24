using System;

namespace AGS.Types.Interfaces
{
    public interface ISaveable
    {
        bool IsBeingSaved { get; }
        DateTime LastSavedAt { get; }
    }
}
