using System;
using System.Text;
using System.Text.RegularExpressions;

namespace AGS.Editor.Utils
{

    public class ArgumentsBuilder
    {
        private bool _spacesBetweenFlagParam = false;
        private StringBuilder Cmd { get; } = new StringBuilder();

        private readonly Regex NeedQuotes = new Regex(@"^[a-z\\/:0-9\._\-+=]*$", RegexOptions.None);

        private readonly Regex AllowedUnquoted = new Regex(@"[|><\s,;""]+", RegexOptions.IgnoreCase);

        private void AppendAndFixQuotes(string unquotedText, bool quoteAlways = false)
        {
            if (string.IsNullOrEmpty(unquotedText))
                return;

            bool addQuotes = quoteAlways || NeedQuotes.IsMatch(unquotedText) || !AllowedUnquoted.IsMatch(unquotedText);

            if (addQuotes)
                Cmd.Append('"');

            int literalQuotes = 0;
            for (int i = 0; i < unquotedText.Length; i++)
            {
                if (unquotedText[i] == '"')
                {
                    literalQuotes++;
                }
            }
            if (literalQuotes > 0)
            {
                unquotedText = unquotedText.Replace("\\\"", "\\\\\"");
                unquotedText = unquotedText.Replace("\"", "\\\"");
            }

            Cmd.Append(unquotedText);

            if (addQuotes && unquotedText.EndsWith("\\", StringComparison.Ordinal))
            {
                Cmd.Append('\\');
            }

            if (addQuotes)
                Cmd.Append('"');
        }

        public ArgumentsBuilder(bool spacesBetweenFlagParam = false)
        {
            _spacesBetweenFlagParam = spacesBetweenFlagParam;
        }

        public void AppendFlag(string flagName)
        {
            if (string.IsNullOrEmpty(flagName))
                return;

            if (Cmd.Length != 0 && Cmd[Cmd.Length - 1] != ' ')
            {
                Cmd.Append(' ');
            }

            Cmd.Append(flagName);
        }

        public void AppendFlagAndParameter(string flagName, string parameter, bool quoteAlways = false)
        {
            if (string.IsNullOrEmpty(flagName) || string.IsNullOrEmpty(parameter))
                return;

            AppendFlag(flagName);
            if (_spacesBetweenFlagParam) Cmd.Append(' ');
            AppendAndFixQuotes(parameter, quoteAlways);
        }

        public override string ToString() => Cmd.ToString();
    }
}
