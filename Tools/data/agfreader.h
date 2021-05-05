//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
// AGS::AGF namespace contains utilities for reading and parsing AGS game
// project format (AGF), which is essentially an XML.
//
// AGFReader opens the file and makes basic check for validity.
//
// EntityListParser and its subclasses parse lists of game entities and return
// collection of their document nodes.
//
// EntityParser and its subclasses parse entity itself. There's a number of
// common properties which may be accessed using EntityParser interface, but
// most of the data is only retrieved by a specific parser subtype.
//
// All the above classes use DocElem: a simple copyable type which references
// particular AGF node. DocElem instances are allocated by AGFReader internally
// and persist as long as AGFReader object does. They do not have to be deleted
// by the user.
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__AGFREADER_H
#define __AGS_TOOL_DATA__AGFREADER_H

#include <memory>
#include "data/game_utils.h"
#include "util/error.h"

namespace tinyxml2 { class XMLDocument; class XMLElement; }

namespace AGS
{
namespace AGF
{

using AGS::Common::String;
typedef tinyxml2::XMLDocument Document;
typedef tinyxml2::XMLElement* DocElem;

// AGFReader opens AGF document and checks it for validity.
// Provides a Game data root node.
class AGFReader
{
public:
    AGFReader();
    ~AGFReader();

    AGS::Common::HError Open(const char *filename);
    void Close();

    inline DocElem GetGameRoot() const { return _gameRoot; }

private:
    std::unique_ptr<Document> _doc;
    DocElem _gameRoot;
};

// EntityParser: parent class meant for parsing a game entity
class EntityParser
{
public:
    EntityParser() = default;
    virtual ~EntityParser() = default;

    // Read name of entity's type
    virtual String ReadType(DocElem elem) = 0;
    // Read entity's numeric ID (aka index)
    virtual int ReadID(DocElem elem) = 0;
    // Read entity's script name (may be null)
    virtual String ReadScriptName(DocElem elem) = 0;

protected:
    // Helper functions for reading certain field;
    // all of them return default value if the field cannot be read
    const char *ReadString(DocElem elem, const char *field, const char *def_value = "");
    int ReadInt(DocElem elem, const char *field, int def_value = 0);
};

// EntityListParser: parent class meant for parsing a list of game entities
// and returning a collection of their document nodes.
// In AGF document lists may contain subfolders with (theoretically) unlimited
// nesting. List parser takes care of that. The returned nodes are all pointing
// directly to the object data, skipping folders.
class EntityListParser
{
public:
    EntityListParser() = default;
    virtual ~EntityListParser() = default;

    // Returns a collection of nodes referencing object data
    virtual void GetAll(DocElem root, std::vector<DocElem> &elems) = 0;

protected:
    // Parses a list or tree of nodes;
    // folder_elem - name of the folder element; pass null if entity list
    //     does not support subfolders;
    // list_elem - name of the list element, cannot be null;
    // type_elem - name of the object element; pass null if exact name should
    //     be ignored: this is useful if the list contains multiple types;
    void GetAllElems(DocElem root, std::vector<DocElem> &elems,
        const char *folder_elem, const char *list_elem, const char *type_elem);

private:
    // Internal implementation of recursive subfolder parsing
    void GetElemsRecursive(DocElem folder, std::vector<DocElem> &elems,
        const char *folder_elem, const char *list_elem, const char *type_elem);
};


// AudioClip data parser
class AudioClip : public EntityParser
{
public:
    String ReadType(DocElem elem) override { return "AudioClip"; }
    int ReadID(DocElem elem) override { return ReadInt(elem, "ID", -1); }
    String ReadScriptName(DocElem elem) override { return ReadString(elem, "ScriptName"); }
};

// AudioType data parser
class AudioType : public EntityParser
{
public:
    String ReadType(DocElem elem) override { return ""; }
    int ReadID(DocElem elem) override { return ReadInt(elem, "TypeID"); }
    String ReadScriptName(DocElem elem) override { return ReadString(elem, "Name"); }
};

// Character data parser
class Character : public EntityParser
{
public:
    String ReadType(DocElem elem) override { return "Character"; }
    int ReadID(DocElem elem) override { return ReadInt(elem, "ID", -1); }
    String ReadScriptName(DocElem elem) override { return ReadString(elem, "ScriptName");  }
};

// Cursor data parser
class Cursor : public EntityParser
{
public:
    String ReadType(DocElem elem) override { return ""; }
    int ReadID(DocElem elem) override { return ReadInt(elem, "ID", -1); }
    String ReadScriptName(DocElem elem) override { return ReadString(elem, "Name"); }
};

// Dialog data parser
class Dialog : public EntityParser
{
public:
    String ReadType(DocElem elem) override { return "Dialog"; }
    int ReadID(DocElem elem) override { return ReadInt(elem, "ID", -1); }
    String ReadScriptName(DocElem elem) override { return ReadString(elem, "Name"); }
};

// Font data parser
class Font : public EntityParser
{
public:
    String ReadType(DocElem elem) override { return ""; }
    int ReadID(DocElem elem) override { return ReadInt(elem, "ID", -1); }
    String ReadScriptName(DocElem elem) override { return ReadString(elem, "Name"); }
};

// GUI Control data parser
class GUIControl : public EntityParser
{
public:
    String ReadType(DocElem elem) override;
    int ReadID(DocElem elem) override { return ReadInt(elem, "ID", -1); }
    String ReadScriptName(DocElem elem) override { return ReadString(elem, "Name"); }
};

// GUI data parser
class GUIMain : public EntityParser
{
public:
    String ReadType(DocElem elem) override { return "GUI"; }
    int ReadID(DocElem elem) override;
    String ReadScriptName(DocElem elem) override;

private:
    DocElem GetNormalGUI(DocElem elem);
    DocElem GetTextWindow(DocElem elem);
};

// Inventory Item data parser
class InventoryItem : public EntityParser
{
public:
    String ReadType(DocElem elem) override { return "InventoryItem"; }
    int ReadID(DocElem elem) override { return ReadInt(elem, "ID", -1); }
    String ReadScriptName(DocElem elem) override { return ReadString(elem, "Name"); }
};

// View data parser
class View : public EntityParser
{
public:
    String ReadType(DocElem elem) override { return ""; }
    int ReadID(DocElem elem) override { return ReadInt(elem, "ID", -1); }
    String ReadScriptName(DocElem elem) override { return ReadString(elem, "Name"); }
};


// Audio Clip list parser
class AudioClips : public EntityListParser
{
public:
    void GetAll(DocElem root, std::vector<DocElem> &elems) override
    {
        GetAllElems(root, elems, "AudioClipFolder", "AudioClips", "AudioClip");
    }
};

// Audio Type list parser
class AudioTypes : public EntityListParser
{
public:
    void GetAll(DocElem root, std::vector<DocElem> &elems) override
    {
        GetAllElems(root, elems, nullptr, "AudioClipTypes", "AudioClipType");
    }
};

// Character list parser
class Characters : public EntityListParser
{
public:
    void GetAll(DocElem root, std::vector<DocElem> &elems) override
    {
        GetAllElems(root, elems, "CharacterFolder", "Characters", "Character");
    }
};

// Cursor list parser
class Cursors : public EntityListParser
{
public:
    void GetAll(DocElem root, std::vector<DocElem> &elems) override
    {
        GetAllElems(root, elems, nullptr, "Cursors", "MouseCursor");
    }
};

// Dialog list parser
class Dialogs : public EntityListParser
{
public:
    void GetAll(DocElem root, std::vector<DocElem> &elems) override
    {
        GetAllElems(root, elems, "DialogFolder", "Dialogs", "Dialog");
    }
};

// Font list parser
class Fonts : public EntityListParser
{
public:
    void GetAll(DocElem root, std::vector<DocElem> &elems) override
    {
        GetAllElems(root, elems, nullptr, "Fonts", "Font");
    }
};

// GUI list parser
class GUIs : public EntityListParser
{
public:
    void GetAll(DocElem root, std::vector<DocElem> &elems) override
    {
        GetAllElems(root, elems, "GUIFolder", "GUIs", "GUIMain");
    }
};

// GUI Control list parser
class GUIControls : public EntityListParser
{
public:
    void GetAll(DocElem root, std::vector<DocElem> &elems) override
    {
        GetAllElems(root, elems, nullptr, "Controls", nullptr);
    }
};

// Inventory Item list parser
class Inventory : public EntityListParser
{
public:
    void GetAll(DocElem root, std::vector<DocElem> &elems) override
    {
        GetAllElems(root, elems, "InventoryItemFolder", "InventoryItems", "InventoryItem");
    }
};

// View list parser
class Views : public EntityListParser
{
public:
    void GetAll(DocElem root, std::vector<DocElem> &elems) override
    {
        GetAllElems(root, elems, "ViewFolder", "Views", "View");
    }
};


//
// Helper functions
//

// Read entity reference data using given parser from the given doc element
void ReadEntityRef(DataUtil::EntityRef &ent, EntityParser &parser, DocElem elem);
// Reads a group of entity ref data using given list and entity parsers
// from the given doc root element.
void ReadAllEntityRefs(std::vector<DataUtil::EntityRef> &ents, EntityListParser &list_parser,
    EntityParser &parser, DocElem root);
// Reads full game reference data using AGFReader
void ReadGameRef(DataUtil::GameRef &game, AGFReader &reader);

} // namespace AGF
} // namespace AGS

#endif // __AGS_TOOL_DATA__AGFREADER_H
