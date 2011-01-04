using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public enum SourceControlFileStatus
    {
        Invalid = -1,
        NotControlled = 0,
        Controlled = 1,
        CheckedOutByMe = 2,
        CheckedOutByOther = 4,
        ExclusiveCheckout = 8,
        CheckedOutByMultiple = 0x10,
        OutOfDate = 0x20,
        Deleted = 0x40,
        Locked = 0x80,
        Merged = 0x100,
        Shared = 0x200,
        Pinned = 0x400,
        Modified = 0x800,
        CheckedOutByCurrentUser = 0x1000,
        NeverMerge = 0x2000
    }
}
