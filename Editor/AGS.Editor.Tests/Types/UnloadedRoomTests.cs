using NUnit.Framework;
using System.IO;
using System.Xml;

namespace AGS.Types
{
    [TestFixture]
    public class UnloadedRoomTests
    {
        private UnloadedRoom _unloadedRoom;

        [SetUp]
        public void SetUp()
        {
            _unloadedRoom = new UnloadedRoom(0);
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void IdSetFromConstructorIsValid(int number)
        {
            _unloadedRoom = new UnloadedRoom(number);
            Assert.That(_unloadedRoom.Number, Is.EqualTo(number));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsNumber(int number)
        {
            _unloadedRoom.Number = number;
            Assert.That(_unloadedRoom.Number, Is.EqualTo(number));
        }

        [TestCase("")]
        [TestCase("Test")]
        public void GetsAndSetsDescription(string description)
        {
            _unloadedRoom.Description = description;
            Assert.That(_unloadedRoom.Description, Is.EqualTo(description));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsFileName(int number)
        {
            _unloadedRoom.Number = number;
            Assert.That(_unloadedRoom.FileName, Is.EqualTo($"room{number}.crm"));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsUserFileName(int number)
        {
            _unloadedRoom.Number = number;
            Assert.That(_unloadedRoom.UserFileName, Is.EqualTo(Path.Combine(UnloadedRoom.ROOM_DIRECTORY, $"{number}", $"room{number}.crm.user")));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsScriptFileName(int number)
        {
            _unloadedRoom.Number = number;
            Assert.That(_unloadedRoom.ScriptFileName, Is.EqualTo(Path.Combine(UnloadedRoom.ROOM_DIRECTORY, $"{number}", $"room{number}.asc")));
        }

        [TestCase("test1file", "test1", false)]
        [TestCase("test2file", "test2", true)]
        public void GetsAndSetsScript(string fileName, string text, bool isHeader)
        {
            Script expected = new Script(fileName, text, isHeader);
            _unloadedRoom.Script = expected;
            Assert.That(_unloadedRoom.Script, Is.EqualTo(expected));
        }

        [Test]
        public void WillUnloadScript()
        {
            _unloadedRoom.Script = new Script("dummy", "dummy", false);
            _unloadedRoom.Script = null;
            Assert.That(_unloadedRoom.Script, Is.Null);
        }

        [TestCase(1, "description1")]
        [TestCase(2, "description2")]
        public void DeserializesFromXml(int number, string description)
        {
            string xml = $@"
            <UnloadedRoom>
                <Number>{number}</Number>
                <Description xml:space=""preserve"">{description}</Description>
            </UnloadedRoom>";
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            _unloadedRoom = new UnloadedRoom(doc.SelectSingleNode("UnloadedRoom"));

            Assert.That(_unloadedRoom.Number, Is.EqualTo(number));
            Assert.That(_unloadedRoom.Description, Is.EqualTo(description));
        }

        [TestCase(1, "description1")]
        [TestCase(2, "description2")]
        public void SerializesToXml(int number, string description)
        {
            _unloadedRoom.Number = number;
            _unloadedRoom.Description = description;
            XmlDocument doc = _unloadedRoom.ToXmlDocument();

            Assert.That(doc.SelectSingleNode("/UnloadedRoom/Number").InnerText, Is.EqualTo(number.ToString()));
            Assert.That(doc.SelectSingleNode("/UnloadedRoom/Description").InnerText, Is.EqualTo(description.ToString()));
        }
    }
}
