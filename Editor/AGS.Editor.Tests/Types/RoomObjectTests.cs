using NSubstitute;
using NUnit.Framework;
using System.Xml;

namespace AGS.Types
{
    [TestFixture]
    public class RoomObjectTests
    {
        private IChangeNotification _changeNotification;
        private RoomObject _roomObject;

        [SetUp]
        public void SetUp()
        {
            _changeNotification = Substitute.For<IChangeNotification>();
            _roomObject = new RoomObject(_changeNotification);
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsId(int id)
        {
            _roomObject.ID = id;
            Assert.That(_roomObject.ID, Is.EqualTo(id));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsImage(int image)
        {
            Assume.That(image, Is.GreaterThanOrEqualTo(0));
            _roomObject.Image = image;
            Assert.That(_roomObject.Image, Is.EqualTo(image));
        }

        [TestCase(-1)]
        [TestCase(-5)]
        [TestCase(int.MinValue)]
        public void SettingImageWithNegativeValueWillDefaultTo0(int image)
        {
            Assume.That(image, Is.LessThan(0));
            _roomObject.Image = image;
            Assert.That(_roomObject.Image, Is.EqualTo(0));
        }

        [TestCase(true)]
        [TestCase(false)]
        public void GetsAndSetsVisible(bool visible)
        {
            _roomObject.Visible = visible;
            Assert.That(_roomObject.Visible, Is.EqualTo(visible));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        [TestCase(int.MinValue)]
        public void GetsAndSetsBaseline(int baseline)
        {
            _roomObject.Baseline = baseline;
            Assert.That(_roomObject.Baseline, Is.EqualTo(baseline));
        }

        [TestCase(true)]
        [TestCase(false)]
        public void GetsAndSetsClickable(bool clickable)
        {
            _roomObject.Clickable = clickable;
            Assert.That(_roomObject.Clickable, Is.EqualTo(clickable));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        [TestCase(int.MinValue)]
        public void GetsAndSetsEffectiveBaseline(int effectiveBaseline)
        {
            _roomObject.EffectiveBaseline = effectiveBaseline;
            Assert.That(_roomObject.EffectiveBaseline, Is.EqualTo(effectiveBaseline));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        [TestCase(int.MinValue)]
        public void GetsAndSetsStartX(int startX)
        {
            _roomObject.StartX = startX;
            Assert.That(_roomObject.StartX, Is.EqualTo(startX));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        [TestCase(int.MinValue)]
        public void GetsAndSetsStartY(int startY)
        {
            _roomObject.StartY = startY;
            Assert.That(_roomObject.StartY, Is.EqualTo(startY));
        }

        [TestCase("Test")]
        [TestCase("")]
        [TestCase(null)]
        public void GetsAndSetsDescription(string description)
        {
            _roomObject.Description = description;
            Assert.That(_roomObject.Description, Is.EqualTo(description));
        }

        //Name tests

        [TestCase(true)]
        [TestCase(false)]
        public void GetsAndSetsRoomAreaScaling(bool roomAreaScaling)
        {
            _roomObject.UseRoomAreaScaling = roomAreaScaling;
            Assert.That(_roomObject.UseRoomAreaScaling, Is.EqualTo(roomAreaScaling));
        }

        [TestCase(true)]
        [TestCase(false)]
        public void GetsAndSetsRoomAreaLighting(bool roomAreaLighting)
        {
            _roomObject.UseRoomAreaLighting = roomAreaLighting;
            Assert.That(_roomObject.UseRoomAreaLighting, Is.EqualTo(roomAreaLighting));
        }

        [Test]
        public void GetsPropertiesNotNull()
        {
            Assert.That(_roomObject.Properties, Is.Not.Null);
        }

        [Test]
        public void GetsInteractionsNotNull()
        {
            Assert.That(_roomObject.Interactions, Is.Not.Null);
        }

        [Test]
        public void ItemModified()
        {
            _changeNotification.DidNotReceive();
            ((IChangeNotification)_roomObject).ItemModified();
            _changeNotification.Received();
        }

        [TestCase(230, false, -1, false, 200, 100, "description1", "name1", false, false)]
        [TestCase(220, true, 0, true, 190, 90, "description2", "name2", true, true)]
        public void DeserializesFromXml(int image, bool visible, int baseline, bool clickable, int startX, int startY, string description, string name, bool useRoomAreaScaling, bool useRoomAreaLighting)
        {
            string xml = $@"
            <RoomObject>
                <Image>{image}</Image>
                <Visible>{visible}</Visible>
                <Baseline>{baseline}</Baseline>
                <Clickable>{clickable}</Clickable>
                <StartX>{startX}</StartX>
                <StartY>{startY}</StartY>
                <Description>{description}</Description>
                <Name>{name}</Name>
                <UseRoomAreaScaling>{useRoomAreaScaling}</UseRoomAreaScaling>
                <UseRoomAreaLighting>{useRoomAreaLighting}</UseRoomAreaLighting>
                <Properties />
                <Interactions/>
            </RoomObject>";
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            _roomObject = new RoomObject(_changeNotification, doc.SelectSingleNode("RoomObject"));

            Assert.That(_roomObject.Image, Is.EqualTo(image));
            Assert.That(_roomObject.Visible, Is.EqualTo(visible));
            Assert.That(_roomObject.Baseline, Is.EqualTo(baseline));
            Assert.That(_roomObject.Clickable, Is.EqualTo(clickable));
            Assert.That(_roomObject.StartX, Is.EqualTo(startX));
            Assert.That(_roomObject.StartY, Is.EqualTo(startY));
            Assert.That(_roomObject.Description, Is.EqualTo(description));
            Assert.That(_roomObject.Name, Is.EqualTo(name));
            Assert.That(_roomObject.UseRoomAreaScaling, Is.EqualTo(useRoomAreaScaling));
            Assert.That(_roomObject.UseRoomAreaLighting, Is.EqualTo(useRoomAreaLighting));
        }

        [TestCase(230, false, -1, false, 200, 100, "description1", "name1", false, false)]
        [TestCase(220, true, 0, true, 190, 90, "description2", "name2", true, true)]
        public void SerializesToXml(int image, bool visible, int baseline, bool clickable, int startX, int startY, string description, string name, bool useRoomAreaScaling, bool useRoomAreaLighting)
        {
            _roomObject.Image = image;
            _roomObject.Visible = visible;
            _roomObject.Baseline = baseline;
            _roomObject.Clickable = clickable;
            _roomObject.StartX = startX;
            _roomObject.StartY = startY;
            _roomObject.Description = description;
            _roomObject.Name = name;
            _roomObject.UseRoomAreaScaling = useRoomAreaScaling;
            _roomObject.UseRoomAreaLighting = useRoomAreaLighting;
            XmlDocument doc = _roomObject.ToXmlDocument();

            Assert.That(doc.SelectSingleNode("/RoomObject/Image").InnerText, Is.EqualTo(image.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomObject/Visible").InnerText, Is.EqualTo(visible.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomObject/Baseline").InnerText, Is.EqualTo(baseline.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomObject/Clickable").InnerText, Is.EqualTo(clickable.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomObject/StartX").InnerText, Is.EqualTo(startX.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomObject/StartY").InnerText, Is.EqualTo(startY.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomObject/Description").InnerText, Is.EqualTo(description.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomObject/Name").InnerText, Is.EqualTo(name.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomObject/UseRoomAreaScaling").InnerText, Is.EqualTo(useRoomAreaScaling.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomObject/UseRoomAreaLighting").InnerText, Is.EqualTo(useRoomAreaLighting.ToString()));
        }
    }
}
