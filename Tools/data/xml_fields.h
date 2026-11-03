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
//
// Table-driven reading of XML elements into plain structs.
//
// C++ has no reflection, so every struct read from the editor's XML gets a
// table: one row per XML element, naming the element and the function that
// stores its value. ReadFields() walks the children of an element and looks
// each one up in the table:
//
//   const XmlField<RoomXml> kRoomFields[] = {
//       {"Width", XML_READ(&RoomXml::Width)},                         // parser picked by member type
//       {"BlendMode", XML_READ_WITH(&RoomXml::BlendMode, ParseBlendMode)}, // custom parser
//       {"Objects", XML_READ_ELEMENT(&RoomXml::Objects, ReadObjects)}, // nested elements
//       {"SaveLoadEnabled", nullptr},                                 // known, deliberately not read
//   };
//
// Defaults live in the struct's member initializers: a missing element leaves
// its member alone. An element that no row names is an error by default, as
// it is for the editor's own deserializer (AGS.Types SerializeUtils).
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__XML_FIELDS_H
#define __AGS_TOOL_DATA__XML_FIELDS_H

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
#include <tinyxml2.h>
#include "util/error.h"
#include "util/geometry.h"
#include "util/string.h"

namespace AGS
{
namespace AGF
{

// Reads one XML element into the object
template <typename T>
using XmlFieldReader = Common::HError (*)(const tinyxml2::XMLElement *elem, T &obj);

// One row of a table: an XML element, and the function that reads it;
// a null function marks an element that is known but deliberately not read
template <typename T>
struct XmlField
{
    const char *Element;
    XmlFieldReader<T> Read;
};

// What ReadFields does with a child element that no row names
enum class UnknownElements
{
    Fail, // return an error: the data has something that the tools don't know
    Skip  // ignore it: the table reads only a part of the element on purpose
};

// Rectangle as the editor writes it: "x,y,width,height"
struct XmlRect
{
    int X = 0;
    int Y = 0;
    int Width = 0;
    int Height = 0;
};

// Value parsers; each returns false if the text is not a valid value.
// They accept what the editor writes, the way the editor reads it back.
bool ParseXmlValue(const char *text, int &value);
bool ParseXmlValue(const char *text, bool &value); // "True" or "False"
bool ParseXmlValue(const char *text, float &value);
bool ParseXmlValue(const char *text, Common::String &value);
bool ParseXmlValue(const char *text, Point &value); // "x,y"
bool ParseXmlValue(const char *text, XmlRect &value);
// Parses an enum value written by its name; names[i] is the name of value i
bool ParseXmlEnum(const char *text, const char *const names[], size_t count, int &value);
template <size_t N>
bool ParseXmlEnum(const char *text, const std::array<const char *, N> &names, int &value)
{
    return ParseXmlEnum(text, names.data(), N, value);
}

// AGS.Types GraphicAnchor: "x, y"; numbers that don't parse read as 0, both are clamped to 0-1
bool ParseGraphicAnchor(const char *text, Pointf &value);
// AGS.Types BlendMode, which has the values of Common::BlendMode
bool ParseBlendMode(const char *text, int &value);

// Compares "X.Y.Z.W" version strings numerically (missing parts count as 0);
// returns a negative, zero or positive value like strcmp
int CompareXmlVersions(const char *a, const char *b);

namespace XmlDetail
{
template <typename M>
struct MemberPtr;
template <typename C, typename V>
struct MemberPtr<V C::*>
{
    using Class = C;
    using Value = V;
};

// Returns the element's text, or an empty string; never null
const char *ElementText(const tinyxml2::XMLElement *elem);
Common::HError InvalidValue(const tinyxml2::XMLElement *elem);
Common::HError UnknownElement(const tinyxml2::XMLElement *parent, const tinyxml2::XMLElement *elem);
} // namespace XmlDetail

// Row reader that parses the element's text into a member, with the parser for its type.
// C++11 has no "auto" template parameters, so the member pointer comes with its type;
// write the row with XML_READ(&T::Member), which adds the type.
template <typename MP, MP Member>
Common::HError Read(const tinyxml2::XMLElement *elem, typename XmlDetail::MemberPtr<MP>::Class &obj)
{
    if (!ParseXmlValue(XmlDetail::ElementText(elem), obj.*Member))
        return XmlDetail::InvalidValue(elem);
    return Common::HError::None();
}

// Row reader that parses the element's text into a member, with a custom parser;
// write the row with XML_READ_WITH(&T::Member, Parse)
template <typename MP, MP Member, bool (*Parse)(const char *text, typename XmlDetail::MemberPtr<MP>::Value &value)>
Common::HError ReadWith(const tinyxml2::XMLElement *elem, typename XmlDetail::MemberPtr<MP>::Class &obj)
{
    if (!Parse(XmlDetail::ElementText(elem), obj.*Member))
        return XmlDetail::InvalidValue(elem);
    return Common::HError::None();
}

// Row reader that gives the element itself to a function, for nested elements and attributes;
// write the row with XML_READ_ELEMENT(&T::Member, ReadFn)
template <typename MP, MP Member,
          Common::HError (*ReadFn)(const tinyxml2::XMLElement *elem, typename XmlDetail::MemberPtr<MP>::Value &value)>
Common::HError ReadElement(const tinyxml2::XMLElement *elem, typename XmlDetail::MemberPtr<MP>::Class &obj)
{
    return ReadFn(elem, obj.*Member);
}

#define XML_READ(member) AGS::AGF::Read<decltype(member), member>
#define XML_READ_WITH(member, parse) AGS::AGF::ReadWith<decltype(member), member, parse>
#define XML_READ_ELEMENT(member, read_fn) AGS::AGF::ReadElement<decltype(member), member, read_fn>

// Fills the object from the child elements of elem, using the table
template <typename T, size_t N>
Common::HError ReadFields(const tinyxml2::XMLElement *elem, T &obj, const XmlField<T> (&fields)[N],
                          UnknownElements unknown = UnknownElements::Fail)
{
    for (const tinyxml2::XMLElement *child = elem->FirstChildElement(); child; child = child->NextSiblingElement())
    {
        const XmlField<T> *field = nullptr;
        for (const auto &f : fields)
        {
            if (std::strcmp(f.Element, child->Name()) == 0)
            {
                field = &f;
                break;
            }
        }
        if (!field)
        {
            if (unknown == UnknownElements::Fail)
                return XmlDetail::UnknownElement(elem, child);
            continue;
        }
        if (!field->Read)
            continue;
        Common::HError err = field->Read(child, obj);
        if (!err)
            return err;
    }
    return Common::HError::None();
}

// Reads every child of list_elem as one item, using the item table; every child must be
// named item_name. Reads at most max_items, like the editor, which drops the rest.
template <typename T, size_t N>
Common::HError ReadList(const tinyxml2::XMLElement *list_elem, const char *item_name, std::vector<T> &items,
                        const XmlField<T> (&fields)[N], size_t max_items = SIZE_MAX,
                        UnknownElements unknown = UnknownElements::Fail)
{
    items.clear();
    for (const tinyxml2::XMLElement *child = list_elem->FirstChildElement(); child && items.size() < max_items;
         child = child->NextSiblingElement())
    {
        if (std::strcmp(child->Name(), item_name) != 0)
            return XmlDetail::UnknownElement(list_elem, child);
        T item;
        Common::HError err = ReadFields(child, item, fields, unknown);
        if (!err)
            return err;
        items.push_back(std::move(item));
    }
    return Common::HError::None();
}

} // namespace AGF
} // namespace AGS

#endif // __AGS_TOOL_DATA__XML_FIELDS_H
