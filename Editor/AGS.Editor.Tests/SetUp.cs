using System.IO;
using NUnit.Framework;

namespace AGS
{
    [SetUpFixture]
    public class SetUp
    {
        [OneTimeSetUp]
        public void OneTimeSetUp()
        {
            // Running directory is different from test assembly directory, and there are files there we need to
            // execute certain code. We set running directory to assembly directory, if that doesn't work we may
            // have to copy the files over to the original running directory
            Directory.SetCurrentDirectory(TestContext.CurrentContext.TestDirectory);
        }

        [OneTimeTearDown]
        public void OneTimeTearDown()
        {
        }
    }
}
