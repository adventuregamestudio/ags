using NUnit.Framework;

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
            Assert.That(_unloadedRoom.UserFileName, Is.EqualTo($"room{number}.crm.user"));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsScriptFileName(int number)
        {
            _unloadedRoom.Number = number;
            Assert.That(_unloadedRoom.ScriptFileName, Is.EqualTo($"room{number}.asc"));
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
    }
}
