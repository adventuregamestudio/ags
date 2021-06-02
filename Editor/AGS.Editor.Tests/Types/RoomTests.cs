using NUnit.Framework;
using System;
using System.IO;
using System.Xml;

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

        [TestCase(1, 2, 1174750494, 320, 240, 5, 0, true, false, 1, RoomVolumeAdjustment.Normal, 1, 2, 3, 4, 2, "description1")]
        [TestCase(2, 1, 1174750495, 640, 480, 4, 1, false, true, 0, RoomVolumeAdjustment.Loud, 2, 3, 4, 5, 3, "description2")]
        public void DeserializesFromXml(int maskResolution, int backgroundCount, int gameId, int width, int height,
            int backgroundAnimationDelay, int playMusicOnRoomLoad, bool saveLoadEnabled, bool showPlayerCharacter,
            int playerCharacterView, RoomVolumeAdjustment musicVolumeAdjustment, int leftEdgeX, int rightEdgeX,
            int topEdgeY, int bottomEdgeY, int number, string description)
        {
            string xml = $@"
            <Room>
                <MaskResolution>{maskResolution}</MaskResolution>
                <BackgroundCount>{backgroundCount}</BackgroundCount>
                <GameID>{gameId}</GameID>
                <Width>{width}</Width>
                <Height>{height}</Height>
                <BackgroundAnimationDelay>{backgroundAnimationDelay}</BackgroundAnimationDelay>
                <PlayMusicOnRoomLoad>{playMusicOnRoomLoad}</PlayMusicOnRoomLoad>
                <SaveLoadEnabled>{saveLoadEnabled}</SaveLoadEnabled>
                <ShowPlayerCharacter>{showPlayerCharacter}</ShowPlayerCharacter>
                <PlayerCharacterView>{playerCharacterView}</PlayerCharacterView>
                <MusicVolumeAdjustment>{musicVolumeAdjustment}</MusicVolumeAdjustment>
                <LeftEdgeX>{leftEdgeX}</LeftEdgeX>
                <RightEdgeX>{rightEdgeX}</RightEdgeX>
                <TopEdgeY>{topEdgeY}</TopEdgeY>
                <BottomEdgeY>{bottomEdgeY}</BottomEdgeY>
                <Properties/>
                <Number>{number}</Number>
                <Description xml:space=""preserve"">{description}</Description>
                <Interactions>
                    <Event Index=""0"">room_LeftEdge</Event>
                    <Event Index=""1"" />
                    <Event Index=""2"" />
                    <Event Index=""3"" />
                    <Event Index=""4"" />
                    <Event Index=""5"" />
                    <Event Index=""6"" />
                    <Event Index=""7"" />
                    <Event Index=""8"" />
                </Interactions>
                <Messages/>
                <Objects/>
                <Hotspots/>
                <WalkableAreas/>
                <WalkBehinds/>
                <Regions/>
            </Room>";
            Directory.CreateDirectory(Path.Combine("Rooms", $"{number}"));
            File.WriteAllText(Path.Combine("Rooms", $"{number}", $"room{number}.asc"), "Test placeholder");
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            _room = new Room(doc.SelectSingleNode("Room"));

            Assert.That(_room.MaskResolution, Is.EqualTo(maskResolution));
            Assert.That(_room.BackgroundCount, Is.EqualTo(backgroundCount));
            Assert.That(_room.GameID, Is.EqualTo(gameId));
            Assert.That(_room.Width, Is.EqualTo(width));
            Assert.That(_room.Height, Is.EqualTo(height));
            Assert.That(_room.BackgroundAnimationDelay, Is.EqualTo(backgroundAnimationDelay));
            Assert.That(_room.PlayMusicOnRoomLoad, Is.EqualTo(playMusicOnRoomLoad));
            Assert.That(_room.SaveLoadEnabled, Is.EqualTo(saveLoadEnabled));
            Assert.That(_room.ShowPlayerCharacter, Is.EqualTo(showPlayerCharacter));
            Assert.That(_room.PlayerCharacterView, Is.EqualTo(playerCharacterView));
            Assert.That(_room.MusicVolumeAdjustment, Is.EqualTo(musicVolumeAdjustment));
            Assert.That(_room.LeftEdgeX, Is.EqualTo(leftEdgeX));
            Assert.That(_room.RightEdgeX, Is.EqualTo(rightEdgeX));
            Assert.That(_room.TopEdgeY, Is.EqualTo(topEdgeY));
            Assert.That(_room.BottomEdgeY, Is.EqualTo(bottomEdgeY));
            Assert.That(_room.Number, Is.EqualTo(number));
            Assert.That(_room.Description, Is.EqualTo(description));

            Assert.That(_room.Interactions.ScriptFunctionNames[0], Is.EqualTo("room_LeftEdge"));
            Assert.That(_room.Interactions.ScriptFunctionNames[1], Is.Null);
            Assert.That(_room.Interactions.ScriptFunctionNames[2], Is.Null);
            Assert.That(_room.Interactions.ScriptFunctionNames[3], Is.Null);
            Assert.That(_room.Interactions.ScriptFunctionNames[4], Is.Null);
            Assert.That(_room.Interactions.ScriptFunctionNames[5], Is.Null);
            Assert.That(_room.Interactions.ScriptFunctionNames[6], Is.Null);
            Assert.That(_room.Interactions.ScriptFunctionNames[7], Is.Null);
            Assert.That(_room.Interactions.ScriptFunctionNames[8], Is.Null);

            File.Delete("TestScript.asc");
        }

        [TestCase(1, 2, 1174750494, 320, 240, 5, 0, true, false, 1, RoomVolumeAdjustment.Normal, 1, 2, 3, 4, 2, "description1")]
        [TestCase(2, 1, 1174750495, 640, 480, 4, 1, false, true, 0, RoomVolumeAdjustment.Loud, 2, 3, 4, 5, 3, "description2")]
        public void SerializesToXml(int maskResolution, int backgroundCount, int gameId, int width, int height,
            int backgroundAnimationDelay, int playMusicOnRoomLoad, bool saveLoadEnabled, bool showPlayerCharacter,
            int playerCharacterView, RoomVolumeAdjustment musicVolumeAdjustment, int leftEdgeX, int rightEdgeX,
            int topEdgeY, int bottomEdgeY, int number, string description)
        {
            _room.MaskResolution = maskResolution;
            _room.BackgroundCount = backgroundCount;
            _room.GameID = gameId;
            _room.Width = width;
            _room.Height = height;
            _room.BackgroundAnimationDelay = backgroundAnimationDelay;
            _room.PlayMusicOnRoomLoad = playMusicOnRoomLoad;
            _room.SaveLoadEnabled = saveLoadEnabled;
            _room.ShowPlayerCharacter = showPlayerCharacter;
            _room.PlayerCharacterView = playerCharacterView;
            _room.MusicVolumeAdjustment = musicVolumeAdjustment;
            _room.LeftEdgeX = leftEdgeX;
            _room.RightEdgeX = rightEdgeX;
            _room.TopEdgeY = topEdgeY;
            _room.BottomEdgeY = bottomEdgeY;
            _room.Number = number;
            _room.Description = description;

            _room.Interactions.ScriptFunctionNames[0] = "room_LeftEdge";

            XmlDocument doc = _room.ToXmlDocument();

            Assert.That(doc.SelectSingleNode("/Room/MaskResolution").InnerText, Is.EqualTo(maskResolution.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/BackgroundCount").InnerText, Is.EqualTo(backgroundCount.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/GameID").InnerText, Is.EqualTo(gameId.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/Width").InnerText, Is.EqualTo(width.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/Height").InnerText, Is.EqualTo(height.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/BackgroundAnimationDelay").InnerText, Is.EqualTo(backgroundAnimationDelay.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/PlayMusicOnRoomLoad").InnerText, Is.EqualTo(playMusicOnRoomLoad.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/SaveLoadEnabled").InnerText, Is.EqualTo(saveLoadEnabled.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/ShowPlayerCharacter").InnerText, Is.EqualTo(showPlayerCharacter.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/PlayerCharacterView").InnerText, Is.EqualTo(playerCharacterView.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/MusicVolumeAdjustment").InnerText, Is.EqualTo(musicVolumeAdjustment.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/LeftEdgeX").InnerText, Is.EqualTo(leftEdgeX.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/RightEdgeX").InnerText, Is.EqualTo(rightEdgeX.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/TopEdgeY").InnerText, Is.EqualTo(topEdgeY.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/BottomEdgeY").InnerText, Is.EqualTo(bottomEdgeY.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/Number").InnerText, Is.EqualTo(number.ToString()));
            Assert.That(doc.SelectSingleNode("/Room/Description").InnerText, Is.EqualTo(description.ToString()));

            var interactions = doc.SelectSingleNode("/Room/Interactions").SelectNodes("Event");
            
            Assert.That(interactions[0].InnerText, Is.EqualTo("room_LeftEdge"));
            Assert.That(interactions[1].InnerText, Is.EqualTo(""));
            Assert.That(interactions[2].InnerText, Is.EqualTo(""));
            Assert.That(interactions[3].InnerText, Is.EqualTo(""));
            Assert.That(interactions[4].InnerText, Is.EqualTo(""));
            Assert.That(interactions[5].InnerText, Is.EqualTo(""));
            Assert.That(interactions[6].InnerText, Is.EqualTo(""));
            Assert.That(interactions[7].InnerText, Is.EqualTo(""));
            Assert.That(interactions[8].InnerText, Is.EqualTo(""));
        }
    }
}
