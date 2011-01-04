using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    public class SpriteFolder : ISpriteFolder
    {
        private string _name;
        private IList<Sprite> _sprites;
        private List<ISpriteFolder> _subFolders;

        public delegate void SpritesUpdatedHandler();
        /// <summary>
        /// Fired when an external client makes changes to the sprites
        /// (root sprite folder only)
        /// </summary>
        public event SpritesUpdatedHandler SpritesUpdated;

        public SpriteFolder(string name)
        {
            _name = name;
            _sprites = new List<Sprite>();
            _subFolders = new List<ISpriteFolder>();
        }

        public string Name
        {
            get { return _name; }
            set
            {
                if (value == null)
                {
                    throw new ArgumentNullException("Name");
                }
                _name = value;
            }
        }

        public IList<ISpriteFolder> SubFolders
        {
            get { return _subFolders; }
        }

        public IList<Sprite> Sprites
        {
            get { return _sprites; }
			set { _sprites = value; }
        }

		/// <summary>
		/// Finds the Sprite object for the specified sprite number.
		/// Returns null if the sprite is not found.
		/// </summary>
		/// <param name="spriteNumber">Sprite number to look for</param>
		/// <param name="recursive">Whether to also search sub-folders</param>
		public Sprite FindSpriteByID(int spriteNumber, bool recursive)
		{
			foreach (Sprite sprite in _sprites)
			{
				if (sprite.Number == spriteNumber)
				{
					return sprite;
				}
			}

			if (recursive)
			{
				foreach (SpriteFolder subFolder in this.SubFolders)
				{
					Sprite found = subFolder.FindSpriteByID(spriteNumber, recursive);
					if (found != null)
					{
						return found;
					}
				}
			}
			return null;
		}

		/// <summary>
		/// Finds the SpriteFolder object for the folder that contains the sprite.
		/// Returns null if the sprite is not found.
		/// </summary>
		/// <param name="spriteNumber">Sprite number to look for</param>
		public SpriteFolder FindFolderThatContainsSprite(int spriteNumber)
		{
			foreach (Sprite sprite in _sprites)
			{
				if (sprite.Number == spriteNumber)
				{
					return this;
				}
			}

			foreach (SpriteFolder subFolder in this.SubFolders)
			{
				SpriteFolder found = subFolder.FindFolderThatContainsSprite(spriteNumber);
				if (found != null)
				{
					return found;
				}
			}
			return null;
		}

        /// <summary>
        /// Causes the SpritesUpdated event to be fired. You should call this
        /// if you modify the sprites and need the Sprite Manager window
        /// to update to reflect the changes.
        /// Only call this on the Root sprite folder.
        /// </summary>
        public void NotifyClientsOfUpdate()
        {
            if (SpritesUpdated != null)
            {
                SpritesUpdated();
            }
        }

        public SpriteFolder(XmlNode node)
        {
            if (node.Name != "SpriteFolder")
            {
                throw new InvalidDataException("Incorrect node passed to SpriteFolder");
            }
            _name = node.Attributes["Name"].InnerText;
            _sprites = new List<Sprite>();
            _subFolders = new List<ISpriteFolder>();

            foreach (XmlNode childNode in SerializeUtils.GetChildNodes(node, "SubFolders"))
            {
                _subFolders.Add(new SpriteFolder(childNode));
            }

            foreach (XmlNode childNode in SerializeUtils.GetChildNodes(node, "Sprites"))
            {
                _sprites.Add(new Sprite(childNode));
            }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("SpriteFolder");
            writer.WriteAttributeString("Name", _name);

            writer.WriteStartElement("SubFolders");
            foreach (SpriteFolder folder in _subFolders)
            {
                folder.ToXml(writer);
            }
            writer.WriteEndElement();

            writer.WriteStartElement("Sprites");
            foreach (Sprite sprite in _sprites)
            {
                sprite.ToXml(writer);
            }
            writer.WriteEndElement();

            writer.WriteEndElement();
        }
    }
}
