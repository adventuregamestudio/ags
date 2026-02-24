// AGS Editor ImGui - Default Setup pane
// Allows editing the runtime configuration (graphics, audio, mouse, etc.)
#include "default_setup_pane.h"
#include "project/game_data.h"
#include "project/project.h"
#include "app.h"
#include "core/dpi_helper.h"
#include "imgui.h"

#include <cstdio>
#include <cstring>

namespace AGSEditor
{

DefaultSetupPane::DefaultSetupPane(EditorUI& editor)
    : editor_(editor)
{
}

void DefaultSetupPane::Draw()
{
    auto* project = editor_.GetProject();
    if (!project || !project->GetGameData())
    {
        ImGui::TextDisabled("No project loaded.");
        return;
    }

    ImGui::BeginChild("DefaultSetupScroll", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    ImGui::PushItemWidth(Dpi(250));

    DrawGraphicsSection();
    DrawAudioSection();
    DrawGameplaySection();
    DrawMouseSection();
    DrawTouchSection();
    DrawMiscSection();
    DrawPerformanceSection();
    DrawEnvironmentSection();

    ImGui::PopItemWidth();
    ImGui::EndChild();
}

void DefaultSetupPane::DrawGraphicsSection()
{
    auto& rs = editor_.GetProject()->GetGameData()->runtime_setup;

    if (ImGui::CollapsingHeader("Graphics", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(Dpi(8));

        // Graphics driver
        const char* gfx_drivers[] = { "Software", "Direct3D 9", "OpenGL" };
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Graphics driver:");
        ImGui::SameLine(Dpi(220));
        ImGui::Combo("##GfxDriver", &rs.graphics_driver, gfx_drivers, 3);

        // Start in windowed mode
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Start in windowed mode:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##Windowed", &rs.windowed);

        // Fullscreen as borderless window
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Fullscreen as borderless:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##FullscreenDesktop", &rs.fullscreen_desktop);

        // Fullscreen scaling
        const char* fs_modes[] = { "None (original size)", "Proportional stretch", "Stretch to fit", "Integer scaling" };
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Fullscreen scaling:");
        ImGui::SameLine(Dpi(220));
        ImGui::Combo("##FSScaling", &rs.fullscreen_scaling, fs_modes, 4);

        // Windowed scaling
        const char* win_modes[] = { "None (original size)", "Max round multiplier", "Stretch to fit", "Max integer multiplier" };
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Windowed scaling:");
        ImGui::SameLine(Dpi(220));
        ImGui::Combo("##WinScaling", &rs.windowed_scaling, win_modes, 4);

        // Scaling multiplier
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Scaling multiplier:");
        ImGui::SameLine(Dpi(220));
        ImGui::InputInt("##ScaleMul", &rs.scaling_multiplier);
        if (rs.scaling_multiplier < 1) rs.scaling_multiplier = 1;
        if (rs.scaling_multiplier > 8) rs.scaling_multiplier = 8;

        // Graphics filter
        const char* filters[] = { "Nearest-neighbour", "Linear (anti-alias)" };
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Graphics filter:");
        ImGui::SameLine(Dpi(220));
        ImGui::Combo("##GfxFilter", &rs.graphics_filter, filters, 2);

        // VSync
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Vertical sync:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##VSync", &rs.vsync);

        // Anti-alias scaled sprites
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Anti-alias scaled sprites:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##AAScaled", &rs.aa_scaled_sprites);

        // Render at screen resolution
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Render at screen resolution:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##RenderAtScreenRes", &rs.render_at_screen_res);

        // Rotation
        const char* rot_modes[] = { "Unlocked", "Force portrait", "Force landscape" };
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Rotation:");
        ImGui::SameLine(Dpi(220));
        ImGui::Combo("##Rotation", &rs.rotation, rot_modes, 3);

        ImGui::Unindent(Dpi(8));
        ImGui::Spacing();
    }
}

void DefaultSetupPane::DrawAudioSection()
{
    auto& rs = editor_.GetProject()->GetGameData()->runtime_setup;

    if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(Dpi(8));

        // Digital sound
        const char* dsound_opts[] = { "Default", "Disable", "DirectSound (Windows)", "WASAPI (Windows)" };
        // Map: -1=Default, 0=Disable, 1=DirectSound, 2=WASAPI
        int ds_idx = rs.digital_sound + 1; // shift so -1 becomes 0
        if (ds_idx < 0) ds_idx = 0;
        if (ds_idx > 3) ds_idx = 0;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Digital sound:");
        ImGui::SameLine(Dpi(220));
        if (ImGui::Combo("##DigitalSound", &ds_idx, dsound_opts, 4))
            rs.digital_sound = ds_idx - 1;

        // Use voice pack
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Use voice pack if available:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##UseVoicePack", &rs.use_voice_pack);

        ImGui::Unindent(Dpi(8));
        ImGui::Spacing();
    }
}

void DefaultSetupPane::DrawGameplaySection()
{
    auto& rs = editor_.GetProject()->GetGameData()->runtime_setup;

    if (ImGui::CollapsingHeader("Gameplay", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(Dpi(8));

        // Translation
        char trans_buf[256];
        snprintf(trans_buf, sizeof(trans_buf), "%s", rs.translation.c_str());
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Game language:");
        ImGui::SameLine(Dpi(220));
        if (ImGui::InputText("##Translation", trans_buf, sizeof(trans_buf)))
            rs.translation = trans_buf;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Leave blank for default language,\nor enter the translation file name (without extension)");

        ImGui::Unindent(Dpi(8));
        ImGui::Spacing();
    }
}

void DefaultSetupPane::DrawMouseSection()
{
    auto& rs = editor_.GetProject()->GetGameData()->runtime_setup;

    if (ImGui::CollapsingHeader("Mouse", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(Dpi(8));

        // Auto lock mouse
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Auto-lock mouse to window:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##AutoLockMouse", &rs.auto_lock_mouse);

        // Mouse speed
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Mouse cursor speed:");
        ImGui::SameLine(Dpi(220));
        ImGui::SliderFloat("##MouseSpeed", &rs.mouse_speed, 0.1f, 5.0f, "%.2f");

        ImGui::Unindent(Dpi(8));
        ImGui::Spacing();
    }
}

void DefaultSetupPane::DrawTouchSection()
{
    auto& rs = editor_.GetProject()->GetGameData()->runtime_setup;

    if (ImGui::CollapsingHeader("Touch", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(Dpi(8));

        // Touch to mouse emulation
        const char* touch_emu[] = { "Off", "One finger", "Two fingers" };
        int te_idx = rs.touch_emulation;
        if (te_idx < 0 || te_idx > 2) te_idx = 1;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Touch-to-mouse emulation:");
        ImGui::SameLine(Dpi(220));
        if (ImGui::Combo("##TouchEmulate", &te_idx, touch_emu, 3))
            rs.touch_emulation = te_idx;

        // Touch motion mode
        const char* touch_mot[] = { "Relative (like a touchpad)", "Direct (absolute position)" };
        int tm_idx = rs.touch_motion;
        if (tm_idx < 0 || tm_idx > 1) tm_idx = 0;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Touch motion mode:");
        ImGui::SameLine(Dpi(220));
        if (ImGui::Combo("##TouchMotion", &tm_idx, touch_mot, 2))
            rs.touch_motion = tm_idx;

        ImGui::Unindent(Dpi(8));
        ImGui::Spacing();
    }
}

void DefaultSetupPane::DrawMiscSection()
{
    auto& rs = editor_.GetProject()->GetGameData()->runtime_setup;

    if (ImGui::CollapsingHeader("Diagnostics##DefaultSetup"))
    {
        ImGui::Indent(Dpi(8));

        // Show FPS
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Display FPS on screen:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##ShowFPS", &rs.show_fps);

        ImGui::Unindent(Dpi(8));
        ImGui::Spacing();
    }
}

void DefaultSetupPane::DrawPerformanceSection()
{
    auto& rs = editor_.GetProject()->GetGameData()->runtime_setup;

    if (ImGui::CollapsingHeader("Performance & Cache", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Indent(Dpi(8));

        // Sprite cache size
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Sprite cache (MB):");
        ImGui::SameLine(Dpi(220));
        ImGui::InputInt("##SpriteCacheSize", &rs.sprite_cache_size);
        if (rs.sprite_cache_size < 16) rs.sprite_cache_size = 16;
        if (rs.sprite_cache_size > 4096) rs.sprite_cache_size = 4096;

        // Texture cache size
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Texture cache (MB):");
        ImGui::SameLine(Dpi(220));
        ImGui::InputInt("##TextureCacheSize", &rs.texture_cache_size);
        if (rs.texture_cache_size < 16) rs.texture_cache_size = 16;
        if (rs.texture_cache_size > 4096) rs.texture_cache_size = 4096;

        // Sound cache size
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Sound cache (MB):");
        ImGui::SameLine(Dpi(220));
        ImGui::InputInt("##SoundCacheSize", &rs.sound_cache_size);
        if (rs.sound_cache_size < 4) rs.sound_cache_size = 4;
        if (rs.sound_cache_size > 1024) rs.sound_cache_size = 1024;

        // Compress saves
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Compress save files:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##CompressSaves", &rs.compress_saves);

        ImGui::Unindent(Dpi(8));
        ImGui::Spacing();
    }
}

void DefaultSetupPane::DrawEnvironmentSection()
{
    auto& rs = editor_.GetProject()->GetGameData()->runtime_setup;

    if (ImGui::CollapsingHeader("Environment & Paths"))
    {
        ImGui::Indent(Dpi(8));

        // Custom save path
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Use custom save path:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##UseCustomSavePath", &rs.use_custom_save_path);

        if (rs.use_custom_save_path)
        {
            char save_buf[512];
            snprintf(save_buf, sizeof(save_buf), "%s", rs.custom_save_path.c_str());
            ImGui::AlignTextToFramePadding();
            ImGui::Text("  Custom save path:");
            ImGui::SameLine(Dpi(220));
            if (ImGui::InputText("##CustomSavePath", save_buf, sizeof(save_buf)))
                rs.custom_save_path = save_buf;
        }

        // Custom AppData path
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Use custom app data path:");
        ImGui::SameLine(Dpi(220));
        ImGui::Checkbox("##UseCustomAppDataPath", &rs.use_custom_appdata_path);

        if (rs.use_custom_appdata_path)
        {
            char app_buf[512];
            snprintf(app_buf, sizeof(app_buf), "%s", rs.custom_appdata_path.c_str());
            ImGui::AlignTextToFramePadding();
            ImGui::Text("  Custom app data path:");
            ImGui::SameLine(Dpi(220));
            if (ImGui::InputText("##CustomAppDataPath", app_buf, sizeof(app_buf)))
                rs.custom_appdata_path = app_buf;
        }

        ImGui::Separator();

        // Title text (setup window title)
        char title_buf[256];
        snprintf(title_buf, sizeof(title_buf), "%s", rs.title_text.c_str());
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Setup window title:");
        ImGui::SameLine(Dpi(220));
        if (ImGui::InputText("##TitleText", title_buf, sizeof(title_buf)))
            rs.title_text = title_buf;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Custom title for the setup dialog.\nLeave blank for the default title.");

        ImGui::Unindent(Dpi(8));
        ImGui::Spacing();
    }
}

} // namespace AGSEditor
