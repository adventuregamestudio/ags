using System;

namespace AGS.Types
{
    public enum SpriteCompression
    {
        None,
        RLE,
        LZW,
        PNG, // this means DEFLATE
    }
}
