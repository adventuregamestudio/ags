using NUnit.Framework;

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
    }
}
