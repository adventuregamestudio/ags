namespace AGS.Types
{
    public interface ICompileMessage
    {
        string Message { get; }
        string ScriptName { get; }
        int LineNumber { get; }
    }
}
