using NUnit.Framework;

namespace AGS.Types
{
    [TestFixture]
    public class RoomMessageTests
    {
        private RoomMessage _roomMessage;

        [SetUp]
        public void SetUp()
        {
            _roomMessage = new RoomMessage(1);
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void IdSetFromConstructorIsValid(int id)
        {
            _roomMessage = new RoomMessage(id);
            Assert.That(_roomMessage.ID, Is.EqualTo(id));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsId(int id)
        {
            _roomMessage.ID = id;
            Assert.That(_roomMessage.ID, Is.EqualTo(id));
        }

        [TestCase("TestMessage")]
        [TestCase("")]
        public void GetsAndSetsText(string text)
        {
            _roomMessage.Text = text;
            Assert.That(_roomMessage.Text, Is.EqualTo(text));
        }

        [TestCase(true)]
        [TestCase(false)]
        public void GetsAndSetsShowAsSpeech(bool showAsSpeech)
        {
            _roomMessage.ShowAsSpeech = showAsSpeech;
            Assert.That(_roomMessage.ShowAsSpeech, Is.EqualTo(showAsSpeech));
        }

        [TestCase(1)]
        [TestCase(5)]
        public void GetsAndSetsCharacterId(int characterId)
        {
            Assume.That(characterId, Is.GreaterThanOrEqualTo(0));
            _roomMessage.CharacterID = characterId;
            Assert.That(_roomMessage.CharacterID, Is.EqualTo(characterId));
        }

        [TestCase(-1)]
        [TestCase(int.MinValue)]
        public void SetsCharacterIdTo0WhenValueIsLessThan0(int characterId)
        {
            Assume.That(characterId, Is.LessThan(0));
            _roomMessage.CharacterID = characterId;
            Assert.That(_roomMessage.CharacterID, Is.EqualTo(0));
        }

        [TestCase(true)]
        [TestCase(false)]
        public void GetsAndSetsDisplayNextMessageAfter(bool displayNextMessageAfter)
        {
            _roomMessage.DisplayNextMessageAfter = displayNextMessageAfter;
            Assert.That(_roomMessage.DisplayNextMessageAfter, Is.EqualTo(displayNextMessageAfter));
        }

        [TestCase(true)]
        [TestCase(false)]
        public void GetsAndSetsAutoRemoveAfterTime(bool autoRemoveAfterTime)
        {
            _roomMessage.AutoRemoveAfterTime = autoRemoveAfterTime;
            Assert.That(_roomMessage.AutoRemoveAfterTime, Is.EqualTo(autoRemoveAfterTime));
        }
    }
}
