// AGS Editor ImGui - Clipboard Manager
// Provides both system text clipboard (via SDL2) and an internal
// typed clipboard for game entities (characters, controls, views, etc.)
#pragma once

#include <string>
#include <any>
#include <vector>

#include <SDL.h>

namespace AGSEditor
{

// Clipboard content types beyond plain text
enum class ClipboardType
{
    None,
    Text,
    GUIControls,
    ViewFrames,
    ViewLoop,
    TreeNode,     // Project tree item
    Custom,
};

class ClipboardManager
{
public:
    ClipboardManager() = default;

    // --- System text clipboard (via SDL2) ---
    void SetText(const std::string& text)
    {
        SDL_SetClipboardText(text.c_str());
        type_ = ClipboardType::Text;
    }

    std::string GetText() const
    {
        char* text = SDL_GetClipboardText();
        std::string result = text ? text : "";
        SDL_free(text);
        return result;
    }

    bool HasText() const
    {
        return SDL_HasClipboardText() == SDL_TRUE;
    }

    // --- Typed internal clipboard for game entities ---
    // Store any type of data using std::any
    template <typename T>
    void SetData(ClipboardType type, const T& data)
    {
        type_ = type;
        data_ = data;
    }

    template <typename T>
    void SetData(ClipboardType type, const std::vector<T>& data)
    {
        type_ = type;
        data_ = data;
    }

    template <typename T>
    bool GetData(T& out) const
    {
        try {
            out = std::any_cast<T>(data_);
            return true;
        } catch (...) {
            return false;
        }
    }

    template <typename T>
    bool GetDataVector(std::vector<T>& out) const
    {
        try {
            out = std::any_cast<std::vector<T>>(data_);
            return true;
        } catch (...) {
            return false;
        }
    }

    ClipboardType GetType() const { return type_; }
    bool HasData() const { return type_ != ClipboardType::None; }

    void Clear()
    {
        type_ = ClipboardType::None;
        data_.reset();
    }

private:
    ClipboardType type_ = ClipboardType::None;
    std::any data_;
};

} // namespace AGSEditor
