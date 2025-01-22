//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_CCGUICONTROL_H
#define __AC_CCGUICONTROL_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct CCGUIControl : AGSCCDynamicObject
{
public:
    // return the type name of the object
    const char *GetType() override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

protected:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;
};

//
// Following subclasses are necessary only for GetType() overrides,
// matching type names in script API.
//
struct CCGUIButton final : public CCGUIControl
{
public:
    const char *GetType() override { return "Button"; }
};

struct CCGUIInvWindow final : public CCGUIControl
{
public:
    const char *GetType() override { return "InvWindow"; }
};

struct CCGUILabel final : public CCGUIControl
{
public:
    const char *GetType() override { return "Label"; }
};

struct CCGUIListBox final : public CCGUIControl
{
public:
    const char *GetType() override { return "ListBox"; }
};

struct CCGUISlider final : public CCGUIControl
{
public:
    const char *GetType() override { return "Slider"; }
};

struct CCGUITextBox final : public CCGUIControl
{
public:
    const char *GetType() override { return "TextBox"; }
};

#endif // __AC_CCGUICONTROL_H
