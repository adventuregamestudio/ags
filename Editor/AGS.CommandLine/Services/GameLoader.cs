using System;
using System.IO;
using System.Xml;
using AGS.Types;

namespace AGS.CommandLine.Services
{
    /// <summary>
    /// Loads an AGS game project from a Game.agf file using the AGS.Types API.
    /// Mirrors the pattern used by AGSEditor.LoadGameFromDisk.
    /// </summary>
    public class GameLoader
    {
        /// <summary>
        /// Load an AGS project from the given directory. Returns the populated Game object.
        /// Throws FileNotFoundException or InvalidDataException on failure.
        /// </summary>
        public Game Load(string projectPath)
        {
            if (string.IsNullOrEmpty(projectPath))
                throw new ArgumentException("projectPath must not be empty");

            string gameFile = Path.Combine(projectPath, "Game.agf");
            if (!File.Exists(gameFile))
                throw new FileNotFoundException(
                    string.Format("Game.agf not found in: {0}", projectPath), gameFile);

            XmlDocument doc = new XmlDocument();
            try
            {
                doc.Load(gameFile);
            }
            catch (XmlException ex)
            {
                throw new System.IO.InvalidDataException(
                    string.Format("Failed to parse Game.agf: {0}", ex.Message), ex);
            }

            if (doc.DocumentElement == null || doc.DocumentElement.Name != "AGSEditorDocument")
                throw new System.IO.InvalidDataException(
                    string.Format("Invalid Game.agf: root element must be 'AGSEditorDocument'"));


            // Read version attributes the same way AGSEditor does
            XmlElement root = doc.DocumentElement;
            string xmlVersion = root.Attributes["Version"] != null
                ? root.Attributes["Version"].InnerText : null;
            string editorVersion = root.Attributes["EditorVersion"] != null
                ? root.Attributes["EditorVersion"].InnerText : null;
            int? versionIndex = null;
            if (root.Attributes["VersionIndex"] != null)
            {
                int parsed;
                if (int.TryParse(root.Attributes["VersionIndex"].InnerText, out parsed))
                    versionIndex = parsed;
            }

            Game game = new Game();
            game.SavedXmlVersion = xmlVersion;
            game.SavedXmlEditorVersion = editorVersion;
            game.SavedXmlVersionIndex = versionIndex;

            // Game.FromXml takes the document-element (AGSEditorDocument), not the Game child node
            game.FromXml(root);

            game.DirectoryPath = projectPath;
            return game;
        }
    }
}
