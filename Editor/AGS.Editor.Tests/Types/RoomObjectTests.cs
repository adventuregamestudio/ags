using NSubstitute;
using NUnit.Framework;

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
    }
}
