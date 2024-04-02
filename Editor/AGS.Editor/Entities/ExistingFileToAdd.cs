using System;

namespace AGS.Editor
{
    /// <summary>
    /// A file that should be added to the game project, represented by its source (from anywhere) and destination in the game project.
    /// </summary>
    struct ExistingFileToAdd : IEquatable<ExistingFileToAdd>
    {
        public string SrcFileName;
        public string DstFileName;

        public bool Equals(ExistingFileToAdd other)
        {
            return this.SrcFileName == other.SrcFileName && this.DstFileName == other.DstFileName;
        }
    }
}
