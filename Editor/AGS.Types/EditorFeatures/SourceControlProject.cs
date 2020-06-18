
namespace AGS.Types
{
    public class SourceControlProject
    {
        public string AuxPath;
        public string ProjectName;

        public SourceControlProject(string auxPath, string projectName)
        {
            AuxPath = auxPath;
            ProjectName = projectName;
        }
    }
}
