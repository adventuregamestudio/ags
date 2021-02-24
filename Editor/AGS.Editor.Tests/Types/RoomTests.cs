using NUnit.Framework;
using System;

namespace AGS.Types
{
    [TestFixture]
    public class RoomTests
    {
        private Room _room;

        [SetUp]
        public void SetUp()
        {
            _room = new Room(0);
        }

        [TestCase(true)]
        [TestCase(false)]
        public void GetsAndSetsModified(bool modified)
        {
            _room.Modified = modified;
            Assert.That(_room.Modified, Is.EqualTo(modified));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsMaskResolution(int maskResolution)
        {
            _room.MaskResolution = maskResolution;
            Assert.That(_room.MaskResolution, Is.EqualTo(maskResolution));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsBackgroundCount(int backgroundCount)
        {
            _room.BackgroundCount = backgroundCount;
            Assert.That(_room.BackgroundCount, Is.EqualTo(backgroundCount));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsGameID(int gameID)
        {
            _room.GameID = gameID;
            Assert.That(_room.GameID, Is.EqualTo(gameID));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsColorDepth(int colorDepth)
        {
            _room.ColorDepth = colorDepth;
            Assert.That(_room.ColorDepth, Is.EqualTo(colorDepth));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsWidth(int width)
        {
            _room.Width = width;
            Assert.That(_room.Width, Is.EqualTo(width));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsHeight(int height)
        {
            _room.Height = height;
            Assert.That(_room.Height, Is.EqualTo(height));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsBackgroundAnimationDelay(int backgroundAnimationDelay)
        {
            _room.BackgroundAnimationDelay = backgroundAnimationDelay;
            Assert.That(_room.BackgroundAnimationDelay, Is.EqualTo(backgroundAnimationDelay));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsPlayMusicOnRoomLoad(int playMusicOnRoomLoad)
        {
            _room.PlayMusicOnRoomLoad = playMusicOnRoomLoad;
            Assert.That(_room.PlayMusicOnRoomLoad, Is.EqualTo(playMusicOnRoomLoad));
        }

        [TestCase(false)]
        [TestCase(true)]
        public void GetsAndSetsSaveLoadEnabled(bool saveLoadEnabled)
        {
            _room.SaveLoadEnabled = saveLoadEnabled;
            Assert.That(_room.SaveLoadEnabled, Is.EqualTo(saveLoadEnabled));
        }

        [TestCase(false)]
        [TestCase(true)]
        public void GetsAndSetsShowPlayerCharacter(bool showPlayerCharacter)
        {
            _room.ShowPlayerCharacter = showPlayerCharacter;
            Assert.That(_room.ShowPlayerCharacter, Is.EqualTo(showPlayerCharacter));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsPlayerCharacterView(int playerCharacterView)
        {
            _room.PlayerCharacterView = playerCharacterView;
            Assert.That(_room.PlayerCharacterView, Is.EqualTo(playerCharacterView));
        }

        [TestCase(RoomVolumeAdjustment.Loud)]
        [TestCase(RoomVolumeAdjustment.Louder)]
        [TestCase(RoomVolumeAdjustment.Loudest)]
        [TestCase(RoomVolumeAdjustment.Normal)]
        [TestCase(RoomVolumeAdjustment.Quiet)]
        [TestCase(RoomVolumeAdjustment.Quieter)]
        [TestCase(RoomVolumeAdjustment.Quietest)]
        public void GetsAndSetsPlayerCharacterView(RoomVolumeAdjustment musicVolumeAdjustment)
        {
            _room.MusicVolumeAdjustment = musicVolumeAdjustment;
            Assert.That(_room.MusicVolumeAdjustment, Is.EqualTo(musicVolumeAdjustment));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsLeftEdgeX(int leftEdgeX)
        {
            _room.LeftEdgeX = leftEdgeX;
            Assert.That(_room.LeftEdgeX, Is.EqualTo(leftEdgeX));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsRightEdgeX(int rightEdgeX)
        {
            _room.RightEdgeX = rightEdgeX;
            Assert.That(_room.RightEdgeX, Is.EqualTo(rightEdgeX));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsTopEdgeY(int topEdgeY)
        {
            _room.TopEdgeY = topEdgeY;
            Assert.That(_room.TopEdgeY, Is.EqualTo(topEdgeY));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsBottomEdgeY(int bottomEdgeY)
        {
            _room.BottomEdgeY = bottomEdgeY;
            Assert.That(_room.BottomEdgeY, Is.EqualTo(bottomEdgeY));
        }

        [TestCase(1, RoomAreaMaskType.WalkBehinds, 1.00)]
        [TestCase(2, RoomAreaMaskType.WalkBehinds, 1.00)]
        [TestCase(1, RoomAreaMaskType.Hotspots, 1.00)]
        [TestCase(2, RoomAreaMaskType.Hotspots, 0.50)]
        [TestCase(3, RoomAreaMaskType.Hotspots, 0.33)]
        [TestCase(4, RoomAreaMaskType.Hotspots, 0.25)]
        [TestCase(1, RoomAreaMaskType.WalkableAreas, 1.00)]
        [TestCase(1, RoomAreaMaskType.Regions, 1.00)]
        public void GetsMaskScale(int maskResolution, RoomAreaMaskType mask, double expected)
        {
            _room.MaskResolution = maskResolution;
            Assert.That(_room.GetMaskScale(mask), Is.EqualTo(expected).Within(0.009));
        }

        [Test]
        public void GetsMaskScaleThrowsExceptionWithIllegalMask()
        {
            Assert.Throws<ArgumentException>(() => _room.GetMaskScale(RoomAreaMaskType.None));
        }
    }
}
