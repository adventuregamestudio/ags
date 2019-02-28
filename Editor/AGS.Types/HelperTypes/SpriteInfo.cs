using System;

namespace AGS.Types
{
    /// <summary>
    /// Struct for passing basic information about a sprite.
    /// </summary>
    public class SpriteInfo
    {
        public int Width;
        public int Height;
        public SpriteImportResolution Resolution;

        public SpriteInfo(int width, int height, SpriteImportResolution res)
        {
            Width = width;
            Height = height;
            Resolution = res;
        }
    };
}
