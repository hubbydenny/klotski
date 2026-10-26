#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"

#include "../../source2-sdk/sdk.hpp"
#include "../../source2-sdk/interfaces/interfaces.hpp"
#include "../config/config.hpp"
#include "../config/binds.hpp"
#include "../misc/misc.hpp"
#include "overlay.hpp"
#include "../../utilities/luasys/scripts.hpp"

#include "../../utilities/imgui/imgui.h"
#include "../../utilities/imgui/imgui_internal.h"
#include "../../utilities/imgui/imgui_impl_win32.h"
#include "../../utilities/imgui/imgui_impl_dx11.h"

#include <cmath>
#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <vector>

bool menu::open = true;

struct menu_style_scope {
    int colors{}, variables{};
    void color(ImGuiCol id, ImVec4 value) { ImGui::PushStyleColor(id, value); ++colors; }
    void var(ImGuiStyleVar id, float value) { ImGui::PushStyleVar(id, value); ++variables; }
    void var(ImGuiStyleVar id, ImVec2 value) { ImGui::PushStyleVar(id, value); ++variables; }
    ~menu_style_scope() { ImGui::PopStyleColor(colors); ImGui::PopStyleVar(variables); }
};

static char config_name_buffer[128] = "";
static int selected_config = -1;
static std::vector<std::string> config_files;

static ImVec4 accent_color(float brightness = 1.f, float alpha = 1.f)
{
    const auto& color = config::context.menu_accent;
    return ImVec4(ImClamp(color[0] * brightness, 0.f, 1.f), ImClamp(color[1] * brightness, 0.f, 1.f),
        ImClamp(color[2] * brightness, 0.f, 1.f), alpha);
}

static ImU32 accent_packed(float brightness = 1.f)
{
    return ImGui::GetColorU32(accent_color(brightness));
}

static bool obelus_tab_button(const char* label, int* selected, int index, int total)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    const ImVec2 pos = window->Pos + ImVec2(12.f + ((window->Size.x - 24.f) / total) * index, 32.f);
    const ImVec2 size((window->Size.x - 29.f) / total, 28.f);
    const ImRect bounds(pos, pos + size);
    ImGui::ItemSize(size, ImGui::GetStyle().FramePadding.y);
    if (!ImGui::ItemAdd(bounds, id)) return false;

    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bounds, id, &hovered, &held);
    const ImU32 top = IM_COL32(41, 41, 41, 255);
    const ImU32 bottom = *selected == index ? IM_COL32(24, 24, 24, 255) : IM_COL32(15, 15, 15, 255);
    window->DrawList->AddRectFilledMultiColor(pos, pos + size, top, top, bottom, bottom);
    window->DrawList->AddRect(pos - ImVec2(1.f, 1.f), pos + size + ImVec2(1.f, 1.f), IM_COL32(1, 1, 1, 255));
    window->DrawList->AddRect(pos, pos + size, IM_COL32(41, 41, 41, 255));
    const ImU32 text = *selected == index ? accent_packed() : hovered ? IM_COL32(185, 185, 185, 255) : IM_COL32(140, 140, 140, 255);
    window->DrawList->AddText(ImVec2(pos.x + (size.x - label_size.x) * .5f, pos.y + (size.y - label_size.y) * .5f - 1.f), text, label);
    if (pressed) *selected = index;
    return pressed;
}

static bool menu_button(const char* label, ImVec2 size = ImVec2(150.f, 20.f))
{
    if (size.y <= 0.f) size.y = 22.f;
    size.x = ImMax(1.f, ImMin(size.x, ImGui::GetContentRegionAvail().x));
    const auto pos = ImGui::GetCursorScreenPos();
    const bool pressed = ImGui::InvisibleButton(label, size);
    auto draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(ImVec2(pos.x - 1.f, pos.y - 1.f), ImVec2(pos.x + size.x + 1.f, pos.y + size.y + 1.f), IM_COL32(0, 0, 0, 255));
    const ImU32 top = ImGui::IsItemHovered() ? IM_COL32(41,41,41,255) : IM_COL32(31,31,31,255);
    const ImU32 bottom = ImGui::IsItemHovered() ? IM_COL32(24,24,24,255) : IM_COL32(15,15,15,255);
    draw->AddRectFilledMultiColor(pos, pos + size, top, top, bottom, bottom);
    draw->AddRect(pos, pos + size, IM_COL32(41,41,41,255));
    draw->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
    draw->AddText(ImVec2(pos.x + (size.x - ImGui::CalcTextSize(label).x) * .5f, pos.y + (size.y - ImGui::GetFontSize()) * .5f), ImGui::GetColorU32(ImGuiCol_Text), label);
    draw->PopClipRect();
    return pressed;
}

static void refresh_config_files()
{
	config_files = config::list_files();

	if (selected_config >= static_cast<int>(config_files.size()))
	{
		selected_config = -1;
	}
}

static bool color_picker(const char* label, float color[4], bool alpha, bool compact = false)
{
    ImGui::PushID(label);
    const auto pos = ImGui::GetCursorScreenPos();
    if (!compact) ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 16.f));
    const float swatch_offset = compact ? 0.f : ImMax(0.f, ImGui::GetContentRegionAvail().x - 26.f);
    auto draw = ImGui::GetWindowDrawList();
    if (!compact) {
        draw->AddText(ImVec2(pos.x + 21.f, pos.y + 1.f), IM_COL32(0, 0, 0, 255), label);
        draw->AddText(ImVec2(pos.x + 20.f, pos.y), ImGui::GetColorU32(ImGuiCol_Text), label);
    }
    ImGui::SetCursorScreenPos(ImVec2(pos.x + swatch_offset, pos.y + 3.f));
    const bool pressed = ImGui::InvisibleButton("##swatch", ImVec2(20.f, 8.f));
    draw->AddRectFilled(ImVec2(pos.x + swatch_offset - 1.f, pos.y + 2.f), ImVec2(pos.x + swatch_offset + 21.f, pos.y + 12.f), IM_COL32(0, 0, 0, 255));
    ImGui::RenderColorRectWithAlphaCheckerboard(draw, ImVec2(pos.x + swatch_offset, pos.y + 3.f),
        ImVec2(pos.x + swatch_offset + 20.f, pos.y + 11.f),
        ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], alpha ? color[3] : 1.f)), 4.f, ImVec2(0.f, 0.f));
    if (!compact) ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + 19.f));
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", label);
    auto storage = ImGui::GetStateStorage();
    const ImGuiID reference_id = ImGui::GetID("##reference");
    if (pressed) {
        for (int i = 0; i < 4; ++i) storage->SetFloat(reference_id + i, color[i]);
        ImGui::OpenPopup("##picker");
    }
    bool changed = false;
    ImGui::SetNextWindowPos(ImVec2(pos.x + swatch_offset, pos.y + 13.f), ImGuiCond_Appearing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(9.f / 255.f, 9.f / 255.f, 9.f / 255.f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(44.f / 255.f, 44.f / 255.f, 44.f / 255.f, 1.f));
    if (ImGui::BeginPopup("##picker"))
    {
        float reference[4];
        for (int i = 0; i < 4; ++i) reference[i] = storage->GetFloat(reference_id + i, color[i]);
        ImGui::TextUnformatted(label);
        ImGui::Separator();
        ImGui::SetNextItemWidth(180.f);
        const ImGuiColorEditFlags flags = ImGuiColorEditFlags_PickerHueBar | ImGuiColorEditFlags_NoOptions
            | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_InputRGB
            | (alpha ? ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf : ImGuiColorEditFlags_NoAlpha);
        changed = ImGui::ColorPicker4("##rgba", color, flags, reference);
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
    ImGui::PopID();
    return changed;
}

bool menu::color_edit4(const char* label, float (&color)[4])
{
	return color_picker(label, color, true);
}
bool menu::color_edit4(const char* label, float (&color)[3], float* alpha)
{
	float rgba[4] = { color[0], color[1], color[2], alpha ? *alpha : 1.f };
	if (!color_picker(label, rgba, alpha != nullptr)) return false;
	for (int i = 0; i < 3; i++) color[i] = rgba[i];
	if (alpha) *alpha = rgba[3];
	return true;
}


template <std::size_t count>
static void inline_color(const char* id, float (&color)[count], float* alpha = nullptr)
{
    static_assert(count == 3 || count == 4);
    if constexpr (count == 4) {
        if (!alpha) alpha = &color[3];
    }
    const auto next = ImGui::GetCursorScreenPos();
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 26.f);
    float rgba[4] = { color[0], color[1], color[2], alpha ? *alpha : 1.f };
    if (color_picker(id, rgba, alpha != nullptr, true)) {
        for (int i = 0; i < 3; ++i) color[i] = rgba[i];
        if (alpha) *alpha = rgba[3];
    }
    ImGui::SetCursorScreenPos(next);
}

template <typename entry_t, std::size_t count>
static const char* multi_select_preview(const entry_t (&entries)[count], const bool* values)
{
	static char buffer[128] = { };

	std::size_t length = 0;

	buffer[0] = '\0';

	for (std::size_t i = 0; i < count; i++)
	{
		if (!values[i]) continue;

		const char* label = entries[i].label;
		const std::size_t label_length = std::strlen(label);

		if (length + label_length + 3 >= sizeof(buffer)) break;

		if (length != 0)
		{
			buffer[length++] = ',';
			buffer[length++] = ' ';
		}

		std::memcpy(buffer + length, label, label_length);
		length += label_length;
		buffer[length] = '\0';
	}

	return length == 0 ? "none" : buffer;
}

static bool menu_combo_begin(const char* label, const char* preview)
{
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    const auto pos = ImGui::GetCursorScreenPos();
    const float width = ImMax(40.f, ImMin(ImGui::CalcItemWidth(), ImGui::GetContentRegionAvail().x));
    const float height = 20.f;
    const bool pressed = ImGui::InvisibleButton("##dropdown", ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    if (pressed) ImGui::OpenPopup("##choices");
    const bool expanded = ImGui::IsPopupOpen("##choices");
    auto draw = ImGui::GetWindowDrawList();
    draw->AddRectFilledMultiColor(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(41,41,41,255), IM_COL32(41,41,41,255), IM_COL32(15,15,15,255), IM_COL32(15,15,15,255));
    draw->AddRect(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(1,1,1,255));
    draw->AddRect(ImVec2(pos.x+1.f,pos.y+1.f), ImVec2(pos.x+width-1.f,pos.y+height-1.f), expanded ? accent_packed(.7f) : hovered ? IM_COL32(65,65,65,255) : IM_COL32(41,41,41,255));
    draw->AddLine(ImVec2(pos.x+width-23.f,pos.y+3.f), ImVec2(pos.x+width-23.f,pos.y+height-3.f), IM_COL32(45,45,45,255));
    draw->PushClipRect(pos, ImVec2(pos.x + width - 19.f, pos.y + height), true);
    draw->AddText(ImVec2(pos.x + 8.f, pos.y + (height-ImGui::GetFontSize())*.5f), IM_COL32(180,180,180,255), preview);
    draw->PopClipRect();
    const float arrow = expanded ? -1.f : 1.f;
    draw->AddTriangleFilled(ImVec2(pos.x+width-15.f,pos.y+height*.5f-2.f*arrow),
        ImVec2(pos.x+width-7.f,pos.y+height*.5f-2.f*arrow),
        ImVec2(pos.x+width-11.f,pos.y+height*.5f+2.f*arrow), expanded ? accent_packed() : IM_COL32(153,153,153,255));
    ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + height + 3.f));
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.f), ImVec2(width, 300.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f,4.f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f,2.f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(19.f/255.f,19.f/255.f,19.f/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(45.f/255.f,45.f/255.f,45.f/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(32.f/255.f,32.f/255.f,32.f/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(41.f/255.f,41.f/255.f,41.f/255.f,1.f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(48.f/255.f,48.f/255.f,48.f/255.f,1.f));
    if (ImGui::BeginPopup("##choices")) return true;
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(3);
    ImGui::PopID();
    return false;
}

static void menu_combo_end()
{
    ImGui::EndPopup();
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(3);
    ImGui::PopID();
}

static bool menu_combo_item(const char* label, bool selected, ImGuiSelectableFlags flags = 0)
{
    ImGui::PushStyleColor(ImGuiCol_Text, selected ? accent_color() : ImVec4(180.f/255.f,180.f/255.f,180.f/255.f,1.f));
    const bool pressed = ImGui::Selectable(label, selected, flags, ImVec2(0.f,20.f));
    if (selected) {
        const auto pos = ImGui::GetItemRectMin();
        const auto end = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(end.x-3.f,pos.y+5.f),ImVec2(end.x-1.f,end.y-5.f),accent_packed());
    }
    ImGui::PopStyleColor();
    return pressed;
}

static bool menu_combo(const char* label, int* selected, const char* const* items, int count)
{
    bool changed = false;
    if (!menu_combo_begin(label, *selected >= 0 && *selected < count ? items[*selected] : "none")) return false;
    for (int i = 0; i < count; ++i) {
        if (menu_combo_item(items[i], *selected == i)) { *selected = i; changed = true; }
        if (*selected == i) ImGui::SetItemDefaultFocus();
    }
    menu_combo_end();
    return changed;
}

static bool menu_combo(const char* label, int* selected, const char* items)
{
    std::vector<const char*> entries;
    for (auto item = items; *item; item += std::strlen(item) + 1) entries.push_back(item);
    return menu_combo(label, selected, entries.data(), static_cast<int>(entries.size()));
}

template <typename entry_t, std::size_t count>
static void multi_select_combo(const char* label, const entry_t (&entries)[count], bool* values)
{
	if (!menu_combo_begin(label, multi_select_preview(entries, values))) return;

	for (std::size_t i = 0; i < count; i++)
	{
		if (menu_combo_item(entries[i].label, values[i], ImGuiSelectableFlags_DontClosePopups))
		{
			values[i] = !values[i];
		}
	}

	menu_combo_end();
}

const char* menu::key_to_string(unsigned int key)
{
	static char buffer[16];

	if (!key) return "none";
	if (key >= VK_F1 && key <= VK_F12)
	{
		sprintf_s(buffer, sizeof(buffer), "F%d", key - VK_F1 + 1);
		return buffer;
	}

	if (key >= '0' && key <= '9' || key >= 'A' && key <= 'Z')
	{
		buffer[0] = static_cast<char>(key);
		buffer[1] = '\0';
		return buffer;
	}

	switch (key) {
	case VK_SPACE: return "space";
	case VK_SHIFT: return "shift";
	case VK_CONTROL: return "ctrl";
	case VK_MENU: return "alt";
	case VK_INSERT: return "insert";
	case VK_DELETE: return "delete";
	case VK_HOME: return "home";
	case VK_CAPITAL: return "caps";
	case VK_END: return "end";
	case VK_PRIOR: return "pgup";
	case VK_NEXT: return "pgdn";
	case VK_LBUTTON:  return "mouse1";
	case VK_RBUTTON:  return "mouse2";
	case VK_MBUTTON:  return "mouse3";
	case VK_XBUTTON1: return "mouse4";
	case VK_XBUTTON2: return "mouse5";
	default: break;
	}

	sprintf_s(buffer, sizeof(buffer), "0x%02X", key);
	return buffer;
}

static void practice_keybind(const char* label, int& key)
{
	static int* capturing = nullptr;
	static bool ready = false;
	static bool previous[256]{};
	ImGui::PushID(label);
	ImGui::TextUnformatted(label);
	if (menu_button(capturing == &key ? "press key" : menu::key_to_string(key)))
	{
		capturing = capturing == &key ? nullptr : &key;
		ready = false;
		for (int i = 1; i < 256; i++) previous[i] = binds::key_down(i);
	}
	if (capturing == &key)
	{
		if (!binds::key_down(VK_LBUTTON)) ready = true;
		for (int i = 1; i < 256; i++)
		{
			const bool down = binds::key_down(i);
			if (ready && down && !previous[i])
			{
				const bool mouse = i == VK_LBUTTON || i == VK_RBUTTON || i == VK_MBUTTON || i == VK_XBUTTON1 || i == VK_XBUTTON2;
				if (!mouse || !ImGui::IsAnyItemHovered())
				{
					key = i == VK_ESCAPE ? 0 : i;
					capturing = nullptr;
				}
			}
			previous[i] = down;
			if (!capturing) break;
		}
	}
	ImGui::PopID();
}

static const char* bind_modes[] = { "hold", "toggle", "always" };
static const char* flag_positions[] = { "bottom", "right" };
static const char* chams_materials[] = { "flat", "glow", "bloom", "hologram", "textured", "illuminate", "metallic", "pearlescent" };

static bool menu_checkbox(const char* label, bool* value);
static void bind_checkbox(const char* name, const char* label, bool* value) {
	static std::array<bool, 256> capture_keys{};
	menu_checkbox(label, value);

	binds::bind_t& bind = binds::list[binds::get(name, value)];

	ImGui::PushID(name);

	const char* text = bind.capturing ? "..." : menu::key_to_string(bind.key);

	ImGui::Indent(12.f);
	const bool clicked = menu_button(text);
	const bool hovered = ImGui::IsItemHovered();
	if (clicked)
	{
		for (auto& other : binds::list)
		{
			if (&other == &bind) continue;
			other.capturing = false;
			other.capture_ready = false;
		}
		bind.capturing = !bind.capturing;
		bind.capture_ready = false;
		for (std::size_t i = 1; i < capture_keys.size(); ++i) capture_keys[i] = binds::key_down(static_cast<std::uint32_t>(i));
	}

	const bool capture_active = bind.capturing;
	if (bind.capturing && !bind.capture_ready && !binds::key_down(VK_LBUTTON)) bind.capture_ready = true;
	if (bind.capturing && bind.capture_ready)
	{
		for (std::uint32_t vkey = 1; vkey < 255; vkey++)
		{
			const bool mouse = vkey == VK_LBUTTON || vkey == VK_RBUTTON || vkey == VK_MBUTTON || vkey == VK_XBUTTON1 || vkey == VK_XBUTTON2;
			const bool down = binds::key_down(vkey);
			const bool pressed = vkey == VK_LBUTTON ? ImGui::IsMouseClicked(ImGuiMouseButton_Left) : vkey == VK_RBUTTON ? ImGui::IsMouseClicked(ImGuiMouseButton_Right) : vkey == VK_MBUTTON ? ImGui::IsMouseClicked(ImGuiMouseButton_Middle) : down && !capture_keys[vkey];
			capture_keys[vkey] = down;
			if (pressed)
			{
				if (mouse && ImGui::IsAnyItemHovered()) continue;
				bind.key = (vkey == VK_ESCAPE) ? 0 : vkey;
				bind.previous = binds::key_down(bind.key);
				bind.capturing = false;
				bind.capture_ready = false;
				break;
			}
		}
	}

	if (!capture_active && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) ImGui::OpenPopup("mode");
	if (ImGui::BeginPopup("mode"))
	{
		for (int m = 0; m < binds::bind_mode_count; m++)
		{
			if (ImGui::Selectable(bind_modes[m], bind.mode == m))
			{
				bind.mode = m;
				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::EndPopup();
	}
	ImGui::PopID();
	ImGui::Unindent(12.f);
}


static bool menu_checkbox(const char* label, bool* value)
{
    auto window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;
    const ImGuiID id = window->GetID(label);
    const ImVec2 pos = window->DC.CursorPos;
    const ImVec2 text = ImGui::CalcTextSize(label, nullptr, true);
    const float height = ImGui::GetFontSize();
    const ImRect bounds(pos, ImVec2(pos.x + text.x + 23.f, pos.y + height));
    ImGui::ItemSize(bounds);
    if (!ImGui::ItemAdd(bounds, id)) return false;
    bool hovered = false, held = false;
    const bool pressed = ImGui::ButtonBehavior(bounds, id, &hovered, &held);
    if (pressed) { *value = !*value; ImGui::MarkItemEdited(id); }
    const ImVec2 box(pos.x + 2.f, pos.y + 2.f);
    const ImU32 top = *value ? accent_packed() : hovered ? IM_COL32(79,79,79,255) : IM_COL32(69,69,69,255);
    const ImU32 bottom = *value ? IM_COL32(30,30,30,255) : hovered ? IM_COL32(46,46,46,255) : IM_COL32(36,36,36,255);
    window->DrawList->AddRectFilledMultiColor(box, box + ImVec2(8.f,8.f), top, top, bottom, bottom);
    window->DrawList->AddRect(box, box + ImVec2(8.f,8.f), IM_COL32(1,1,1,255));
    ImGui::RenderText(ImVec2(pos.x + 22.f, pos.y - 1.f), label);
    ImGui::RenderNavHighlight(bounds, id);
    return pressed;
}

static bool menu_slider(const char* label, void* value, ImGuiDataType type, const void* minimum, const void* maximum, const char* format)
{
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    const auto pos = ImGui::GetCursorScreenPos();
    const float width = ImMax(40.f, ImMin(ImGui::CalcItemWidth(), ImGui::GetContentRegionAvail().x - 52.f));
    static ImGuiID editing_id = 0;
    static int editing_frame = -1;
    static int editing_int = 0;
    static float editing_float = 0.f;
    const ImGuiID slider_id = ImGui::GetID("##slider");
    const int frame = ImGui::GetFrameCount();
    if (editing_id && frame > editing_frame + 1) editing_id = 0;
    const bool is_float = type == ImGuiDataType_Float;
    const double original = is_float ? *static_cast<float*>(value) : *static_cast<int*>(value);
    char preview[64]{};
    if (is_float) ImFormatString(preview, sizeof(preview), format, original);
    else ImFormatString(preview, sizeof(preview), format, static_cast<int>(original));
    const float hit_width = width + 8.f + ImGui::CalcTextSize(preview).x;
    bool focus_input = false;
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
        ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + hit_width, pos.y + 16.f)) &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        editing_id = slider_id;
        editing_frame = frame;
        if (is_float) editing_float = *static_cast<float*>(value);
        else editing_int = *static_cast<int*>(value);
        if (GImGui->ActiveId == slider_id) ImGui::ClearActiveID();
        focus_input = true;
    }
    if (editing_id == slider_id) {
        editing_frame = frame;
        if (focus_input) ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(width);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.f, ImMax(0.f, (16.f - ImGui::GetFontSize()) * 0.5f)));
        void* temporary = is_float ? static_cast<void*>(&editing_float) : static_cast<void*>(&editing_int);
        const bool enter = ImGui::InputScalar("##value", type, temporary, nullptr, nullptr,
            is_float ? "%.6f" : "%d", ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        const bool cancel = ImGui::IsKeyPressed(ImGuiKey_Escape);
        const bool leave = !focus_input && ImGui::IsItemDeactivated();
        ImGui::PopStyleVar();
        bool changed = false;
        if (!cancel && (enter || leave)) {
            if (is_float && std::isfinite(editing_float)) {
                const float next = ImClamp(editing_float, *static_cast<const float*>(minimum), *static_cast<const float*>(maximum));
                changed = next != *static_cast<float*>(value);
                *static_cast<float*>(value) = next;
            } else if (!is_float) {
                const int next = ImClamp(editing_int, *static_cast<const int*>(minimum), *static_cast<const int*>(maximum));
                changed = next != *static_cast<int*>(value);
                *static_cast<int*>(value) = next;
            }
        }
        if (cancel || enter || leave) editing_id = 0;
        ImGui::PopID();
        return changed;
    }
    ImGui::InvisibleButton("##slider", ImVec2(width, 16.f));
    const double low = type == ImGuiDataType_Float ? *static_cast<const float*>(minimum) : *static_cast<const int*>(minimum);
    const double high = type == ImGuiDataType_Float ? *static_cast<const float*>(maximum) : *static_cast<const int*>(maximum);
    const double old = type == ImGuiDataType_Float ? *static_cast<float*>(value) : *static_cast<int*>(value);
    double next = old;
    bool interacting = false;
    if (ImGui::IsItemActive() && ImGui::IsMouseDown(0) && high > low) {
        next = low + ImClamp((ImGui::GetIO().MousePos.x - pos.x) / width, 0.f, 1.f) * (high - low);
        interacting = true;
    }
    if (ImGui::IsItemFocused() && (GImGui->ActiveId == 0 || GImGui->ActiveId == ImGui::GetItemID())) {
        const double step = type == ImGuiDataType_Float ? (high - low) / 100.0 : 1.0;
		if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) { next -= step; interacting = true; }
		if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { next += step; interacting = true; }
    }
    if (interacting) {
        next = ImClamp(next, low, high);
        if (type == ImGuiDataType_Float) *static_cast<float*>(value) = static_cast<float>(next);
        else *static_cast<int*>(value) = static_cast<int>(std::round(next));
    }
    const double current = type == ImGuiDataType_Float ? *static_cast<float*>(value) : *static_cast<int*>(value);
    const float fraction = high > low ? ImClamp(static_cast<float>((current - low) / (high - low)), 0.f, 1.f) : 0.f;
    auto draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + 7.f), IM_COL32(0, 0, 0, 255));
    draw->AddRectFilledMultiColor(ImVec2(pos.x + 1.f, pos.y + 1.f), ImVec2(pos.x + width - 1.f, pos.y + 6.f),IM_COL32(69, 69, 69, 255), IM_COL32(69, 69, 69, 255), IM_COL32(44, 44, 44, 255), IM_COL32(44, 44, 44, 255));
    if (fraction > 0.f) draw->AddRectFilledMultiColor(ImVec2(pos.x + 1.f, pos.y + 1.f), ImVec2(pos.x + 1.f + (width - 2.f) * fraction, pos.y + 6.f), accent_packed(), accent_packed(), accent_packed(0.58f), accent_packed(0.58f));
    char buffer[64]{};
    if (type == ImGuiDataType_Float) ImFormatString(buffer, sizeof(buffer), format, current);
    else ImFormatString(buffer, sizeof(buffer), format, static_cast<int>(current));
    const float value_width = ImGui::CalcTextSize(buffer).x;
    const float value_x = ImClamp(pos.x + width * fraction - value_width * .5f, pos.x, pos.x + width - value_width);
    draw->AddText(ImVec2(value_x + 1.f, pos.y - 3.f), IM_COL32(0,0,0,255), buffer);
    draw->AddText(ImVec2(value_x, pos.y - 4.f), ImGui::GetColorU32(ImGuiCol_Text), buffer);
    if (current != old) ImGui::MarkItemEdited(ImGui::GetItemID());
    ImGui::PopID();
    return current != old;
}

static bool menu_slider_float(const char* label, float* value, float minimum, float maximum, const char* format = "%.3f") {
    return menu_slider(label, value, ImGuiDataType_Float, &minimum, &maximum, format);
}

static bool menu_slider_int(const char* label, int* value, int minimum, int maximum, const char* format = "%d")
{
    return menu_slider(label, value, ImGuiDataType_S32, &minimum, &maximum, format);
}

static ImVec2 group_start{};
static const char* group_title = nullptr;
static float group_width = 0.f;
static float menu_panel_height = 540.f;
static int menu_column_index = 0;
static bool column_has_group = false;
static constexpr float obelus_window_width = 525.f;
static constexpr float obelus_window_height = 563.f;
static constexpr float obelus_panel_margin = 26.f;
static constexpr float obelus_panel_top = 79.f;

static void menu_group_end()
{
    if (!group_title) return;
    group_title = nullptr;
    ImGui::PopID();
    ImGui::Dummy(ImVec2(0.f, 14.f));
}

static void menu_group(const char* title)
{
    menu_group_end();
    if (!column_has_group) {
        column_has_group = true;
        auto window = ImGui::GetCurrentWindow();
        auto draw = window->DrawList;
        draw->PushClipRect(window->Pos - ImVec2(0.f, 10.f), window->Pos + window->Size, false);
        const float width = ImGui::CalcTextSize(title).x;
        draw->AddRectFilled(window->Pos + ImVec2(10.f, -7.f), window->Pos + ImVec2(17.f + width, 9.f), IM_COL32(13,13,13,255));
        draw->AddText(window->Pos + ImVec2(13.f, 2.f - ImGui::GetFontSize() * .5f), IM_COL32(205,205,205,255), title);
        draw->PopClipRect();
        group_title = title;
        ImGui::PushID(title);
        return;
    }
    group_start = ImGui::GetCursorScreenPos();
    group_width = ImMax(40.f, ImGui::GetContentRegionAvail().x);
    group_title = title;
    ImGui::PushID(title);
    auto draw = ImGui::GetWindowDrawList();
    const float title_width = ImGui::CalcTextSize(title).x;
    draw->AddText(group_start, menu::overlay::muted, title);
    const float line_start = group_start.x + title_width + 10.f;
    const float right = group_start.x + group_width;
    if (line_start < right) draw->AddLine(ImVec2(line_start, group_start.y + 7.f), ImVec2(right, group_start.y + 7.f), menu::overlay::divider);
    ImGui::Dummy(ImVec2(0.f, ImGui::GetFontSize() + 9.f));
}

static void menu_column(const char* id)
{
    const float width = ImGui::GetWindowSize().x * .5f - 32.f;
    const float x = menu_column_index++ == 0 ? 26.f : ImGui::GetWindowSize().x * .5f + 6.f;
    ImGui::SetCursorPos(ImVec2(x, 79.f));
    column_has_group = false;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.f, 16.f));
	ImGui::BeginChild(id, ImVec2(width, menu_panel_height), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    ImGui::PushItemWidth(ImMax(80.f, ImGui::GetContentRegionAvail().x - 44.f));

}

static void menu_column_end()
{
    menu_group_end();
    ImGui::PopItemWidth();
    ImGui::EndChild();
}

void menu::render() {
    binds::get("aimbot", &config::context.aimbot);
    binds::get("players", &config::context.player_esp);
    binds::get("ej", &config::context.ej);
    binds::get("lj", &config::context.longjump);
    binds::get("mj", &config::context.mj);
    binds::get("ladder_ej", &config::context.ladder_ej);
    binds::get("ladderbug", &config::context.ladder_bug);
    binds::get("edgebug", &config::context.edgebug);
    binds::get("jumpbug", &config::context.jumpbug);
    binds::get("autobounce", &config::context.autobounce);
    binds::get("visualize", &config::context.visualize_edgebug);
	if (!menu::open) return;

	if (menu::font) ImGui::PushFont(menu::font);
    menu_style_scope style;
	style.var(ImGuiStyleVar_WindowMinSize, ImVec2(obelus_window_width, obelus_window_height));
	style.var(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	style.var(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 5.f));
	style.var(ImGuiStyleVar_FramePadding, ImVec2(7.f, 3.f));
	style.var(ImGuiStyleVar_WindowRounding, 0.f);
	style.var(ImGuiStyleVar_ChildRounding, 0.f);
	style.var(ImGuiStyleVar_FrameRounding, 0.f);
	style.var(ImGuiStyleVar_FrameBorderSize, 1.f);
	style.var(ImGuiStyleVar_ScrollbarSize, 0.f);
	style.color(ImGuiCol_WindowBg, ImVec4(4.f / 255.f, 4.f / 255.f, 4.f / 255.f, 1.f));
	style.color(ImGuiCol_ChildBg, ImVec4(19.f / 255.f, 19.f / 255.f, 19.f / 255.f, 1.f));
    style.color(ImGuiCol_Text, ImVec4(180.f / 255.f,180.f / 255.f,180.f / 255.f,1.f));
	style.color(ImGuiCol_Border, ImVec4(41.f / 255.f, 41.f / 255.f, 41.f / 255.f, 1.f));
	style.color(ImGuiCol_FrameBg, ImVec4(0.075f, 0.075f, 0.09f, 1.f));
	style.color(ImGuiCol_FrameBgHovered, ImVec4(0.12f, 0.12f, 0.15f, 1.f));
	style.color(ImGuiCol_FrameBgActive, accent_color(0.25f));
	style.color(ImGuiCol_Header, accent_color(0.7f, 0.85f));
	style.color(ImGuiCol_HeaderHovered, accent_color(0.9f, 0.95f));
	style.color(ImGuiCol_Tab, ImVec4(0.055f, 0.055f, 0.07f, 1.f));
	style.color(ImGuiCol_TabHovered, accent_color(0.8f, 0.8f));
	style.color(ImGuiCol_TabActive, accent_color());
	style.color(ImGuiCol_CheckMark, accent_color(1.2f));
	style.color(ImGuiCol_SliderGrab, accent_color());
	style.color(ImGuiCol_SliderGrabActive, accent_color(1.2f));

	const auto display = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowSizeConstraints(ImVec2(obelus_window_width, obelus_window_height), ImVec2((std::max)(obelus_window_width, display.x - 16.f), (std::max)(obelus_window_height, display.y - 16.f)));
	ImGui::SetNextWindowSize(ImVec2(obelus_window_width, obelus_window_height), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("klotski", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{

        static int selected_tab = 0;
        const char* tabs[] = { "combat", "visuals", "movement", "misc", "configs", "LUA" };
        auto draw = ImGui::GetWindowDrawList();
        const auto pos = ImGui::GetWindowPos();
        const auto size = ImGui::GetWindowSize();
        draw->PushClipRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), false);
        draw->AddRectFilled(pos, pos + size, IM_COL32(12,12,12,255));
        draw->AddRect(pos, pos + size, IM_COL32(0,0,0,255));
        draw->AddRect(pos + ImVec2(1.f,1.f), pos + size - ImVec2(1.f,1.f), IM_COL32(48,48,48,255));
        draw->AddText(pos + ImVec2(11.f,8.f), accent_packed(), "klotski");
        const float title_width = ImGui::CalcTextSize("klotski").x;
        draw->AddText(pos + ImVec2(11.f + title_width,8.f), IM_COL32(135,135,135,255), " | ");
        draw->AddText(pos + ImVec2(11.f + title_width + ImGui::CalcTextSize(" | ").x,8.f), IM_COL32(142,142,142,255), __DATE__);
        draw->AddRect(pos + ImVec2(11.f,24.f), pos + ImVec2(size.x - 12.f,26.f), accent_packed());
        draw->AddRect(pos + ImVec2(11.f,25.f), pos + ImVec2(size.x - 12.f,26.f), IM_COL32(0,0,0,58));
        draw->AddRectFilled(pos + ImVec2(11.f,67.f), pos + ImVec2(size.x - 12.f,size.y - 13.f), IM_COL32(13,13,13,255));
        draw->AddRect(pos + ImVec2(11.f,67.f), pos + ImVec2(size.x - 12.f,size.y - 13.f), IM_COL32(0,0,0,255));
        draw->AddRect(pos + ImVec2(12.f,68.f), pos + ImVec2(size.x - 13.f,size.y - 14.f), IM_COL32(51,51,51,255));
        draw->PopClipRect();
        for (int i = 0; i < IM_ARRAYSIZE(tabs); ++i)
        {
            ImGui::PushID(i);
            if (obelus_tab_button(tabs[i], &selected_tab, i, IM_ARRAYSIZE(tabs))) {
                for (auto& bind : binds::list) { bind.capturing = false; bind.capture_ready = false; }
                ImGui::ClearActiveID();
            }
            ImGui::PopID();
        }
		menu_panel_height = size.y - obelus_panel_top - 27.f;
		ImGui::SetCursorPos(ImVec2(obelus_panel_margin, obelus_panel_top));
        menu_column_index = 0;
        {
			if (selected_tab == 0)
			{
				menu_column("aimbot");
                menu_group("aimbot");
				bind_checkbox("aimbot", "aimbot", &config::context.aimbot);
                if (config::context.aimbot) {
					menu_slider_float("fov", &config::context.aimbot_fov, 1.f, 180.f, "%.1f");
					menu_slider_float("max distance", &config::context.aimbot_max_distance, 0.f, 5000.f, "%.0f");
					menu_checkbox("visible only", &config::context.onlyvisible);
					menu_checkbox("silent", &config::context.silent_aim);
                    menu_checkbox("no spread", &config::context.no_spread);
					multi_select_combo("aim bones", aim_bones_table, config::context.aim_bones);
					ImGui::Spacing();
					menu_checkbox("smooth", &config::context.aimbot_smooth);

					if (config::context.aimbot_smooth)
					{
						menu_slider_float("smooth speed", &config::context.aimbot_speed, 1.f, 100.f, "%.0f");
					}

					ImGui::Spacing();
                }
                menu_column_end();
                menu_column("combat_right");
                menu_group("recoil control");
                menu_checkbox("rcs", &config::context.standalone_rcs);
					if (config::context.standalone_rcs)
					{
						menu_slider_int("strength", &config::context.standalone_rcs_strength, 0, 100);
						menu_slider_int("rcs min", &config::context.standalone_rcs_min, 0, 100);
						menu_slider_int("rcs max", &config::context.standalone_rcs_max, 0, 100);
					}
                menu_column_end();
			}

			if (selected_tab == 1)
			{
				menu_column("player esp / chams");
                menu_group("player esp");
				bind_checkbox("players", "players", &config::context.player_esp);
				
				if (config::context.player_esp) {
					menu_checkbox("teammates", &config::context.teammate_esp);
                    inline_color("teammate color", config::context.esp_colors_teammate);
					ImGui::Spacing();
                    ImGui::TextDisabled("box");

					menu_checkbox("draw box", &config::context.draw_box);
                    inline_color("draw box color", config::context.esp_box_color, &config::context.esp_box_alpha);

					if (config::context.draw_box)
					{
						menu_checkbox("corner", &config::context.box_corner);

						if (config::context.box_corner)
						{
							menu_slider_float("corner size", &config::context.box_corner_size, 1.f, 50.f, "%.0f%%");
						}

						menu_slider_float("thickness", &config::context.box_thickness, 1.f, 6.f, "%.0f");
						menu_checkbox("outline", &config::context.box_outline);
						menu_checkbox("fill", &config::context.box_fill);

						if (config::context.box_fill)
						{
							menu_slider_float("fill alpha", &config::context.box_fill_alpha, 0.01f, 1.f, "%.2f");
						}
					}

					ImGui::Spacing();
                    ImGui::TextDisabled("elements");

					menu_checkbox("name", &config::context.draw_name);
                    inline_color("name color", config::context.esp_name_color, &config::context.esp_name_alpha);
					menu_checkbox("health", &config::context.draw_health);
                    inline_color("health color", config::context.esp_health_color, &config::context.esp_health_alpha);
					if (config::context.draw_health)
					{
						menu_checkbox("smooth health", &config::context.health_smooth);
						menu_checkbox("gradient health", &config::context.health_gradient);
						menu_checkbox("custom health color", &config::context.health_custom_color);
						menu_checkbox("health text", &config::context.health_text);
						if (config::context.health_text)
						{
							menu_combo("health text position", &config::context.health_text_position, "below box\0beside bar\0");
							menu_checkbox("hp suffix", &config::context.health_suffix);
						}
					}
					menu_checkbox("weapon", &config::context.draw_weapon);
                    inline_color("weapon color", config::context.esp_weapon_color, &config::context.esp_weapon_alpha);
					menu_checkbox("ammo bar", &config::context.draw_ammo_bar);
                    inline_color("ammo bar color", config::context.esp_ammo_color, &config::context.esp_ammo_alpha);
					if (config::context.draw_ammo_bar) menu_checkbox("smooth ammo", &config::context.ammo_smooth);
					menu_checkbox("skeleton", &config::context.draw_skeleton);
                    inline_color("skeleton color", config::context.esp_skeleton_color, &config::context.esp_skeleton_alpha);
					menu_checkbox("flags", &config::context.draw_flags);
                    inline_color("flags color", config::context.esp_flags_color, &config::context.esp_flags_alpha);
					menu_checkbox("out of fov arrow", &config::context.out_of_fov_arrows);
                    inline_color("out of fov arrow color", config::context.out_of_fov_arrow_color, &config::context.out_of_fov_arrow_opacity);
					menu_checkbox("hitmarker", &config::context.hitmarker);
                    inline_color("hitmarker color", config::context.hitmarker_color);

					if (config::context.draw_flags)
					{
						multi_select_combo("flags list", esp_flags_table, config::context.esp_flags);
						menu_combo("flags position", &config::context.esp_flags_position, flag_positions, IM_ARRAYSIZE(flag_positions));
					}

					if (config::context.out_of_fov_arrows)
					{
						menu_slider_float("arrow radius", &config::context.out_of_fov_arrow_radius, 40.f, 600.f, "%.0f");
						menu_slider_float("arrow size", &config::context.out_of_fov_arrow_size, 6.f, 32.f, "%.0f");
						menu_slider_float("arrow width", &config::context.out_of_fov_arrow_width, 0.2f, 1.f, "%.2f");
						menu_slider_float("arrow opacity", &config::context.out_of_fov_arrow_opacity, 0.f, 1.f, "%.2f");
						menu_checkbox("arrow outline", &config::context.out_of_fov_arrow_outline);
						menu_checkbox("arrow pulse", &config::context.out_of_fov_arrow_pulse);
					}

					if (config::context.hitmarker)
					{
						menu_slider_float("hitmarker time", &config::context.hitmarker_duration, 0.05f, 1.5f, "%.2f");
						menu_slider_float("hitmarker size", &config::context.hitmarker_size, 2.f, 32.f, "%.0f");
						menu_slider_float("hitmarker gap", &config::context.hitmarker_gap, 0.f, 24.f, "%.0f");
						menu_slider_float("hitmarker thickness", &config::context.hitmarker_thickness, 1.f, 6.f, "%.1f");
					}

					ImGui::Spacing();
                    ImGui::TextDisabled("colors");
					ImGui::PushID("esp_colors");
                    menu::color_edit4("enemy", config::context.esp_colors, &config::context.esp_opacity);
					menu_checkbox("by health", &config::context.esp_color_by_health);
					menu_slider_float("opacity", &config::context.esp_opacity, 0.05f, 1.f, "%.2f");
                    ImGui::PopID();

					ImGui::Spacing();
                    ImGui::TextDisabled("distance");

					menu_slider_float("max", &config::context.esp_max_distance, 0.f, 3000.f, "%.0f");
					menu_checkbox("fade", &config::context.esp_distance_fade);

					if (config::context.esp_distance_fade)
					{
						menu_checkbox("color by distance", &config::context.esp_color_by_distance);
					}

					
				menu_group_end();
                }

				menu_column_end();
                menu_column("visuals_right");
                menu_group("chams");
                menu_checkbox("chams", &config::context.chams);
                if (config::context.chams) {
                    menu_checkbox("local player", &config::context.chams_local);
                    menu_checkbox("visible", &config::context.chams_visible);
                    inline_color("visible color", config::context.chams_visible_color, &config::context.chams_visible_alpha);
                    if (config::context.chams_visible)
                        menu_combo("visible material", &config::context.chams_material, chams_materials, IM_ARRAYSIZE(chams_materials));
                    menu_checkbox("invisible", &config::context.chams_invisible);
                    inline_color("invisible color", config::context.chams_invisible_color, &config::context.chams_invisible_alpha);
                    if (config::context.chams_invisible)
                        menu_combo("invisible material", &config::context.chams_invisible_material, chams_materials, IM_ARRAYSIZE(chams_materials));
                }

                menu_group("glow");
                menu_checkbox("glow", &config::context.glow);
                inline_color("glow color", config::context.glow_color, &config::context.glow_alpha);
                if (config::context.glow) {
                    menu_checkbox("invincible", &config::context.glow_invincible);
                    inline_color("invincible color", config::context.glow_color_invincible, &config::context.glow_invincible_alpha);
                    menu_slider_float("brightness", &config::context.glow_brightness, 0.1f, 3.f, "%.2f");
                }

                menu_group("camera");
                menu_checkbox("hide crosshair", &config::context.hide_crosshair);
                menu_checkbox("fov changer", &config::context.fov_changer);
                if (config::context.fov_changer) {
                    menu_slider_float("fov", &config::context.fov, 60.f, 150.f, "%.1f");
                    menu_checkbox("scoped fov", &config::context.scopefov);
                }

                menu_group("viewmodel");
                menu_checkbox("viewmodel", &config::context.viewmodel);
                if (config::context.viewmodel) {
                    menu_slider_float("fov", &config::context.viewmodel_fov, 40.f, 120.f, "%.1f");
                    menu_slider_float("x", &config::context.viewmodel_x, -20.f, 20.f, "%.1f");
                    menu_slider_float("y", &config::context.viewmodel_y, -20.f, 20.f, "%.1f");
                    menu_slider_float("z", &config::context.viewmodel_z, -20.f, 20.f, "%.1f");
                }
                menu_column_end();
            }


            if (selected_tab == 2) {
                menu_column("movement");
                menu_group("movement");
                menu_checkbox("bhop", &config::context.bhop);
                bind_checkbox("ej", "ej", &config::context.ej);
                bind_checkbox("lj", "lj", &config::context.longjump);
                bind_checkbox("mj", "mini jump", &config::context.mj);
                if (config::context.mj) menu_checkbox("edge jump", &config::context.mj_ej);
                bind_checkbox("ladder_ej", "ladder ej", &config::context.ladder_ej);
                bind_checkbox("ladderbug", "ladderbug", &config::context.ladder_bug);
                bind_checkbox("jumpbug", "jumpbug", &config::context.jumpbug);
                bind_checkbox("autobounce", "autobounce", &config::context.autobounce);
                if (config::context.autobounce) {
                    menu_combo("mode", &config::context.autobounce_mode, "drop\0keep\0");
                    menu_slider_float("minimum gain", &config::context.autobounce_min_gain, 1.f, 200.f, "%.0f");
                    menu_slider_float("reach", &config::context.autobounce_reach, 16.f, 256.f, "%.0f");
                    menu_checkbox("align", &config::context.autobounce_align);
                    menu_checkbox("jump after bounce", &config::context.autobounce_jump);
                    menu_checkbox("crouch", &config::context.autobounce_crouch);
                    menu_checkbox("visualize", &config::context.autobounce_visualize);
                    if (config::context.autobounce_visualize)
                        inline_color("bounce color", config::context.autobounce_color);
                }

                menu_column_end();

                menu_column("movement_right");
                menu_group("edgebug");
                bind_checkbox("edgebug", "edgebug", &config::context.edgebug);
                if (config::context.edgebug) {
                    menu_slider_int("ticks", &config::context.edgebug_ticks, 1, 64);
                    menu_checkbox("chat detect", &config::context.edgebug_chat_print);
                    menu_checkbox("lock", &config::context.eb_lock);
                    if (config::context.eb_lock)
                        menu_checkbox("silent camera", &config::context.eb_silent);
                    bind_checkbox("visualize", "visualize", &config::context.visualize_edgebug);
                    inline_color("path color", config::context.edgebug_path_color);
                    if (config::context.visualize_edgebug)
                        menu_combo("shape", &config::context.edgebug_visual_shape, "box\0circle\0");
                }
                menu_column_end();
            }

            if (selected_tab == 3) {
                menu_column("misc");
                menu_group("watermark");
                menu_checkbox("watermark", &config::context.watermark);
                if (config::context.watermark) {
                    menu_slider_float("right offset", &config::context.watermark_x, 0.f, static_cast<float>(sdk::screen_width), "%.0f");
                    menu_slider_float("top offset", &config::context.watermark_y, 0.f, static_cast<float>(sdk::screen_height), "%.0f");
                }

                menu_group("spectator list");
                menu_checkbox("spectator list", &config::context.spec_list);
                inline_color("title color", config::context.spec_name_color);
                if (config::context.spec_list) {
                    menu_checkbox("spec all", &config::context.spec_all);
                    menu_checkbox("hide empty", &config::context.spec_list_hide_empty);
                    menu::color_edit4("player names", config::context.spec_player_name_color);
                    menu_slider_float("x", &config::context.speclist_pos.x, 0.f, static_cast<float>(sdk::screen_width), "%.0f");
                    menu_slider_float("y", &config::context.speclist_pos.y, 0.f, static_cast<float>(sdk::screen_height), "%.0f");
                }

                menu_group("velocity");
                menu_checkbox("velocity", &config::context.velocity);
                if (config::context.velocity) {
                    menu_slider_float("bottom offset", &config::context.velo_y, 0.f, static_cast<float>(sdk::screen_height), "%.0f");
                    menu_slider_float("size", &config::context.velo_size, 8.f, 48.f, "%.0f");
                    menu_checkbox("pre speed", &config::context.velo_pre_speed);
                    menu_checkbox("fade by speed", &config::context.velo_fade);
                    menu_checkbox("gradient color", &config::context.velo_gradient);
                    if (menu_button("reset velocity colors")) {
                        const config::config_t defaults{};
                        for (int i = 0; i < 5; ++i)
                            for (int channel = 0; channel < 3; ++channel)
                                config::context.velo_colors[i][channel] = defaults.velo_colors[i][channel];
                        config::context.velo_gradient = false;
                        config::context.velo_fade = false;
                    }
                    if (config::context.velo_gradient) {
                        menu::color_edit4("low speed", config::context.velo_colors[0]);
                        menu::color_edit4("high speed", config::context.velo_colors[1]);
                    } else {
                        menu::color_edit4("steady", config::context.velo_colors[2]);
                        menu::color_edit4("slowing", config::context.velo_colors[3]);
                        menu::color_edit4("accelerating", config::context.velo_colors[4]);
                    }
                }

                menu_group("movement indicators");
                menu_checkbox("indicators", &config::context.movement_indicators);
                inline_color("color", config::context.indicator_color);
                if (config::context.movement_indicators) {
                    menu::color_edit4("success color", config::context.indicator_success_color);
                    menu_slider_float("bottom offset", &config::context.indicator_offset, 0.f, static_cast<float>(sdk::screen_height), "%.0f");
                    menu_slider_float("size", &config::context.indicator_size, 8.f, 48.f, "%.0f");
                    const char* labels[] = { "eb", "ej", "lj", "mj", "jb", "lb" };
                    const int slots[] = { 0, 2, 3, 4, 5, 6 };
                    for (int i = 0; i < 6; ++i) menu_checkbox(labels[i], &config::context.indicator_items[slots[i]]);
                }

                menu_group("keystrokes");
                menu_checkbox("keystrokes", &config::context.keystrokes);
                if (config::context.keystrokes) {
                    menu_checkbox("show in menu", &config::context.show_in_menu);
                    menu_checkbox("show crouch", &config::context.show_crouch);
                    menu_checkbox("show jump", &config::context.show_in_jump);
                    menu_slider_float("x", &config::context.keystroke_pos.x, 0.f, static_cast<float>(sdk::screen_width), "%.0f");
                    menu_slider_float("y", &config::context.keystroke_pos.y, 0.f, static_cast<float>(sdk::screen_height), "%.0f");
                    menu_slider_float("size", &config::context.keystrokes_size, 16.f, 48.f, "%.0f");
                }
                menu_column_end();

                menu_column("misc_right");
                menu_group("world");
                menu_checkbox("world lighting", &config::context.world_lighting);
                inline_color("light color", config::context.world_light_color);
                if (config::context.world_lighting)
                    menu_slider_float("light intensity", &config::context.world_light_intensity, 0.f, 5.f, "%.2f");
                menu_checkbox("grenade trail", &config::context.grenade_trail);
                menu_checkbox("grenade preview", &config::context.grenade_preview);

                menu_group("skybox");
                menu_checkbox("skybox", &config::context.skybox);
                inline_color("skybox color", config::context.skybox_color);
                if (config::context.skybox)
                    menu_slider_float("intensity", &config::context.skybox_intensity, 0.f, 5.f, "%.2f");

                menu_group("practice");
                menu_checkbox("practice", &config::context.practice);
                if (config::context.practice) {
                    practice_keybind("save key", config::context.practice_save_key);
                    practice_keybind("load key", config::context.practice_load_key);
                    menu_checkbox("practice list", &config::context.practice_list);
                    if (config::context.practice_list) {
                        menu_slider_float("list x", &config::context.practice_list_pos.x, 0.f, static_cast<float>(sdk::screen_width), "%.0f");
                        menu_slider_float("list y", &config::context.practice_list_pos.y, 0.f, static_cast<float>(sdk::screen_height), "%.0f");
                    }
                    if (menu_button("save")) misc::practice_request(false);
                    if (menu_button("load")) misc::practice_request(true);
                }
                menu_column_end();
            }

            if (selected_tab == 5) {
                static bool refreshed = false;
                static std::string selected_script;
                if (!refreshed) { scripts::refresh(); refreshed = true; }
                auto entries = scripts::entries();
                menu_column("lua scripts");
                menu_group("scripts");
                if (ImGui::BeginListBox("##scripts", ImVec2(ImGui::GetContentRegionAvail().x, 160.f))) {
                    for (const auto& entry : entries)
                        if (ImGui::Selectable(entry.name.c_str(), selected_script == entry.name)) selected_script = entry.name;
                    ImGui::EndListBox();
                }
                if (menu_button("refresh")) scripts::refresh();
                menu_column_end();
                menu_column("lua selected script");
                menu_group("script");
                const auto found = std::find_if(entries.begin(),entries.end(),[&](const auto& entry){return entry.name == selected_script;});
                const bool selected = found != entries.end();
                ImGui::BeginDisabled(!selected);
                if (menu_button(selected && found->loaded ? "reload" : "load")) scripts::load(selected_script);
                ImGui::BeginDisabled(!selected || !found->loaded);
                if (menu_button("unload")) scripts::unload(selected_script);
                ImGui::EndDisabled();
                ImGui::EndDisabled();
                if (selected) {
                    ImGui::TextUnformatted(found->loaded ? "loaded" : "unloaded");
                    if (!found->error.empty()) ImGui::TextWrapped("%s",found->error.c_str());
                }
                menu_column_end();
            }
			if (selected_tab == 4)
			{
				menu_column("configs");
                menu_group("configs");
				static bool needs_refresh = true;

				if (needs_refresh)
				{
					refresh_config_files();
					needs_refresh = false;
				}

				if (ImGui::BeginListBox("##configs", ImVec2(ImGui::GetContentRegionAvail().x - 4.f, 112.f)))
				{
					if (config_files.empty()) ImGui::TextDisabled("no configs");
					for (int i = 0; i < static_cast<int>(config_files.size()); i++)
					{
						if (ImGui::Selectable(config_files[i].c_str(), selected_config == i))selected_config = i;
					}

					ImGui::EndListBox();
				}

				const bool has_selection = selected_config >= 0 && selected_config < static_cast<int>(config_files.size());
				ImGui::BeginDisabled(!has_selection);
				if (menu_button("load", ImVec2(ImGui::GetContentRegionAvail().x - 4.f, 22.f)) && has_selection)
				{
					config::load(config_files[selected_config].c_str());
				}
				

				if (menu_button("save", ImVec2(ImGui::GetContentRegionAvail().x - 4.f, 22.f)) && has_selection)
				{
					config::save(config_files[selected_config].c_str());
				}
				

				if (menu_button("delete", ImVec2(ImGui::GetContentRegionAvail().x - 4.f, 22.f)) && has_selection)
				{
					config::remove(config_files[selected_config].c_str());
					refresh_config_files();
				}
				ImGui::EndDisabled();
				if (menu_button("refresh", ImVec2(ImGui::GetContentRegionAvail().x - 4.f, 22.f))) refresh_config_files();
				menu_column_end();
                menu_column("create config");
                menu_group("create config");
				ImGui::TextUnformatted("name");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 4.f);
                ImGui::InputText("##config_name", config_name_buffer, sizeof(config_name_buffer));

				ImGui::BeginDisabled(config_name_buffer[0] == '\0');
				if (menu_button("create", ImVec2(ImGui::GetContentRegionAvail().x - 4.f, 22.f)))
				{
					if (config_name_buffer[0])
					{
						config::save(config_name_buffer);
						refresh_config_files();
					}
				}
				ImGui::EndDisabled();
				menu_group("appearance");
                menu::color_edit4("menu accent", config::context.menu_accent);
				menu_column_end();
			}
		}
	}
	ImGui::End();
	if (menu::font) ImGui::PopFont();
}

