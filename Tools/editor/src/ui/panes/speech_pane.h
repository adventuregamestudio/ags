// AGS Editor ImGui - Speech Pane
// Shows information about the Speech directory and voice-over files.
// Provides quick access to speech tools (auto-numbering, voice acting script).
#pragma once

#include "ui/editor_ui.h"
#include <string>
#include <vector>

namespace AGSEditor
{

class SpeechPane : public EditorPane
{
public:
    explicit SpeechPane(EditorUI& editor);

    void Draw() override;
    const char* GetTitle() const override { return "Speech"; }

private:
    void RefreshFileList();

    EditorUI& editor_;
    bool needs_refresh_ = true;

    struct SpeechFileInfo {
        std::string filename;
        std::string extension;
        size_t size = 0;
    };

    std::vector<SpeechFileInfo> files_;
    std::vector<std::string> subdirs_;
    size_t total_size_ = 0;
    int ogg_count_ = 0;
    int mp3_count_ = 0;
    int wav_count_ = 0;
    int other_count_ = 0;
};

} // namespace AGSEditor
