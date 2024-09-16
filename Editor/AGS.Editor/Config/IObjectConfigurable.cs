using System;

namespace AGS.Editor
{
    /// <summary>
    /// A trivial interface for an object that may be configured using IObjectConfig.
    /// </summary>
    public interface IObjectConfigurable
    {
        void ReadConfig(IObjectConfig config);
        void WriteConfig(IObjectConfig config);
    }
}
