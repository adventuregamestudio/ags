using System;

namespace AGS.Editor
{
    /// <summary>
    /// A basic interface for reading and writing values into a configuration.
    /// This interface may be extended by a more specialized one, that lets
    /// read and write complex objects, if necessary.
    /// 
    /// TODO: consider separating config Reader and Writer interfaces,
    /// in case some class must be read-only.
    /// TODO: consider ColorTheme class implementing this? although ColorTheme's
    /// interface methods are slightly different, but the idea is similar
    /// to reading a config.
    /// </summary>
    public interface IObjectConfig
    {
        /// <summary>
        /// Returns a nested config object under the given key.
        /// </summary>
        IObjectConfig GetObject(string id);

        /// <summary>
        /// Returns a nested config object under the given key.
        /// Creates one if it does not exist.
        /// </summary>
        IObjectConfig GetOrAddObject(string id);

        int  GetInt(string id, int defValue);
        void SetInt(string id, int value);

        /// <summary>
        /// Loads this config from the file, overwriting all contents.
        /// </summary>
        bool LoadFromFile(string filepath);

        /// <summary>
        /// Saves this config to the file.
        /// </summary>
        void SaveToFile(string filepath);
    }
}
