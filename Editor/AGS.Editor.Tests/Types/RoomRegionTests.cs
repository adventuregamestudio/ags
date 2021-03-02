using System;
using System.Xml;
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

        [TestCase(false, 100, 30, 40, 50, 60, 70)]
        [TestCase(true, 90, 40, 50, 60, 70, 80)]
        public void DeserializesFromXml(bool useColourTint, int lightLevel, int redTint, int greenTint, int blueTint, int tintSaturation, int tintLuminance)
        {
            string xml = $@"
            <RoomRegion>
                <UseColourTint>{useColourTint}</UseColourTint>
                <LightLevel>{lightLevel}</LightLevel>
                <RedTint>{redTint}</RedTint>
                <GreenTint>{greenTint}</GreenTint>
                <BlueTint>{blueTint}</BlueTint>
                <TintSaturation>{tintSaturation}</TintSaturation>
                <TintLuminance>{tintLuminance}</TintLuminance>
                <Interactions/>
            </RoomRegion>";
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            _roomRegion = new RoomRegion(doc.SelectSingleNode("RoomRegion"));

            Assert.That(_roomRegion.UseColourTint, Is.EqualTo(useColourTint));
            Assert.That(_roomRegion.LightLevel, Is.EqualTo(lightLevel));
            Assert.That(_roomRegion.RedTint, Is.EqualTo(redTint));
            Assert.That(_roomRegion.GreenTint, Is.EqualTo(greenTint));
            Assert.That(_roomRegion.BlueTint, Is.EqualTo(blueTint));
            Assert.That(_roomRegion.TintSaturation, Is.EqualTo(tintSaturation));
            Assert.That(_roomRegion.TintLuminance, Is.EqualTo(tintLuminance));
        }

        [TestCase(false, 100, 30, 40, 50, 60, 70)]
        [TestCase(true, 90, 40, 50, 60, 70, 80)]
        public void SerializesToXml(bool useColourTint, int lightLevel, int redTint, int greenTint, int blueTint, int tintSaturation, int tintLuminance)
        {
            _roomRegion.UseColourTint = useColourTint;
            _roomRegion.LightLevel = lightLevel;
            _roomRegion.RedTint = redTint;
            _roomRegion.GreenTint = greenTint;
            _roomRegion.BlueTint = blueTint;
            _roomRegion.TintSaturation = tintSaturation;
            _roomRegion.TintLuminance = tintLuminance;
            XmlDocument doc = _roomRegion.ToXmlDocument();

            Assert.That(doc.SelectSingleNode("/RoomRegion/UseColourTint").InnerText, Is.EqualTo(useColourTint.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomRegion/LightLevel").InnerText, Is.EqualTo(lightLevel.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomRegion/RedTint").InnerText, Is.EqualTo(redTint.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomRegion/GreenTint").InnerText, Is.EqualTo(greenTint.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomRegion/BlueTint").InnerText, Is.EqualTo(blueTint.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomRegion/TintSaturation").InnerText, Is.EqualTo(tintSaturation.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomRegion/TintLuminance").InnerText, Is.EqualTo(tintLuminance.ToString()));
        }
    }
}
