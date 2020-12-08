using NUnit.Framework;
using System.Xml;

namespace AGS.Types
{
    [TestFixture]
    public class RoomWalkBehindTests
    {
        private RoomWalkBehind _roomWalkBehind;

        [SetUp]
        public void SetUp()
        {
            _roomWalkBehind = new RoomWalkBehind();
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsId(int id)
        {
            _roomWalkBehind.ID = id;
            Assert.That(_roomWalkBehind.ID, Is.EqualTo(id));
        }

        [TestCase(0, 0)]
        [TestCase(int.MinValue, 0)]
        [TestCase(5, 5)]
        [TestCase(int.MaxValue, int.MaxValue)]
        public void GetsAndSetsBaseline(int baseline, int expectedBaseline)
        {
            _roomWalkBehind.Baseline = baseline;
            Assert.That(_roomWalkBehind.Baseline, Is.EqualTo(expectedBaseline));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsPropertyGridTitle(int id)
        {
            _roomWalkBehind.ID = id;
            Assert.That(_roomWalkBehind.PropertyGridTitle, Is.EqualTo($"Walk-behind area ID {id}"));
        }

        [TestCase(230)]
        [TestCase(250)]
        public void DeserializesFromXml(int baseline)
        {
            string xml = $@"
            <RoomWalkBehind>
                <Baseline>{baseline}</Baseline>
            </RoomWalkBehind>";
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            _roomWalkBehind = new RoomWalkBehind(doc.SelectSingleNode("RoomWalkBehind"));

            Assert.That(_roomWalkBehind.Baseline, Is.EqualTo(baseline));
        }

        [TestCase(230)]
        [TestCase(250)]
        public void SerializesToXml(int baseline)
        {
            _roomWalkBehind.Baseline = baseline;
            XmlDocument doc = _roomWalkBehind.ToXmlDocument();

            Assert.That(doc.SelectSingleNode("/RoomWalkBehind/Baseline").InnerText, Is.EqualTo(baseline.ToString()));
        }
    }
}
