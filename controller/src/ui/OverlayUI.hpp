#pragma once

#include "entities/OverlayConfig.hpp"
#include "imgui.h"
#include <string>
#include <cstring>

namespace overlayx {

/// Modern Windows 11-inspired ImGui View.
class OverlayUI {
public:
    /// Call once at startup after ImGui context is created.
    static void applyTheme() {
        ImGuiStyle& s = ImGui::GetStyle();

        // ─── Geometry — Windows 11 proportions ─────────────────
        s.WindowRounding    = 0.0f;   // OS frame handles corners
        s.ChildRounding     = 6.0f;
        s.FrameRounding     = 4.0f;
        s.GrabRounding      = 4.0f;
        s.TabRounding       = 4.0f;
        s.PopupRounding     = 6.0f;
        s.ScrollbarRounding = 6.0f;
        s.WindowPadding     = ImVec2(0, 0);
        s.FramePadding      = ImVec2(12, 7);
        s.ItemSpacing       = ImVec2(10, 7);
        s.ItemInnerSpacing  = ImVec2(8, 4);
        s.WindowBorderSize  = 0.0f;
        s.FrameBorderSize   = 1.0f;
        s.ScrollbarSize     = 10.0f;
        s.GrabMinSize       = 8.0f;
        s.IndentSpacing     = 20.0f;

        auto& c = s.Colors;

        // ─── Mica-inspired dark palette ────────────────────────
        ImVec4 bg          = ImVec4(0.11f, 0.11f, 0.13f, 1.00f);
        ImVec4 bgSidebar   = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);
        ImVec4 surface     = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
        ImVec4 surfaceHov  = ImVec4(0.19f, 0.19f, 0.22f, 1.00f);
        ImVec4 surfaceAct  = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
        ImVec4 accent      = ImVec4(0.38f, 0.56f, 0.94f, 1.00f);  // Windows blue
        ImVec4 accentHov   = ImVec4(0.48f, 0.64f, 0.98f, 1.00f);
        ImVec4 accentAct   = ImVec4(0.30f, 0.48f, 0.86f, 1.00f);
        ImVec4 border      = ImVec4(0.22f, 0.22f, 0.25f, 0.70f);
        ImVec4 textPri     = ImVec4(0.95f, 0.95f, 0.96f, 1.00f);
        ImVec4 textSec     = ImVec4(0.60f, 0.60f, 0.64f, 1.00f);
        ImVec4 textDim     = ImVec4(0.42f, 0.42f, 0.46f, 1.00f);

        c[ImGuiCol_WindowBg]          = bg;
        c[ImGuiCol_ChildBg]           = ImVec4(0, 0, 0, 0); // transparent by default
        c[ImGuiCol_PopupBg]           = ImVec4(0.13f, 0.13f, 0.15f, 0.97f);
        c[ImGuiCol_Border]            = border;
        c[ImGuiCol_FrameBg]           = surface;
        c[ImGuiCol_FrameBgHovered]    = surfaceHov;
        c[ImGuiCol_FrameBgActive]     = surfaceAct;
        c[ImGuiCol_TitleBg]           = bgSidebar;
        c[ImGuiCol_TitleBgActive]     = bgSidebar;
        c[ImGuiCol_Tab]               = surface;
        c[ImGuiCol_TabHovered]        = accentHov;
        c[ImGuiCol_TabSelected]       = accent;
        c[ImGuiCol_Button]            = surface;
        c[ImGuiCol_ButtonHovered]     = surfaceHov;
        c[ImGuiCol_ButtonActive]      = surfaceAct;
        c[ImGuiCol_Header]            = surface;
        c[ImGuiCol_HeaderHovered]     = surfaceHov;
        c[ImGuiCol_HeaderActive]      = surfaceAct;
        c[ImGuiCol_SliderGrab]        = accent;
        c[ImGuiCol_SliderGrabActive]  = accentHov;
        c[ImGuiCol_CheckMark]         = accent;
        c[ImGuiCol_Separator]         = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
        c[ImGuiCol_SeparatorHovered]  = accent;
        c[ImGuiCol_Text]              = textPri;
        c[ImGuiCol_TextDisabled]      = textSec;
        c[ImGuiCol_ScrollbarBg]       = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ScrollbarGrab]     = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.36f, 0.40f, 1.00f);
        c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.44f, 0.44f, 0.48f, 1.00f);
        c[ImGuiCol_ResizeGrip]        = ImVec4(0, 0, 0, 0);
    }

    /// Returns true if config was modified this frame.
    bool render(OverlayConfig& config, bool engineConnected) {
        bool modified = false;
        static int selectedTab = 0;

        const ImGuiIO& io = ImGui::GetIO();
        ImFont* fontBody  = io.Fonts->Fonts.Size > 0 ? io.Fonts->Fonts[0] : nullptr;
        ImFont* fontTitle = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : fontBody;

        // Fill the OS window client area
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##Main", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

        auto* drawList = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();

        // ─── Sidebar Background ────────────────────────────────
        const float sidebarW = 240.0f;
        drawList->AddRectFilled(wp, ImVec2(wp.x + sidebarW, wp.y + ws.y),
            ImColor(23, 23, 28, 255));
        // Separator line
        drawList->AddLine(ImVec2(wp.x + sidebarW, wp.y),
            ImVec2(wp.x + sidebarW, wp.y + ws.y),
            ImColor(40, 40, 46, 255), 1.0f);

        // ─── Sidebar ───────────────────────────────────────────
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::BeginChild("Sidebar", ImVec2(sidebarW, 0), false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
        {
            ImGui::SetCursorPos(ImVec2(20, 18));
            if (fontTitle) ImGui::PushFont(fontTitle);
            ImGui::TextColored(ImVec4(0.38f, 0.56f, 0.94f, 1.0f), "OverlayX");
            if (fontTitle) ImGui::PopFont();

            ImGui::SetCursorPosX(20);
            ImGui::TextColored(ImVec4(0.42f, 0.42f, 0.46f, 1.0f), "v3.0");

            ImGui::SetCursorPosY(70);
            ImGui::Spacing();

            // Navigation items
            struct NavItem { const char* label; int id; };
            NavItem items[] = {
                {"Overview",           0},
                {"Crosshair Designer", 4},
                {"Position & Presets", 2},
                {"Hotkeys",            3},
                {"Settings",           1},
            };

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 12));
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.38f, 0.56f, 0.94f, 0.15f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(1, 1, 1, 0.06f));

            for (auto& item : items) {
                bool sel = (selectedTab == item.id);
                ImVec2 cursor = ImGui::GetCursorScreenPos();

                if (sel) {
                    // Active indicator bar
                    drawList->AddRectFilled(
                        ImVec2(cursor.x + 4, cursor.y + 6),
                        ImVec2(cursor.x + 7, cursor.y + 34),
                        ImColor(97, 143, 240, 255), 2.0f);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0, 0.5f));
                std::string label = std::string("   ") + item.label;
                if (ImGui::Selectable(label.c_str(), sel, 0, ImVec2(sidebarW, 40))) {
                    selectedTab = item.id;
                }
                ImGui::PopStyleVar();
            }

            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);

            // Status at bottom
            ImGui::SetCursorPosY(ws.y - 65);
            drawList->AddLine(
                ImVec2(wp.x + 16, wp.y + ws.y - 68),
                ImVec2(wp.x + sidebarW - 16, wp.y + ws.y - 68),
                ImColor(40, 40, 46, 255), 1.0f);

            ImGui::SetCursorPosX(20);
            if (engineConnected) {
                ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.55f, 1.0f), "Engine Connected");
            } else {
                ImGui::TextColored(ImVec4(0.85f, 0.35f, 0.35f, 1.0f), "Engine Offline");
            }
            ImGui::SetCursorPosX(20);
            ImGui::TextColored(ImVec4(0.42f, 0.42f, 0.46f, 1.0f),
                config.editMode ? "Drag Mode Active" : "Ready");
        }
        ImGui::EndChild();

        // ─── Content Area ──────────────────────────────────────
        ImGui::SetCursorPos(ImVec2(sidebarW + 1, 0));
        ImGui::BeginChild("Content", ImVec2(ws.x - sidebarW - 1, 0), false,
            ImGuiWindowFlags_NoBackground);
        {
            ImGui::SetCursorPos(ImVec2(28, 22));

            const char* titles[] = {
                "Overview", "Settings", "Position & Presets",
                "Hotkeys", "Crosshair Designer"
            };

            if (fontTitle) ImGui::PushFont(fontTitle);
            ImGui::Text("%s", titles[selectedTab]);
            if (fontTitle) ImGui::PopFont();

            ImGui::SetCursorPosX(28);
            ImGui::Spacing();

            // Thin separator under title
            ImVec2 sepP = ImGui::GetCursorScreenPos();
            drawList->AddLine(
                ImVec2(sepP.x, sepP.y),
                ImVec2(sepP.x + ws.x - sidebarW - 56, sepP.y),
                ImColor(40, 40, 46, 255), 1.0f);
            ImGui::Spacing(); ImGui::Spacing();

            // Content with padding
            ImGui::SetCursorPosX(28);
            ImGui::BeginGroup();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);

            renderContent(selectedTab, config, modified, fontTitle, drawList);

            ImGui::PopItemWidth();
            ImGui::EndGroup();
        }
        ImGui::EndChild();

        ImGui::End();
        return modified;
    }

private:

    // ─── Card helper ───────────────────────────────────────────
    static void BeginCard(const char* label, ImDrawList* dl) {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float w = ImGui::GetContentRegionAvail().x - 28;
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.16f, 1.0f));
        ImGui::BeginChild(label, ImVec2(w, 0), false,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
        ImGui::SetCursorPos(ImVec2(16, 12));
    }

    static void EndCard() {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12);
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::EndGroup();
        ImGui::Spacing();
    }

    // ─── Section label ─────────────────────────────────────────
    static void SectionLabel(const char* text) {
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.64f, 1.0f), "%s", text);
        ImGui::Spacing();
    }

    // ─── Accent Button ─────────────────────────────────────────
    static bool AccentButton(const char* label, ImVec2 size = ImVec2(0, 36)) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.38f, 0.56f, 0.94f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.48f, 0.64f, 0.98f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.30f, 0.48f, 0.86f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,           ImVec4(1, 1, 1, 1));
        bool clicked = ImGui::Button(label, size);
        ImGui::PopStyleColor(4);
        return clicked;
    }

    void renderContent(int tab, OverlayConfig& config, bool& modified,
                       ImFont* fontTitle, ImDrawList* dl) {
        if (tab == 0) renderOverview(config, modified, fontTitle, dl);
        else if (tab == 1) renderSettings(config, modified);
        else if (tab == 2) renderPositions(config, modified, dl);
        else if (tab == 3) renderHotkeys(config, modified);
        else if (tab == 4) renderDesigner(config, modified, fontTitle, dl);
    }

    // ═══════════════════════════════════════════════════════════
    // Tab: Overview
    // ═══════════════════════════════════════════════════════════
    void renderOverview(OverlayConfig& config, bool& modified,
                        ImFont* fontTitle, ImDrawList* dl) {
        ImGui::TextWrapped(
            "Welcome to OverlayX. Use the Crosshair Designer to build "
            "complex, multi-layered crosshairs. Save presets to switch "
            "between configurations instantly.");
        ImGui::Spacing(); ImGui::Spacing();

        SectionLabel("QUICK ACTIONS");

        // Layer count info
        ImGui::Text("Active Layers: %zu", config.layers.size());
        ImGui::Spacing();

        if (AccentButton("Enable All Layers", ImVec2(200, 36))) {
            for (auto& l : config.layers) l.enabled = true;
            modified = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Disable All Layers", ImVec2(200, 36))) {
            for (auto& l : config.layers) l.enabled = false;
            modified = true;
        }
    }

    // ═══════════════════════════════════════════════════════════
    // Tab: Settings
    // ═══════════════════════════════════════════════════════════
    void renderSettings(OverlayConfig& config, bool& modified) {
        SectionLabel("VISIBILITY");
        if (ImGui::Checkbox("Master Visibility", &config.visible)) modified = true;

        ImGui::Spacing(); ImGui::Spacing();
        SectionLabel("EDIT MODE");
        if (ImGui::Checkbox("Edit Mode (Drag Overlay)", &config.editMode)) modified = true;
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.54f, 1.0f),
            "Enable to manually drag the crosshair to a new position.");
    }

    // ═══════════════════════════════════════════════════════════
    // Tab: Position & Presets
    // ═══════════════════════════════════════════════════════════
    void renderPositions(OverlayConfig& config, bool& modified, ImDrawList* dl) {
        SectionLabel("FINE TUNING");
        ImGui::PushItemWidth(300);
        if (ImGui::SliderFloat("Horizontal %", &config.posX, 0.0f, 1.0f, "%.4f")) modified = true;
        if (ImGui::SliderFloat("Vertical %",   &config.posY, 0.0f, 1.0f, "%.4f")) modified = true;
        ImGui::PopItemWidth();

        if (AccentButton("Center Crosshair", ImVec2(200, 36))) {
            config.posX = 0.5f; config.posY = 0.5f; modified = true;
        }

        ImGui::Spacing(); ImGui::Spacing();
        SectionLabel("SAVED POSITIONS");

        static char posName[64] = "";
        ImGui::PushItemWidth(300);
        ImGui::InputTextWithHint("##posname", "Position name...", posName, 64);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (AccentButton("Save##pos", ImVec2(80, 0))) {
            if (strlen(posName) > 0) {
                config.savedPositions.push_back({posName, config.posX, config.posY});
                posName[0] = '\0';
                modified = true;
            }
        }

        ImGui::Spacing();
        ImGui::BeginChild("PosList", ImVec2(400, 140), true);
        for (size_t i = 0; i < config.savedPositions.size(); ++i) {
            auto& p = config.savedPositions[i];
            if (ImGui::Selectable(p.name.c_str(), false)) {
                config.posX = p.x; config.posY = p.y;
                modified = true;
            }
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete")) {
                    config.savedPositions.erase(config.savedPositions.begin() + i);
                    modified = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();

        ImGui::Spacing(); ImGui::Spacing();
        SectionLabel("CROSSHAIR PRESETS");

        static char presetName[64] = "";
        ImGui::PushItemWidth(300);
        ImGui::InputTextWithHint("##presetname", "Preset name...", presetName, 64);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (AccentButton("Save##preset", ImVec2(80, 0))) {
            if (strlen(presetName) > 0) {
                config.savedPresets.push_back({presetName, config.layers});
                presetName[0] = '\0';
                modified = true;
            }
        }

        ImGui::Spacing();
        ImGui::BeginChild("PresetList", ImVec2(400, 140), true);
        for (size_t i = 0; i < config.savedPresets.size(); ++i) {
            auto& pr = config.savedPresets[i];
            if (ImGui::Selectable(pr.name.c_str(), false)) {
                config.layers = pr.layers;
                modified = true;
            }
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete")) {
                    config.savedPresets.erase(config.savedPresets.begin() + i);
                    modified = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();
    }

    // ═══════════════════════════════════════════════════════════
    // Tab: Hotkeys
    // ═══════════════════════════════════════════════════════════
    void renderHotkeys(OverlayConfig& config, bool& modified) {
        SectionLabel("TOGGLE HOTKEY");
        ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.54f, 1.0f),
            "Select a function key to toggle overlay visibility.");
        ImGui::Spacing();

        static const char* keys[] = {
            "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12"
        };
        int cur = config.hotkey - 0x70;
        ImGui::PushItemWidth(200);
        if (ImGui::ListBox("##hotkeys", &cur, keys, IM_ARRAYSIZE(keys), 6)) {
            config.hotkey = 0x70 + cur;
            modified = true;
        }
        ImGui::PopItemWidth();
    }

    // ═══════════════════════════════════════════════════════════
    // Tab: Crosshair Designer
    // ═══════════════════════════════════════════════════════════
    void renderDesigner(OverlayConfig& config, bool& modified,
                        ImFont* fontTitle, ImDrawList* dl) {
        static int selLayer = 0;

        // ─── Left: Layer Stack ─────────────────────────────────
        ImGui::BeginChild("LayerPanel", ImVec2(230, 0), false);
        {
            SectionLabel("LAYER STACK");

            if (AccentButton("+ Add Layer", ImVec2(-1, 34))) {
                config.layers.push_back(
                    Layer{"Layer " + std::to_string(config.layers.size())});
                modified = true;
            }
            ImGui::Spacing();

            if (selLayer >= (int)config.layers.size())
                selLayer = (int)config.layers.size() - 1;
            if (selLayer < 0) selLayer = 0;

            ImGui::BeginChild("LayerList", ImVec2(-1, -1), true);
            for (int i = 0; i < (int)config.layers.size(); ++i) {
                auto& l = config.layers[i];
                // Color indicator dot
                ImVec2 cp = ImGui::GetCursorScreenPos();
                ImU32 dotCol = ImColor(
                    l.color.r / 255.0f, l.color.g / 255.0f,
                    l.color.b / 255.0f, l.enabled ? 1.0f : 0.3f);
                dl->AddCircleFilled(ImVec2(cp.x + 10, cp.y + 14), 5, dotCol);

                std::string label = "   " + l.name;
                if (!l.enabled) label += " (off)";

                if (ImGui::Selectable(label.c_str(), selLayer == i, 0, ImVec2(0, 28))) {
                    selLayer = i;
                }
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Duplicate")) {
                        config.layers.push_back(l);
                        modified = true;
                    }
                    if (ImGui::MenuItem("Delete")) {
                        config.layers.erase(config.layers.begin() + i);
                        modified = true;
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        // ─── Right: Properties ─────────────────────────────────
        ImGui::SameLine();
        ImGui::BeginChild("PropPanel", ImVec2(0, 0), false);
        {
            if (selLayer >= 0 && selLayer < (int)config.layers.size()) {
                auto& ly = config.layers[selLayer];

                SectionLabel("LAYER PROPERTIES");

                char nb[64];
                strncpy(nb, ly.name.c_str(), 63); nb[63] = '\0';
                ImGui::PushItemWidth(250);
                if (ImGui::InputText("Name", nb, 64)) {
                    ly.name = nb; modified = true;
                }

                if (ImGui::Checkbox("Enabled", &ly.enabled)) modified = true;

                ImGui::Spacing();
                SectionLabel("SHAPE");

                static const char* shapes[] = {"Cross", "Dot", "Circle"};
                int cs = (int)ly.shape;
                if (ImGui::Combo("Shape", &cs, shapes, 3)) {
                    ly.shape = (ShapeType)cs; modified = true;
                }

                ImGui::Spacing();
                SectionLabel("DIMENSIONS");

                if (ImGui::SliderFloat("Width",  &ly.width,  1.0f, 500.0f)) modified = true;
                if (ImGui::SliderFloat("Height", &ly.height, 1.0f, 500.0f)) modified = true;
                if (ImGui::SliderFloat("Thickness", &ly.thickness, 0.1f, 100.0f)) modified = true;
                if (ly.shape == ShapeType::Cross) {
                    if (ImGui::SliderFloat("Gap", &ly.gap, 0.0f, 200.0f)) modified = true;
                }

                ImGui::Spacing();
                SectionLabel("OFFSET");
                if (ImGui::SliderFloat("X Offset", &ly.offsetX, -500.0f, 500.0f)) modified = true;
                if (ImGui::SliderFloat("Y Offset", &ly.offsetY, -500.0f, 500.0f)) modified = true;

                ImGui::Spacing();
                SectionLabel("COLOR");
                float col[4] = {
                    ly.color.r / 255.0f, ly.color.g / 255.0f,
                    ly.color.b / 255.0f, ly.color.a / 255.0f
                };
                if (ImGui::ColorEdit4("##lcol", col,
                        ImGuiColorEditFlags_AlphaBar |
                        ImGuiColorEditFlags_AlphaPreviewHalf)) {
                    ly.color.r = (int)(col[0]*255);
                    ly.color.g = (int)(col[1]*255);
                    ly.color.b = (int)(col[2]*255);
                    ly.color.a = (int)(col[3]*255);
                    modified = true;
                }
                ImGui::PopItemWidth();
            } else {
                ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.54f, 1.0f),
                    "Add or select a layer to edit.");
            }
        }
        ImGui::EndChild();
    }
};

} // namespace overlayx
