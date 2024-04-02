using System;

namespace AGS.Editor
{
    /// <summary>
    /// A script and header pair that exists as a file somewhere and should be added to the game project.
    /// </summary>
    struct ExistingScriptHeaderToAdd : IEquatable<ExistingScriptHeaderToAdd>
    {
        public ExistingFileToAdd Header;
        public ExistingFileToAdd Script;

        public bool Equals(ExistingScriptHeaderToAdd other)
        {
            return this.Header.Equals(other.Header) && this.Script.Equals(other.Script);
        }
    }
}
