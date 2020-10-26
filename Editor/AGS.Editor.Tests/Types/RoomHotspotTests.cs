using System;
using System.Drawing;
using AGS.Types;
using NSubstitute;
using NUnit.Framework;

namespace AGS.Types
{
    [TestFixture]
    public class RoomHotspotTests
    {
        private IChangeNotification _changeNotification;
        private RoomHotspot _roomHotspot;

        [SetUp]
        public void SetUp()
        {
            _changeNotification = Substitute.For<IChangeNotification>();
            _roomHotspot = new RoomHotspot(_changeNotification);
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsId(int id)
        {
            _roomHotspot.ID = id;
            Assert.That(_roomHotspot.ID, Is.EqualTo(id));
        }

        [TestCase("")]
        [TestCase("TestDescription")]
        public void GetsAndSetsDescription(string description)
        {
            _roomHotspot.Description = description;
            Assert.That(_roomHotspot.Description, Is.EqualTo(description));
        }

        [TestCase("")]
        [TestCase("TestName")]
        public void GetsAndSetsName(string name)
        {
            _roomHotspot.Name = name;
            Assert.That(_roomHotspot.Name, Is.EqualTo(name));
        }

        [TestCase("6TestName")]
        [TestCase("Test#Name")]
        public void SetNameThrowsInvalidDataExceptionWhenNameIsInvalid(string name)
        {
            Assert.Throws<InvalidDataException>(() => _roomHotspot.Name = name);
        }

        [TestCase(0, 0)]
        [TestCase(5, 5)]
        [TestCase(int.MaxValue, int.MaxValue)]
        public void GetsAndSetsWalkToPoint(int x, int y)
        {
            Point walkToPoint = new Point { X = x, Y = y };
            _roomHotspot.WalkToPoint = walkToPoint;
            Assert.That(_roomHotspot.WalkToPoint.X, Is.EqualTo(walkToPoint.X));
            Assert.That(_roomHotspot.WalkToPoint.Y, Is.EqualTo(walkToPoint.Y));
        }

        [TestCase("TestName", 0)]
        [TestCase("TestName", 5)]
        [TestCase("TestName", int.MaxValue)]
        public void GetsPropertyGridTitle(string name, int id)
        {
            _roomHotspot.Name = name;
            _roomHotspot.ID = id;
            Assert.That(_roomHotspot.PropertyGridTitle, Is.EqualTo($"{name} (Hotspot; ID {id})"));
        }

        [Test]
        public void GetsPropertiesNotNull()
        {
            Assert.That(_roomHotspot.Properties, Is.Not.Null);
        }

        [Test]
        public void GetsInteractionsNotNull()
        {
            Assert.That(_roomHotspot.Interactions, Is.Not.Null);
        }

        [Test]
        public void ItemModified()
        {
            _changeNotification.DidNotReceive();
            ((IChangeNotification)_roomHotspot).ItemModified();
            _changeNotification.Received();
        }
    }
}
