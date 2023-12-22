using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NUnit.Framework;

namespace AGS.Types
{
    [TestFixture]
    public class RuntimeSetupTests
    {
        private Settings _dummySettings;

        [SetUp]
        public void SetUp()
        {
            _dummySettings = new Settings();
        }

        [Test]
        public void Constructor_InitializesProperties()
        {
            var runtimeSetup = new RuntimeSetup(_dummySettings);

            Assert.IsNotNull(runtimeSetup);
            Assert.AreEqual(GraphicsDriver.D3D9, runtimeSetup.GraphicsDriver);
            Assert.IsFalse(runtimeSetup.Windowed);
            Assert.AreEqual(GameScaling.ProportionalStretch, runtimeSetup.FullscreenGameScaling);
        }

        [Test]
        public void SetDefaults_SetsDefaultValues()
        {
            var runtimeSetup = new RuntimeSetup(_dummySettings);

            runtimeSetup.SetDefaults();

            Assert.AreEqual(GraphicsDriver.D3D9, runtimeSetup.GraphicsDriver);
            Assert.IsFalse(runtimeSetup.Windowed);
            Assert.AreEqual(GameScaling.ProportionalStretch, runtimeSetup.FullscreenGameScaling);
        }

        [Test]
        public void GraphicsDriver_Setter_UpdatesGraphicsFilter()
        {
            var runtimeSetup = new RuntimeSetup(_dummySettings);

            runtimeSetup.GraphicsDriver = GraphicsDriver.OpenGL;

            Assert.IsNotNull(runtimeSetup.GraphicsFilter);
            Assert.AreEqual(GraphicsDriver.OpenGL, runtimeSetup.GraphicsDriver);
        }

        [Test]
        public void UseCustomSavePath_Setter_UpdatesCustomSavePath()
        {
            var runtimeSetup = new RuntimeSetup(_dummySettings);

            runtimeSetup.UseCustomSavePath = true;
            runtimeSetup.CustomSavePath = "CustomPath";

            Assert.IsTrue(runtimeSetup.UseCustomSavePath);
            Assert.AreEqual("CustomPath", runtimeSetup.CustomSavePath);
        }
    }
}
