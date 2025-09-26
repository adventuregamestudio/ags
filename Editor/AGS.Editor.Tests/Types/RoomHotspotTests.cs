﻿using System;
using System.Drawing;
using System.IO;
using System.Xml;
using AGS.Types;
using NSubstitute;
using NUnit.Framework;

namespace AGS.Types
{
    [TestFixture]
    public class RoomHotspotTests
    {
        private Room _room;
        private RoomHotspot _roomHotspot;

        [SetUp]
        public void SetUp()
        {
            _room = new Room(0);
            _roomHotspot = new RoomHotspot(_room);
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
            Assert.That(_room.Modified, Is.EqualTo(false));
            ((IChangeNotification)_roomHotspot).ItemModified();
            Assert.That(_room.Modified, Is.EqualTo(true));
        }

        [TestCase("Hotspot 1", "hHotspot1", 0, 0)]
        [TestCase("Hotspot 2", "hHotspot2", 5, 5)]
        public void DeserializesFromXml(string description, string name, int walkToX, int walkToY)
        {
            string xml = $@"
            <RoomHotspot>
              <Description xml:space=""preserve"">{description}</Description>
              <Name>{name}</Name>
              <WalkToPoint>{walkToX},{walkToY}</WalkToPoint>
              <Properties />
              <Interactions />
            </RoomHotspot>";
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            _roomHotspot = new RoomHotspot(_room, doc.SelectSingleNode("RoomHotspot"));

            Assert.That(_roomHotspot.Description, Is.EqualTo(description));
            Assert.That(_roomHotspot.Name, Is.EqualTo(name));
            Assert.That(_roomHotspot.WalkToPoint.X, Is.EqualTo(walkToX));
            Assert.That(_roomHotspot.WalkToPoint.Y, Is.EqualTo(walkToY));
        }

        [TestCase("Hotspot 1", "hHotspot1", 0, 0)]
        [TestCase("Hotspot 2", "hHotspot2", 5, 5)]
        public void SerializeToXml(string description, string name, int walkToX, int walkToY)
        {
            _roomHotspot.Description = description;
            _roomHotspot.Name = name;
            _roomHotspot.WalkToPoint = new Point(walkToX, walkToY);
            XmlDocument doc = _roomHotspot.ToXmlDocument();

            Assert.That(doc.SelectSingleNode("/RoomHotspot/Description").InnerText, Is.EqualTo(description));
            Assert.That(doc.SelectSingleNode("/RoomHotspot/Name").InnerText, Is.EqualTo(name));
            Assert.That(doc.SelectSingleNode("/RoomHotspot/WalkToPoint").InnerText, Is.EqualTo($"{walkToX},{walkToY}"));
        }
    }
}
