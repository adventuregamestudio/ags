using NUnit.Framework;
using System.Xml;

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

        [TestCase(4, "Test room message.", false, 1, true, false)]
        [TestCase(10, "Another test room message.", true, 5, false, true)]
        public void DeserializesFromXml(int id, string text, bool showAsSpeech, int characterId, bool displayNextMessageAfter, bool autoRemoveAfterTime)
        {
            string xml = $@"
            <RoomMessage>
                <Text xml:space=""preserve"">{text}</Text>
                <ShowAsSpeech>{showAsSpeech}</ShowAsSpeech>
                <CharacterID>{characterId}</CharacterID>
                <DisplayNextMessageAfter>{displayNextMessageAfter}</DisplayNextMessageAfter>
                <AutoRemoveAfterTime>{autoRemoveAfterTime}</AutoRemoveAfterTime>
            </RoomMessage>";
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            _roomMessage = new RoomMessage(id, doc.SelectSingleNode("RoomMessage"));

            Assert.That(_roomMessage.ID, Is.EqualTo(id));
            Assert.That(_roomMessage.Text, Is.EqualTo(text));
            Assert.That(_roomMessage.ShowAsSpeech, Is.EqualTo(showAsSpeech));
            Assert.That(_roomMessage.CharacterID, Is.EqualTo(characterId));
            Assert.That(_roomMessage.DisplayNextMessageAfter, Is.EqualTo(displayNextMessageAfter));
            Assert.That(_roomMessage.AutoRemoveAfterTime, Is.EqualTo(autoRemoveAfterTime));
        }

        [TestCase("Test room message.", false, 1, true, false)]
        [TestCase("Another test room message.", true, 5, false, true)]
        public void SerializesToXml(string text, bool showAsSpeech, int characterId, bool displayNextMessageAfter, bool autoRemoveAfterTime)
        {
            _roomMessage.Text = text;
            _roomMessage.ShowAsSpeech = showAsSpeech;
            _roomMessage.CharacterID = characterId;
            _roomMessage.DisplayNextMessageAfter = displayNextMessageAfter;
            _roomMessage.AutoRemoveAfterTime = autoRemoveAfterTime;
            XmlDocument doc = _roomMessage.ToXmlDocument();

            Assert.That(doc.SelectSingleNode("/RoomMessage/Text").InnerText, Is.EqualTo(text));
            Assert.That(doc.SelectSingleNode("/RoomMessage/ShowAsSpeech").InnerText, Is.EqualTo(showAsSpeech.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomMessage/CharacterID").InnerText, Is.EqualTo(characterId.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomMessage/DisplayNextMessageAfter").InnerText, Is.EqualTo(displayNextMessageAfter.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomMessage/AutoRemoveAfterTime").InnerText, Is.EqualTo(autoRemoveAfterTime.ToString()));
        }
    }
}
