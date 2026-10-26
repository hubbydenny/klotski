#pragma once
#include "../../utilities/imgui/imgui.h"
#include <algorithm>
#include <cmath>
#include <string>
import math;

// uhh this for drag-able items (liek practice & speclist)
namespace menu::overlay {
    inline constexpr float header_height = 28.f;
    inline constexpr float row_height = 22.f;
    inline constexpr float rounding = 0.f;
    inline constexpr ImU32 background = IM_COL32(24, 24, 24, 255);
    inline constexpr ImU32 section_background = IM_COL32(27, 27, 27, 255);
    inline constexpr ImU32 border = IM_COL32(50, 50, 54, 255);
    inline constexpr ImU32 divider = IM_COL32(32, 32, 36, 255);
    inline constexpr ImU32 text = IM_COL32(186, 186, 186, 255);
    inline constexpr ImU32 muted = IM_COL32(140, 140, 140, 255);

    inline std::string fit_text(const char* value, float width)
    {
        std::string result = value ? value : "";
        if (width <= 0.f) return {};
        if (ImGui::CalcTextSize(result.c_str()).x <= width) return result;
        if (ImGui::CalcTextSize("...").x > width) return {};
        while (!result.empty() && ImGui::CalcTextSize((result + "...").c_str()).x > width) {
            auto last = result.size() - 1;
            while (last > 0 && (static_cast<unsigned char>(result[last]) & 0xc0u) == 0x80u) --last;
            result.resize(last);
        }
        return result + "...";
    }

    inline void surface(ImDrawList* draw, ImVec2 pos, ImVec2 size, ImU32 accent)
    {
        const ImVec2 end(pos.x + size.x, pos.y + size.y);
        draw->AddRectFilled(pos, end, IM_COL32(5,5,5,255));
        draw->AddRectFilled(ImVec2(pos.x+1.f,pos.y+1.f), ImVec2(end.x-1.f,end.y-1.f), IM_COL32(45,45,45,255));
        draw->AddRectFilled(ImVec2(pos.x+2.f,pos.y+2.f), ImVec2(end.x-2.f,end.y-2.f), background);
        draw->AddRectFilled(ImVec2(pos.x+3.f,pos.y+3.f), ImVec2(end.x-3.f,pos.y+4.f), accent);
        draw->AddRectFilled(ImVec2(pos.x+3.f,pos.y+4.f), ImVec2(end.x-3.f,pos.y+5.f), (accent & ~IM_COL32_A_MASK) | IM_COL32(0,0,0,100));
    }

    inline void panel(ImDrawList* draw, const char* title, ImVec2 pos, ImVec2 size, ImU32 title_color, ImU32 accent)
    {
        surface(draw,pos,size,accent);
        const auto label = fit_text(title, size.x - 54.f);
        draw->AddText(ImVec2(pos.x+8.f,pos.y+7.f), title_color, label.c_str());
        draw->AddLine(ImVec2(pos.x+3.f,pos.y+header_height-1.f), ImVec2(pos.x+size.x-3.f,pos.y+header_height-1.f), IM_COL32(45,45,45,255));
    }

    inline void row(ImDrawList* draw, ImVec2 pos, float width, const char* label, const char* value, ImU32 value_color = text)
    {
        const auto right = fit_text(value, width * .48f);
        const float right_width = ImGui::CalcTextSize(right.c_str()).x;
        const auto left = fit_text(label, width - right_width - 34.f);
        const float y = pos.y + (row_height - ImGui::GetFontSize()) * .5f;
        draw->AddText(ImVec2(pos.x + 11.f, y), muted, left.c_str());
        draw->AddText(ImVec2(pos.x + width - 11.f - right_width, y), value_color, right.c_str());
    }
    inline void clamp_position(vec2_t& pos, ImVec2 size)
    {
        const auto display = ImGui::GetIO().DisplaySize;
        if (!std::isfinite(pos.x)) pos.x = 12.f;
        if (!std::isfinite(pos.y)) pos.y = 12.f;
        pos.x = std::clamp(pos.x, 0.f, (std::max)(0.f, display.x - size.x));
        pos.y = std::clamp(pos.y, 0.f, (std::max)(0.f, display.y - size.y));
    }

    inline void drag_header(vec2_t& pos, ImVec2 size, bool interactive)
    {
        const auto origin = ImGui::GetWindowPos();
        ImGui::SetCursorScreenPos(origin);
        ImGui::InvisibleButton("##drag_header", ImVec2(size.x, header_height));
        auto storage = ImGui::GetStateStorage();
        const auto x_id = ImGui::GetID("##drag_offset_x");
        const auto y_id = ImGui::GetID("##drag_offset_y");
        const auto mouse = ImGui::GetIO().MousePos;
        if (!interactive || !ImGui::IsMousePosValid(&mouse)) return;
        if (ImGui::IsItemActivated()) {
            storage->SetFloat(x_id, mouse.x - origin.x);
            storage->SetFloat(y_id, mouse.y - origin.y);
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            pos.x = mouse.x - storage->GetFloat(x_id);
            pos.y = mouse.y - storage->GetFloat(y_id);
            clamp_position(pos, size);
        }
    }

    inline void frame(const char* title, ImVec2 pos, ImVec2 size, ImU32 title_color, ImU32 accent)
    {
        panel(ImGui::GetWindowDrawList(), title, pos, size, title_color, accent);
    }
}
