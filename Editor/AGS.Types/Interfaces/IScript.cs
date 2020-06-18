
namespace AGS.Types.Interfaces
{
    public interface IScript
    {
        string FileName { get; }
        string Text { get; }
        ScriptAutoCompleteData AutoCompleteData { get; }
    }
}
