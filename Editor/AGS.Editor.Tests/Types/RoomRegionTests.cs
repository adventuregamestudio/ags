using System;
using NUnit.Framework;

namespace AGS.Types
{
    [TestFixture]
    public class RoomRegionTests
    {
        private RoomRegion _roomRegion;

        [SetUp]
        public void SetUp()
        {
            _roomRegion = new RoomRegion();
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsId(int id)
        {
            _roomRegion.ID = id;
            Assert.That(_roomRegion.ID, Is.EqualTo(id));
        }

        [TestCase(false)]
        [TestCase(true)]
        public void GetsAndSetsUseColourTint(bool useColourTint)
        {
            _roomRegion.UseColourTint = useColourTint;
            Assert.That(_roomRegion.UseColourTint, Is.EqualTo(useColourTint));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsLightLevel(int lightLevel)
        {
            _roomRegion.LightLevel = lightLevel;
            Assert.That(_roomRegion.LightLevel, Is.EqualTo(lightLevel));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsRedTint(int redTint)
        {
            _roomRegion.RedTint = redTint;
            Assert.That(_roomRegion.RedTint, Is.EqualTo(redTint));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsGreenTint(int greenTint)
        {
            _roomRegion.GreenTint = greenTint;
            Assert.That(_roomRegion.GreenTint, Is.EqualTo(greenTint));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsBlueTint(int blueTint)
        {
            _roomRegion.BlueTint = blueTint;
            Assert.That(_roomRegion.BlueTint, Is.EqualTo(blueTint));
        }

        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsTintSaturation(int tintSaturation)
        {
            _roomRegion.TintSaturation = tintSaturation;
            Assert.That(_roomRegion.TintSaturation, Is.EqualTo(tintSaturation));
        }

        [Test]
        public void SetsTintSaturationTo0ThrowsArgumentOutOfRangeException()
        {
            Assert.Throws<ArgumentOutOfRangeException>(() => _roomRegion.TintSaturation = 0);
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsTintLuminance(int tintLuminance)
        {
            _roomRegion.TintLuminance = tintLuminance;
            Assert.That(_roomRegion.TintLuminance, Is.EqualTo(tintLuminance));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsPropertyGridTitle(int id)
        {
            _roomRegion.ID = id;
            Assert.That(_roomRegion.PropertyGridTitle, Is.EqualTo($"Region ID {id}"));
        }
    }
}
