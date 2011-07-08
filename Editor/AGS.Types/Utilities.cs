using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Types
{
    public static class Utilities
    {
        public static int GetGameResolutionWidth(GameResolutions resolution)
        {
            switch (resolution)
            {
                case GameResolutions.R320x200:
                case GameResolutions.R320x240:
                    return 320;
                case GameResolutions.R640x400:
                case GameResolutions.R640x480:
                    return 640;
                case GameResolutions.R800x600:
                    return 800;
                case GameResolutions.R1024x768:
                    return 1024;
            }
            throw new InvalidDataException("Invalid game resolution: " + resolution.ToString());
        }

        public static int GetGameResolutionHeight(GameResolutions resolution)
        {
            switch (resolution)
            {
                case GameResolutions.R320x200:
                    return 200;
                case GameResolutions.R640x400:
                    return 400;
                case GameResolutions.R320x240:
                    return 240;
                case GameResolutions.R640x480:
                    return 480;
                case GameResolutions.R800x600:
                    return 600;
                case GameResolutions.R1024x768:
                    return 768;
            }
            throw new InvalidDataException("Invalid game resolution: " + resolution.ToString());
        }

        public static string RemoveInvalidCharactersFromScriptName(string name)
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < name.Length; i++)
            {
                if ((IsEnglishLetterOrDigit(name[i])) || (name[i] == '_'))
                {
                    sb.Append(name[i]);
                }
                else
                {
                    sb.Append('_');
                }
            }
            return sb.ToString();
        }

        private static bool IsEnglishLetterOrDigit(char theChar)
        {
            if ((theChar >= 'A') && (theChar <= 'Z')) return true;
            if ((theChar >= 'a') && (theChar <= 'z')) return true;
            if ((theChar >= '0') && (theChar <= '9')) return true;
            return false;
        }

        public static string ValidateScriptName(string name, int truncateToLength)
        {
            for (int i = 0; i < name.Length; i++)
            {
                if ((i == 0) && (Char.IsDigit(name[i])))
                {
                    throw new InvalidDataException("Script name cannot start with number: " + name);
                }
                if ((!IsEnglishLetterOrDigit(name[i])) && (name[i] != '_'))
                {
                    throw new InvalidDataException("Invalid script name; name must only include letters, numbers and underscores: " + name);
                }
            }
            if (name.Length > truncateToLength)
            {
                return name.Substring(0, truncateToLength);
            }
            return name;
        }

        public static string ValidateScriptName(string name)
        {
            return ValidateScriptName(name, 99999);
        }
    }
}
