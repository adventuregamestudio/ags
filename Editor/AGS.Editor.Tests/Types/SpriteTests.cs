using NUnit.Framework;
using System;
using System.Xml;

namespace AGS.Types
{
    [TestFixture]
    public class SpriteTests
    {
        private Sprite _sprite;

        [SetUp]
        public void SetUp()
        {
            _sprite = new Sprite(0, 50, 50);
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsNumber(int number)
        {
            _sprite.Number = number;
            Assert.That(_sprite.Number, Is.EqualTo(number));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsWidth(int width)
        {
            _sprite.Width = width;
            Assert.That(_sprite.Width, Is.EqualTo(width));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsHeight(int height)
        {
            _sprite.Height = height;
            Assert.That(_sprite.Height, Is.EqualTo(height));
        }

        [Test]
        public void GetsSizeOnDisk()
        {
            _sprite.ColorDepth = 8;
            Assert.That(_sprite.SizeOnDisk, Is.EqualTo("2 KB"));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsColorDepth(int colorDepth)
        {
            _sprite.ColorDepth = colorDepth;
            Assert.That(_sprite.ColorDepth, Is.EqualTo(colorDepth));
        }

        [TestCase(false)]
        [TestCase(true)]
        public void GetsAndSetsAlphaChannel(bool alphaChannel)
        {
            _sprite.AlphaChannel = alphaChannel;
            Assert.That(_sprite.AlphaChannel, Is.EqualTo(alphaChannel));
        }

        [TestCase(null)]
        [TestCase("")]
        [TestCase("sprite.bmp")]
        public void GetsAndSetsSourceFile(string sourceFile)
        {
            _sprite.SourceFile = sourceFile;
            Assert.That(_sprite.SourceFile, Is.EqualTo(sourceFile));
        }

        [TestCase(false)]
        [TestCase(true)]
        public void GetsAndSetsImportAlphaChannel(bool importAlphaChannel)
        {
            _sprite.ImportAlphaChannel = importAlphaChannel;
            Assert.That(_sprite.ImportAlphaChannel, Is.EqualTo(importAlphaChannel));
        }

        [TestCase(null)]
        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsColoursLockedToRoom(int? coloursLockedToRoom)
        {
            _sprite.ColoursLockedToRoom = coloursLockedToRoom;
            Assert.That(_sprite.ColoursLockedToRoom, Is.EqualTo(coloursLockedToRoom));

            if (coloursLockedToRoom == null)
                Assert.That(_sprite.ColoursLockedToRoomDescription, Is.EqualTo("(None)"));
            else
                Assert.That(_sprite.ColoursLockedToRoomDescription, Is.EqualTo(coloursLockedToRoom.ToString()));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsOffsetX(int offsetX)
        {
            _sprite.OffsetX = offsetX;
            Assert.That(_sprite.OffsetX, Is.EqualTo(offsetX));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsOffsetY(int offsetY)
        {
            _sprite.OffsetY = offsetY;
            Assert.That(_sprite.OffsetY, Is.EqualTo(offsetY));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsImportWidth(int importWidth)
        {
            _sprite.ImportWidth = importWidth;
            Assert.That(_sprite.ImportWidth, Is.EqualTo(importWidth));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsImportHeight(int importHeight)
        {
            _sprite.ImportHeight = importHeight;
            Assert.That(_sprite.ImportHeight, Is.EqualTo(importHeight));
        }

        [TestCase(false)]
        [TestCase(true)]
        public void GetsAndSetsImportAsTile(bool importAsTile)
        {
            _sprite.ImportAsTile = importAsTile;
            Assert.That(_sprite.ImportAsTile, Is.EqualTo(importAsTile));
        }

        [TestCase(0)]
        [TestCase(1)]
        [TestCase(int.MaxValue)]
        public void GetsAndSetsFrame(int frame)
        {
            _sprite.Frame = frame;
            Assert.That(_sprite.Frame, Is.EqualTo(frame));
        }

        [TestCase(SpriteImportTransparency.PaletteIndex0)]
        [TestCase(SpriteImportTransparency.TopLeft)]
        [TestCase(SpriteImportTransparency.BottomLeft)]
        [TestCase(SpriteImportTransparency.TopRight)]
        [TestCase(SpriteImportTransparency.BottomRight)]
        [TestCase(SpriteImportTransparency.LeaveAsIs)]
        [TestCase(SpriteImportTransparency.NoTransparency)]
        public void GetsAndSetsTransparentColour(SpriteImportTransparency transparentColour)
        {
            _sprite.TransparentColour = transparentColour;
            Assert.That(_sprite.TransparentColour, Is.EqualTo(transparentColour));
        }

        [TestCase(false)]
        [TestCase(true)]
        public void GetsAndSetsRemapToGamePalette(bool remapToGamePalette)
        {
            _sprite.RemapToGamePalette = remapToGamePalette;
            Assert.That(_sprite.RemapToGamePalette, Is.EqualTo(remapToGamePalette));
        }

        [TestCase(false)]
        [TestCase(true)]
        public void GetsAndSetsRemapToRoomPalette(bool remapToRoomPalette)
        {
            _sprite.RemapToRoomPalette = remapToRoomPalette;
            Assert.That(_sprite.RemapToRoomPalette, Is.EqualTo(remapToRoomPalette));
        }

        [TestCase(1, 25, 75, 8, false, 1, "sprite1.bmp", 1, 1, 1, 1, false, 1, false, false, SpriteImportTransparency.BottomLeft, false)]
        [TestCase(2, 75, 25, 32, true, 2, "sprite2.bmp", 2, 2, 1, 1, true, 2, true, true, SpriteImportTransparency.BottomRight, true)]
        public void DeserializesFromXml(int slot, int width, int height, int colorDepth, bool alphaChannel,
            int colorsLockedToRoom, string filename, int offsetX, int offsetY, int importHeight,
            int importWidth, bool importAsTile, int frame, bool remapToGamePalette, bool remapToRoomPalette,
            SpriteImportTransparency importMethod, bool importAlphaChannel)
        {
            string xml = $@"
            <Sprite Slot=""{slot}"" Width=""{width}"" Height=""{height}"" ColorDepth=""{colorDepth}"" Resolution=""LowRes"" AlphaChannel=""{alphaChannel}"" ColoursLockedToRoom=""{colorsLockedToRoom}"">
                <Source>
                    <FileName>{filename}</FileName>
                    <OffsetX>{offsetX}</OffsetX>
                    <OffsetY>{offsetY}</OffsetY>
                    <ImportHeight>{importHeight}</ImportHeight>
                    <ImportWidth>{importWidth}</ImportWidth>
                    <ImportAsTile>{importAsTile}</ImportAsTile>
                    <Frame>{frame}</Frame>
                    <RemapToGamePalette>{remapToGamePalette}</RemapToGamePalette>
                    <RemapToRoomPalette>{remapToRoomPalette}</RemapToRoomPalette>
                    <ImportMethod>{importMethod}</ImportMethod>
                    <ImportAlphaChannel>{importAlphaChannel}</ImportAlphaChannel>
                </Source>
            </Sprite>";
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            _sprite = new Sprite(doc.SelectSingleNode("Sprite"));

            Assert.That(_sprite.Number, Is.EqualTo(slot));
            Assert.That(_sprite.Width, Is.EqualTo(width));
            Assert.That(_sprite.Height, Is.EqualTo(height));
            Assert.That(_sprite.ColorDepth, Is.EqualTo(colorDepth));
            Assert.That(_sprite.AlphaChannel, Is.EqualTo(alphaChannel));
            Assert.That(_sprite.ColoursLockedToRoom, Is.EqualTo(colorsLockedToRoom));
            Assert.That(_sprite.SourceFile, Is.EqualTo(filename));
            Assert.That(_sprite.OffsetX, Is.EqualTo(offsetX));
            Assert.That(_sprite.OffsetY, Is.EqualTo(offsetY));
            Assert.That(_sprite.ImportHeight, Is.EqualTo(importHeight));
            Assert.That(_sprite.ImportWidth, Is.EqualTo(importWidth));
            Assert.That(_sprite.ImportAsTile, Is.EqualTo(importAsTile));
            Assert.That(_sprite.Frame, Is.EqualTo(frame));
            Assert.That(_sprite.RemapToGamePalette, Is.EqualTo(remapToGamePalette));
            Assert.That(_sprite.RemapToRoomPalette, Is.EqualTo(remapToRoomPalette));
            Assert.That(_sprite.TransparentColour, Is.EqualTo(importMethod));
            Assert.That(_sprite.ImportAlphaChannel, Is.EqualTo(importAlphaChannel));
        }

        [TestCase(1, 25, 75, 8, false, 1, "sprite1.bmp", 1, 1, 1, 1, false, 1, false, false, SpriteImportTransparency.BottomLeft, false)]
        [TestCase(2, 75, 25, 32, true, 2, "sprite2.bmp", 2, 2, 1, 1, true, 2, true, true, SpriteImportTransparency.BottomRight, true)]
        public void SerializesToXml(int slot, int width, int height, int colorDepth, bool alphaChannel,
            int colorsLockedToRoom, string filename, int offsetX, int offsetY, int importHeight,
            int importWidth, bool importAsTile, int frame, bool remapToGamePalette, bool remapToRoomPalette,
            SpriteImportTransparency importMethod, bool importAlphaChannel)
        {
            _sprite.Number = slot;
            _sprite.Width = width;
            _sprite.Height = height;
            _sprite.ColorDepth = colorDepth;
            _sprite.AlphaChannel = alphaChannel;
            _sprite.ColoursLockedToRoom = colorsLockedToRoom;
            _sprite.SourceFile = filename;
            _sprite.OffsetX = offsetX;
            _sprite.OffsetY = offsetY;
            _sprite.ImportHeight = importHeight;
            _sprite.ImportWidth = importWidth;
            _sprite.ImportAsTile = importAsTile;
            _sprite.Frame = frame;
            _sprite.RemapToGamePalette = remapToGamePalette;
            _sprite.RemapToRoomPalette = remapToRoomPalette;
            _sprite.TransparentColour = importMethod;
            _sprite.ImportAlphaChannel = importAlphaChannel;
            XmlDocument doc = _sprite.ToXmlDocument();

            Assert.That(doc.SelectSingleNode("/Sprite").Attributes["Slot"].InnerText, Is.EqualTo(_sprite.Number.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite").Attributes["Width"].InnerText, Is.EqualTo(_sprite.Width.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite").Attributes["Height"].InnerText, Is.EqualTo(_sprite.Height.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite").Attributes["ColorDepth"].InnerText, Is.EqualTo(_sprite.ColorDepth.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite").Attributes["Resolution"].InnerText, Is.EqualTo(SpriteImportResolution.Real.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite").Attributes["AlphaChannel"].InnerText, Is.EqualTo(_sprite.AlphaChannel.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite").Attributes["ColoursLockedToRoom"].InnerText, Is.EqualTo(_sprite.ColoursLockedToRoom.ToString()));

            Assert.That(doc.SelectSingleNode("/Sprite/Source/FileName").InnerText, Is.EqualTo(_sprite.SourceFile.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite/Source/OffsetX").InnerText, Is.EqualTo(_sprite.OffsetX.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite/Source/OffsetY").InnerText, Is.EqualTo(_sprite.OffsetY.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite/Source/ImportHeight").InnerText, Is.EqualTo(_sprite.ImportHeight.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite/Source/ImportWidth").InnerText, Is.EqualTo(_sprite.ImportWidth.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite/Source/ImportAsTile").InnerText, Is.EqualTo(_sprite.ImportAsTile.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite/Source/Frame").InnerText, Is.EqualTo(_sprite.Frame.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite/Source/RemapToGamePalette").InnerText, Is.EqualTo(_sprite.RemapToGamePalette.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite/Source/RemapToRoomPalette").InnerText, Is.EqualTo(_sprite.RemapToRoomPalette.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite/Source/ImportMethod").InnerText, Is.EqualTo(_sprite.TransparentColour.ToString()));
            Assert.That(doc.SelectSingleNode("/Sprite/Source/ImportAlphaChannel").InnerText, Is.EqualTo(_sprite.ImportAlphaChannel.ToString()));
        }
    }
}
