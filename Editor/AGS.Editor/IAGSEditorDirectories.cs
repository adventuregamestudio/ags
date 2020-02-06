namespace AGS.Editor
{
    /// <summary>
    /// All important editor directories
    /// </summary>
    public interface IAGSEditorDirectories
    {
        string BaseGameFileName { get; }
        string EditorDirectory { get; }
        string GameDirectory { get; }
        string LocalAppData { get; }
        string TemplatesDirectory { get; }
        string UserTemplatesDirectory { get; }
    }
}
