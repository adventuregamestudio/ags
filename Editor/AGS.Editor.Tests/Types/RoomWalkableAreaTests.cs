using NUnit.Framework;
using System.Xml;

namespace AGS.Types
{
    [TestFixture]
    public class RoomWalkableAreaTests
    {
        private RoomWalkableArea _roomWalkAbleArea;

        [SetUp]
        public void SetUp()
        {
            _roomWalkAbleArea = new RoomWalkableArea();
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsId(int id)
        {
            _roomWalkAbleArea.ID = id;
            Assert.That(_roomWalkAbleArea.ID, Is.EqualTo(id));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsAreaSpecificView(int areaSpecificView)
        {
            _roomWalkAbleArea.AreaSpecificView = areaSpecificView;
            Assert.That(_roomWalkAbleArea.AreaSpecificView, Is.EqualTo(areaSpecificView));
        }

        [TestCase(false)]
        [TestCase(true)]
        public void GetsAndSetsUseContinuousScaling(bool useContinuousScaling)
        {
            _roomWalkAbleArea.UseContinuousScaling = useContinuousScaling;
            Assert.That(_roomWalkAbleArea.UseContinuousScaling, Is.EqualTo(useContinuousScaling));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsScalingLevel(int scalingLevel)
        {
            _roomWalkAbleArea.ScalingLevel = scalingLevel;
            Assert.That(_roomWalkAbleArea.ScalingLevel, Is.EqualTo(scalingLevel));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsMinScalingLevel(int minScalingLevel)
        {
            _roomWalkAbleArea.MinScalingLevel = minScalingLevel;
            Assert.That(_roomWalkAbleArea.MinScalingLevel, Is.EqualTo(minScalingLevel));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsMaxScalingLevel(int maxScalingLevel)
        {
            _roomWalkAbleArea.MaxScalingLevel = maxScalingLevel;
            Assert.That(_roomWalkAbleArea.MaxScalingLevel, Is.EqualTo(maxScalingLevel));
        }

        [TestCase(0)]
        [TestCase(5)]
        [TestCase(int.MaxValue)]
        public void GetsPropertyGridTitle(int id)
        {
            _roomWalkAbleArea.ID = id;
            Assert.That(_roomWalkAbleArea.PropertyGridTitle, Is.EqualTo($"Walkable area ID {id}"));
        }

        [TestCase(5, false, 50, 60, 70)]
        [TestCase(3, true, 40, 50, 60)]
        public void DeserializesFromXml(int areaSpecificView, bool userContinousScaling, int scalingLevel, int minScalingLevel, int maxScalingLevel)
        {
            string xml = $@"
            <RoomWalkableArea>
                <AreaSpecificView>{areaSpecificView}</AreaSpecificView>
                <UseContinuousScaling>{userContinousScaling}</UseContinuousScaling>
                <ScalingLevel>{scalingLevel}</ScalingLevel>
                <MinScalingLevel>{minScalingLevel}</MinScalingLevel>
                <MaxScalingLevel>{maxScalingLevel}</MaxScalingLevel>
            </RoomWalkableArea>";
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            _roomWalkAbleArea = new RoomWalkableArea(doc.SelectSingleNode("RoomWalkableArea"));

            Assert.That(_roomWalkAbleArea.AreaSpecificView, Is.EqualTo(areaSpecificView));
            Assert.That(_roomWalkAbleArea.UseContinuousScaling, Is.EqualTo(userContinousScaling));
            Assert.That(_roomWalkAbleArea.ScalingLevel, Is.EqualTo(scalingLevel));
            Assert.That(_roomWalkAbleArea.MinScalingLevel, Is.EqualTo(minScalingLevel));
            Assert.That(_roomWalkAbleArea.MaxScalingLevel, Is.EqualTo(maxScalingLevel));
        }

        [TestCase(5, false, 50, 60, 70)]
        [TestCase(3, true, 40, 50, 60)]
        public void SerializesToXml(int areaSpecificView, bool userContinousScaling, int scalingLevel, int minScalingLevel, int maxScalingLevel)
        {
            _roomWalkAbleArea.AreaSpecificView = areaSpecificView;
            _roomWalkAbleArea.UseContinuousScaling = userContinousScaling;
            _roomWalkAbleArea.ScalingLevel = scalingLevel;
            _roomWalkAbleArea.MinScalingLevel = minScalingLevel;
            _roomWalkAbleArea.MaxScalingLevel = maxScalingLevel;
            XmlDocument doc = _roomWalkAbleArea.ToXmlDocument();

            Assert.That(doc.SelectSingleNode("/RoomWalkableArea/AreaSpecificView").InnerText, Is.EqualTo(areaSpecificView.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomWalkableArea/UseContinuousScaling").InnerText, Is.EqualTo(userContinousScaling.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomWalkableArea/ScalingLevel").InnerText, Is.EqualTo(scalingLevel.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomWalkableArea/MinScalingLevel").InnerText, Is.EqualTo(minScalingLevel.ToString()));
            Assert.That(doc.SelectSingleNode("/RoomWalkableArea/MaxScalingLevel").InnerText, Is.EqualTo(maxScalingLevel.ToString()));
        }
    }
}
