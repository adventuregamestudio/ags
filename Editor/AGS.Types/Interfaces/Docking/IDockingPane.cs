
namespace AGS.Types
{
    public interface IDockingPane
    {
        void Refresh();
        IFloatWindow FloatWindow { get; }
    }
}
