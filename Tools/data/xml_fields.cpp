//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "data/xml_fields.h"
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>

using namespace AGS::Common;

namespace AGS
{
namespace AGF
{

// Returns the text without leading and trailing whitespace
static String Trimmed(const char *text)
{
    String s = text;
    s.Trim();
    return s;
}

// Splits "a,b,c" into its parts
static std::vector<String> SplitCommas(const char *text)
{
    return String(text).Split(',');
}

// Like C# Int32.Parse: optional whitespace and sign around decimal digits, no overflow
static bool ParseInt(const String &str, int &value)
{
    const String s = Trimmed(str.GetCStr());
    if (s.IsEmpty())
        return false;
    char *end = nullptr;
    errno = 0;
    const long v = std::strtol(s.GetCStr(), &end, 10);
    if (*end != 0 || errno == ERANGE || v < INT_MIN || v > INT_MAX)
        return false;
    value = static_cast<int>(v);
    return true;
}

bool ParseXmlValue(const char *text, int &value)
{
    return ParseInt(text, value);
}

bool ParseXmlValue(const char *text, bool &value)
{
    // like C# Convert.ToBoolean
    const String s = Trimmed(text);
    if (s.CompareNoCase("True") == 0)
        value = true;
    else if (s.CompareNoCase("False") == 0)
        value = false;
    else
        return false;
    return true;
}

bool ParseXmlValue(const char *text, float &value)
{
    // the editor writes floats with the invariant culture; the tools run with the "C" locale
    const String s = Trimmed(text);
    if (s.IsEmpty())
        return false;
    char *end = nullptr;
    const float v = std::strtof(s.GetCStr(), &end);
    if (*end != 0)
        return false;
    value = v;
    return true;
}

bool ParseXmlValue(const char *text, String &value)
{
    value = text;
    return true;
}

bool ParseXmlValue(const char *text, Point &value)
{
    const auto parts = SplitCommas(text);
    Point pt;
    if (parts.size() < 2 || !ParseInt(parts[0], pt.X) || !ParseInt(parts[1], pt.Y))
        return false;
    value = pt;
    return true;
}

bool ParseXmlValue(const char *text, XmlRect &value)
{
    const auto parts = SplitCommas(text);
    XmlRect r;
    if (parts.size() < 4 || !ParseInt(parts[0], r.X) || !ParseInt(parts[1], r.Y) || !ParseInt(parts[2], r.Width) ||
        !ParseInt(parts[3], r.Height))
        return false;
    value = r;
    return true;
}

bool ParseXmlEnum(const char *text, const char *const names[], size_t count, int &value)
{
    // like C# Enum.Parse: the exact name, or the number
    const String s = Trimmed(text);
    for (size_t i = 0; i < count; ++i)
    {
        if (s.Compare(names[i]) == 0)
        {
            value = static_cast<int>(i);
            return true;
        }
    }
    return ParseInt(s, value);
}

int CompareXmlVersions(const char *a, const char *b)
{
    int va[4] = {}, vb[4] = {};
    std::sscanf(a, "%d.%d.%d.%d", &va[0], &va[1], &va[2], &va[3]);
    std::sscanf(b, "%d.%d.%d.%d", &vb[0], &vb[1], &vb[2], &vb[3]);
    for (int i = 0; i < 4; ++i)
    {
        if (va[i] != vb[i])
            return va[i] < vb[i] ? -1 : 1;
    }
    return 0;
}

namespace XmlDetail
{

const char *ElementText(const tinyxml2::XMLElement *elem)
{
    const char *text = elem->GetText();
    return text ? text : "";
}

HError InvalidValue(const tinyxml2::XMLElement *elem)
{
    return new Error(String::FromFormat("Invalid value \"%s\" of <%s> at line %d", ElementText(elem), elem->Name(),
                                        elem->GetLineNum()));
}

HError UnknownElement(const tinyxml2::XMLElement *parent, const tinyxml2::XMLElement *elem)
{
    return new Error(String::FromFormat("Unknown element <%s> in <%s> at line %d; this project may need a newer "
                                        "version of the tools",
                                        elem->Name(), parent->Name(), elem->GetLineNum()));
}

} // namespace XmlDetail

} // namespace AGF
} // namespace AGS
