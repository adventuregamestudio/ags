using NUnit.Framework;

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
    }
}
